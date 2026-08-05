#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "coro/frame_stats.hpp"
#include "coro/generator.hpp"
#include "coro/task.hpp"
#include "examples.hpp"

TEST_CASE("生成器惰性产出斐波那契数列", "[generator]")
{
  std::vector<std::uint64_t> values;
  for (auto value : examples::fibonacci(10)) {
    values.push_back(value);
  }
  REQUIRE(values == std::vector<std::uint64_t> {0, 1, 1, 2, 3, 5, 8, 13, 21, 34});
}

TEST_CASE("生成器是惰性的：不 resume 就不执行函数体", "[generator]")
{
  bool started = false;
  auto make = [&]() -> coro::generator<int>
  {
    started = true;  // 只有 resume 之后才会执行到这里
    co_yield 1;
  };

  auto gen = make();
  REQUIRE_FALSE(started);  // 调用只分配了协程帧

  REQUIRE(gen.next());
  REQUIRE(started);
  REQUIRE(gen.value() == 1);
  REQUIRE_FALSE(gen.next());  // 没有更多值
}

TEST_CASE("协程体内的异常传播到消费者", "[generator]")
{
  auto gen = examples::throwing_generator(7);
  REQUIRE(gen.next());
  REQUIRE(gen.value() == 7);
  REQUIRE_THROWS_AS(gen.next(), std::runtime_error);
}

TEST_CASE("协程帧在堆上分配并随包装对象销毁", "[frame]")
{
  coro::frame_stats::reset();
  {
    auto gen = examples::fibonacci(5);
    REQUIRE(coro::frame_stats::live_frames == 1);
    REQUIRE(coro::frame_stats::last_frame_size > 0);
    REQUIRE(gen.next());
  }
  // generator 析构 -> handle.destroy() -> 帧释放
  REQUIRE(coro::frame_stats::live_frames == 0);
}

TEST_CASE("co_await 任务链得到正确结果", "[task]")
{
  REQUIRE(coro::sync_wait(examples::async_sum(19, 23)) == 42);
}

TEST_CASE("任务是惰性的：start 之前不创建子任务", "[task]")
{
  coro::frame_stats::reset();
  auto chain = examples::async_sum(1, 2);
  REQUIRE(coro::frame_stats::total_allocations == 1);  // 只有 async_sum 自己的帧
  REQUIRE_FALSE(chain.done());
  REQUIRE(coro::sync_wait(std::move(chain)) == 3);
}

TEST_CASE("任务内的异常经 result 重新抛出", "[task]")
{
  auto failing = []() -> coro::task<int>
  {
    throw std::runtime_error {"task exploded"};
    co_return 0;  // 不可达，但让编译器把函数当协程处理
  };
  REQUIRE_THROWS_AS(coro::sync_wait(failing()), std::runtime_error);
}

TEST_CASE("调度器让两个协程确定性轮转", "[scheduler]")
{
  auto const log = examples::round_robin(2);
  REQUIRE(log == std::vector<std::string> {"ping:1", "pong:1", "ping:2", "pong:2"});
}

TEST_CASE("定时器驱动的协程按到期时间交错", "[scheduler]")
{
  // fast 每 20ms（3 次），slow 每 50ms（2 次）
  auto const log = examples::interleaved_tickers();
  REQUIRE(log == std::vector<std::string> {"fast#1", "fast#2", "slow#1", "fast#3", "slow#2"});
}
