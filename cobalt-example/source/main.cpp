// boost::cobalt 采用 CSP (Communicating Sequential Processes) 并发模型，
// 与 Go 相同：协程之间不共享可变状态，而是通过 channel 通信来传递数据。
//
// 本文件演示两个最典型的 CSP 模式：
//   1. 流水线 (pipeline)：produce -> square -> 消费，channel 关闭作为结束信号
//   2. 多路复用 (race)：等价于 Go 的 select，从多个 channel 中取先就绪的消息

#include <chrono>
#include <iostream>
#include <string>

#include <boost/asio/steady_timer.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/race.hpp>
#include <boost/cobalt/result.hpp>
#include <boost/variant2/variant.hpp>

#include "lib.hpp"

namespace cobalt = boost::cobalt;
namespace asio = boost::asio;
using namespace std::chrono_literals;

// ---- 示例 1: 流水线 ----

// 生产者：向 channel 写满数据后 close，下游以 is_open() 感知结束
auto produce(cobalt::channel<int>& out, int count) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await out.write(i);
  }
  out.close();
}

// 中间阶段：从上游读、变换、写给下游，上游关闭后同样关闭下游。
// as_result 让 read 在 channel 关闭时返回错误而不是抛异常
auto square(cobalt::channel<int>& in, cobalt::channel<int>& out)
    -> cobalt::promise<void>
{
  while (true) {
    auto const value = co_await cobalt::as_result(in.read());
    if (!value) {  // 上游已关闭
      break;
    }
    co_await out.write(*value * *value);
  }
  out.close();
}

// ---- 示例 2: race 多路复用 ----

// 定时向 channel 发送消息，模拟两个节奏不同的事件源
auto ticker(cobalt::channel<std::string>& out,
            std::string name,
            std::chrono::milliseconds interval,
            int count) -> cobalt::promise<void>
{
  asio::steady_timer timer {co_await cobalt::this_coro::executor};
  for (int i = 1; i <= count; ++i) {
    timer.expires_after(interval);
    co_await timer.async_wait(cobalt::use_op);
    co_await out.write(name + " tick #" + std::to_string(i));
  }
}

// cobalt::main 在单线程 io_context 上运行，并自动把 Ctrl+C 转为取消信号
auto co_main(int /*argc*/, char* /*argv*/[]) -> cobalt::main
{
  auto const lib = library {};
  std::cout << "Hello from " << lib.name << "!\n";

  // 示例 1: produce(1..5) -> square -> 打印。
  // 默认 channel 缓冲为 0，读写构成 rendezvous，天然背压。
  cobalt::channel<int> numbers;
  cobalt::channel<int> squares;

  auto producer = produce(numbers, 5);
  auto transformer = square(numbers, squares);

  while (true) {
    auto const value = co_await cobalt::as_result(squares.read());
    if (!value) {  // 流水线已结束
      break;
    }
    std::cout << "pipeline: " << *value << '\n';
  }
  co_await producer;
  co_await transformer;

  // 示例 2: 用 race 同时等待两个 channel，谁先就绪就处理谁（Go 的 select）。
  // race 对 channel 使用 interrupt_await，落选分支不会丢消息。
  cobalt::channel<std::string> fast;
  cobalt::channel<std::string> slow;

  auto fast_ticker = ticker(fast, "fast", 50ms, 4);
  auto slow_ticker = ticker(slow, "slow", 120ms, 2);

  for (int received = 0; received < 6; ++received) {
    auto msg = co_await cobalt::race(fast.read(), slow.read());
    std::cout << "select: "
              << boost::variant2::visit(
                     [](std::string const& s) -> std::string const&
                     { return s; },
                     msg)
              << '\n';
  }

  co_await fast_ticker;
  co_await slow_ticker;
  co_return 0;
}
