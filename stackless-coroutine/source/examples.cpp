#include "examples.hpp"

#include <coroutine>
#include <stdexcept>
#include <utility>

#include <fmt/core.h>

#include "coro/frame_stats.hpp"

namespace examples
{

// ---- co_yield：生成器 ----

auto fibonacci(std::size_t count) -> coro::generator<std::uint64_t>
{
  // a、b、i 都存活在堆上的协程帧里，而不是线程栈上。
  // 每次 co_yield 挂起后，线程栈上属于本函数的部分完全弹出，
  // 但这些变量的值原封不动地留在帧中，等待下一次 resume。
  std::uint64_t a = 0;
  std::uint64_t b = 1;
  for (std::size_t i = 0; i < count; ++i) {
    co_yield a;  // 挂起点：把 a 交给消费者，控制权回到调用方
    a = std::exchange(b, a + b);
  }
}  // 循环结束，隐式 co_return，协程进入"最终挂起"状态

auto throwing_generator(int first) -> coro::generator<int>
{
  co_yield first;
  // 协程体内未捕获的异常会进入 promise.unhandled_exception()，
  // 暂存为 exception_ptr，在消费者调用 next() 时重新抛出
  throw std::runtime_error {"generator exploded"};
}

// ---- co_await / co_return：任务组合 ----

auto async_value(int value) -> coro::task<int>
{
  // 这里可以是任何真正的异步操作（网络、磁盘、定时器…），
  // 演示里直接返回，重点在于调用方"以同步的写法"拿到结果
  co_return value;
}

auto async_sum(int lhs, int rhs) -> coro::task<int>
{
  // co_await 展开后：挂起自己 -> 对称转移到子任务 -> 子任务 co_return
  // 后经 final_awaiter 跳回这里 -> await_resume() 取出结果
  int const left = co_await async_value(lhs);
  int const right = co_await async_value(rhs);
  co_return left + right;
}

// ---- 调度器：单线程协作式并发 ----

auto ticker(coro::scheduler& sched,
            std::string name,
            int count,
            std::chrono::milliseconds interval,
            std::vector<std::string>& log) -> coro::task<void>
{
  // 形参 name 按值传入，副本保存在协程帧里，跨越所有挂起点仍然有效。
  // 注意：按引用传入的 sched 和 log 必须比协程活得久（见 README 陷阱一节）。
  for (int i = 1; i <= count; ++i) {
    co_await sched.sleep_for(interval);  // 挂起，线程去跑别的协程
    log.push_back(fmt::format("{}#{}", name, i));
  }
}

auto interleaved_tickers() -> std::vector<std::string>
{
  using namespace std::chrono_literals;

  coro::scheduler sched;
  std::vector<std::string> log;

  // 两个任务都是惰性的：创建时只分配了协程帧，还没执行任何代码
  auto slow = ticker(sched, "slow", 2, 50ms, log);
  auto fast = ticker(sched, "fast", 3, 20ms, log);

  slow.start();  // 执行到第一个 co_await sleep_for 后挂起
  fast.start();
  sched.run();  // 单线程驱动两个协程按定时器交错执行

  return log;  // 预期：fast#1 fast#2 slow#1 fast#3 slow#2
}

namespace
{

auto polite_worker(coro::scheduler& sched,
                   std::string name,
                   int rounds,
                   std::vector<std::string>& log) -> coro::task<void>
{
  for (int i = 1; i <= rounds; ++i) {
    log.push_back(fmt::format("{}:{}", name, i));
    co_await sched.yield();  // 主动让出，排到就绪队列尾部
  }
}

}  // namespace

auto round_robin(int rounds) -> std::vector<std::string>
{
  coro::scheduler sched;
  std::vector<std::string> log;

  auto ping = polite_worker(sched, "ping", rounds, log);
  auto pong = polite_worker(sched, "pong", rounds, log);

  ping.start();
  pong.start();
  sched.run();

  return log;  // 确定性交错：ping:1 pong:1 ping:2 pong:2 ...
}

// ---- 演示入口 ----

namespace
{

/// 手写一个最小 awaitable，打印协议三件套被调用的顺序
struct logging_awaiter
{
  auto await_ready() const -> bool
  {
    fmt::print("    await_ready()   -> false：告诉编译器需要挂起\n");
    return false;
  }

  auto await_suspend(std::coroutine_handle<> handle) const -> bool
  {
    fmt::print("    await_suspend() -> 协程已挂起，帧地址 = {}\n", handle.address());
    fmt::print("                       真实场景会把这个句柄交给事件循环保存\n");
    return false;  // 返回 false：演示用，立即恢复协程
  }

  void await_resume() const
  {
    fmt::print("    await_resume()  -> 协程从挂起点恢复\n");
  }
};

auto awaitable_protocol_demo() -> coro::task<void>
{
  fmt::print("  co_await 之前\n");
  co_await logging_awaiter {};
  fmt::print("  co_await 之后（这行代码位于状态机的\"下一个状态\"里）\n");
}

void demo_generator()
{
  fmt::print("\n=== 1. co_yield 与协程帧 ===\n");
  coro::frame_stats::reset();
  {
    auto fib = fibonacci(10);
    fmt::print("  调用 fibonacci(10) 后：帧数 = {}，帧大小 = {} 字节（堆分配，惰性，尚未执行函数体）\n",
               coro::frame_stats::live_frames,
               coro::frame_stats::last_frame_size);
    fmt::print("  前 10 个斐波那契数：");
    for (auto value : fib) {  // 每次 ++ 迭代器都 resume 一次协程
      fmt::print("{} ", value);
    }
    fmt::print("\n");
  }  // fib 析构 -> handle.destroy() -> 协程帧释放
  fmt::print("  generator 离开作用域后：帧数 = {}（RAII 销毁协程帧）\n",
             coro::frame_stats::live_frames);
}

void demo_awaitable_protocol()
{
  fmt::print("\n=== 2. awaitable 协议三件套 ===\n");
  coro::sync_wait(awaitable_protocol_demo());
}

void demo_task_chain()
{
  fmt::print("\n=== 3. co_await / co_return 任务链 ===\n");
  coro::frame_stats::reset();
  auto chain = async_sum(19, 23);
  fmt::print("  创建 async_sum(19, 23)：帧数 = {}（惰性，子任务尚未创建）\n",
             coro::frame_stats::live_frames);
  int const result = coro::sync_wait(std::move(chain));
  fmt::print("  sync_wait 结果 = {}\n", result);
}

void demo_scheduler()
{
  fmt::print("\n=== 4. 单线程协作式并发（事件循环 + 定时器）===\n");
  fmt::print("  slow 每 50ms 打点 2 次，fast 每 20ms 打点 3 次，同一个线程交错执行：\n  ");
  for (auto const& entry : interleaved_tickers()) {
    fmt::print("{} ", entry);
  }
  fmt::print("\n  确定性轮转（co_await yield()）：\n  ");
  for (auto const& entry : round_robin(3)) {
    fmt::print("{} ", entry);
  }
  fmt::print("\n");
}

}  // namespace

void run_all_demos()
{
  fmt::print("C++20 无栈协程演示 —— 详解见 README.md\n");
  demo_generator();
  demo_awaitable_protocol();
  demo_task_chain();
  demo_scheduler();
}

}  // namespace examples
