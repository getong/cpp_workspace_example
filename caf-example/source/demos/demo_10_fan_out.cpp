// 10 并行分发/汇聚 (scatter-gather) —— 一行代码的 map-reduce:
//
// * 每个 actor 都可能被调度到不同线程, spawn 多个 worker 即获得并行
// * fan_out_request<select_all> 把同一条消息广播给一组 worker,
//   等全部应答到齐后, 以 vector 形式一次性交给回调
// * select_all 之外还有 select_any: 谁先答复就用谁(冗余竞速)
//
// 本例把 1..100000 的求和切成 4 段, 由 4 个 worker 并行计算再汇总。

#include <chrono>
#include <cstdint>
#include <numeric>
#include <vector>

#include <caf/all.hpp>
#include <caf/policy/select_all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"

namespace
{

// worker 在创建时得到自己的分片编号, 收到分片大小后计算本段区间和
auto range_summer_impl(int64_t index) -> caf::behavior
{
  return {
      [index](int64_t chunk_size)
      {
        auto first = index * chunk_size + 1;
        auto last = (index + 1) * chunk_size;
        int64_t sum = 0;
        for (auto value = first; value <= last; ++value) {
          sum += value;
        }
        return sum;
      },
  };
}

}  // namespace

namespace demo
{

void fan_out(caf::actor_system& sys)
{
  sys.println("=== 10 fan_out: 广播任务并汇聚全部结果 ===");
  constexpr int64_t worker_count = 4;
  constexpr int64_t chunk_size = 25000;
  std::vector<caf::actor> workers;
  workers.reserve(worker_count);
  for (int64_t index = 0; index < worker_count; ++index) {
    workers.push_back(sys.spawn(range_summer_impl, index));
  }
  caf::scoped_actor self {sys};
  self->fan_out_request<caf::policy::select_all>(
          workers, std::chrono::seconds(5), chunk_size)
      .receive(
          [&self](std::vector<int64_t> partial_sums)
          {
            auto total = std::accumulate(
                partial_sums.begin(), partial_sums.end(), int64_t {0});
            self->println("  4 个 worker 并行计算 1..100000 之和 = {}", total);
          },
          [&self](const caf::error& err)
          { self->println("  汇聚失败: {}", caf::to_string(err)); });
  for (auto& worker : workers) {
    self->send_exit(worker, caf::exit_reason::user_shutdown);
  }
}

}  // namespace demo
