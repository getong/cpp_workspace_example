// 11 数据流 (flow API) —— actor 内的响应式流水线:
//
// * make_observable() 在 actor 内创建可观察序列(observable),
//   支持 map/filter/take/reduce 等组合子, 风格类似 RxCpp/RxJava
// * 数据按背压(backpressure)逐个流经流水线, 不会淹没消费者
// * observable 还可以跨 actor 传输(caf::async), 构建流式处理拓扑;
//   这里演示最基本的单 actor 流水线
//
// 流水线: 1,2,3,... -> 取前 10 个 -> 保留偶数 -> 平方 -> 打印并求和

#include <cstdint>

#include <caf/all.hpp>
#include <caf/flow/observable_builder.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"

namespace
{

void pipeline_impl(caf::event_based_actor* self)
{
  self->make_observable()
      .iota(int32_t {1})  // 无穷序列 1,2,3,...
      .take(10)  // 只取前 10 个
      .filter([](int32_t value) { return value % 2 == 0; })
      .map([](int32_t value) { return value * value; })
      .do_on_next([self](int32_t value)
                  { self->println("  流元素: {}", value); })
      .sum()  // 汇聚: 流结束时发出唯一的总和
      .for_each([self](int32_t total)
                { self->println("  流结束, 偶数平方和 = {}", total); });
  // 流跑完后 actor 没有其他行为, 会自动终止
}

}  // namespace

namespace demo
{

void data_flow(caf::actor_system& sys)
{
  sys.println("=== 11 data_flow: 响应式数据流水线 ===");
  auto pipeline = sys.spawn(pipeline_impl);
  caf::scoped_actor self {sys};
  self->wait_for(pipeline);
}

}  // namespace demo
