#include <chrono>
#include <iostream>
#include <memory>

#include "csp_channel.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// CSP（Communicating Sequential Processes，Hoare 1978）是 Go 并发模型的
// 理论来源。与 actor 模型的差别在"中心"不同：
//
//   actor：以进程为中心。消息发给某个进程（Pid），邮箱属于进程。
//   CSP  ：以信道为中心。channel 是第一等公民，收发双方彼此匿名，
//          进程只认识 channel，不认识对方。
//
// Go 基础语法逐条对应：
//
//   Go                          这里
//   --------------------------  ------------------------------------
//   go f(ch)                    co_spawn(executor, f(ch), detached)
//   make(chan T)                make_shared<channel>(ex, 0)   无缓冲
//   make(chan T, 3)             make_shared<channel>(ex, 3)   有缓冲
//   ch <- v                     co_await ch->async_send({}, v, ...)
//   v := <-ch                   co_await ch->async_receive(...)
//   close(ch)                   ch->close()
//   for v := range ch           循环 receive 直到 channel_closed
//
// 无缓冲 channel 是 CSP 的灵魂：发送和接收必须"会合"（rendezvous），
// 双方都到场数据才交接——通信同时就是同步。

namespace modules::csp_channel
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::asio::experimental::awaitable_operators::operator&&;

using int_channel =
    boost::asio::experimental::channel<void(boost::system::error_code, int)>;
using int_channel_ptr = std::shared_ptr<int_channel>;

constexpr auto receiver_delay = 30ms;
constexpr auto consumer_delay = 25ms;
constexpr int buffered_capacity = 3;
constexpr int range_item_count = 5;

// ---- 1. 无缓冲：会合语义 ----

auto rendezvous_sender(int_channel_ptr chan) -> awaitable<void>
{
  std::cout << "sender: sending 42 (unbuffered, will wait for receiver)\n";
  co_await chan->async_send({}, 42, use_awaitable);
  std::cout << "sender: handoff complete, send returned\n";
}

auto rendezvous_receiver(int_channel_ptr chan) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer delay {executor, receiver_delay};
  co_await delay.async_wait(use_awaitable);
  std::cout << "receiver: ready after 30ms\n";
  const int value = co_await chan->async_receive(use_awaitable);
  std::cout << "receiver: got " << value << '\n';
}

auto rendezvous_demo() -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  // 容量 0 == Go 的 make(chan int)：send 挂起直到 receive 到场。
  auto chan = std::make_shared<int_channel>(executor, 0);
  co_await (rendezvous_sender(chan) && rendezvous_receiver(chan));
}

// ---- 2. 有缓冲：解耦生产与消费的节奏 ----

auto buffered_producer(int_channel_ptr chan) -> awaitable<void>
{
  for (int value = 1; value <= buffered_capacity + 1; ++value) {
    co_await chan->async_send({}, value, use_awaitable);
    std::cout << "producer: send " << value
              << (value <= buffered_capacity ? " returned immediately\n"
                                             : " completed (a slot freed)\n");
  }
}

auto buffered_consumer(int_channel_ptr chan) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer delay {executor, consumer_delay};
  co_await delay.async_wait(use_awaitable);
  std::cout << "consumer: starts draining after 25ms\n";
  for (int i = 0; i < buffered_capacity + 1; ++i) {
    const int value = co_await chan->async_receive(use_awaitable);
    std::cout << "consumer: got " << value << '\n';
  }
}

auto buffered_demo() -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  // 容量 3 == make(chan int, 3)：前 3 个 send 不等消费者。
  auto chan = std::make_shared<int_channel>(executor, buffered_capacity);
  co_await (buffered_producer(chan) && buffered_consumer(chan));
}

// ---- 3. close + range：生产者宣告"没有了" ----

auto range_producer(int_channel_ptr chan) -> awaitable<void>
{
  for (int value = 1; value <= range_item_count; ++value) {
    co_await chan->async_send({}, value * 10, use_awaitable);
  }
  chan->close();  // close(ch)
  std::cout << "producer: sent " << range_item_count << " items and closed\n";
}

auto range_consumer(int_channel_ptr chan) -> awaitable<void>
{
  // for v := range ch 的展开形式：receive 到 channel_closed 为止。
  for (;;) {
    auto [err, value] = co_await chan->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    std::cout << "consumer: range got " << value << '\n';
  }
  std::cout << "consumer: range ended (channel closed and drained)\n";
}

auto range_demo() -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  auto chan = std::make_shared<int_channel>(executor, 2);
  co_await (range_producer(chan) && range_consumer(chan));
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  boost::asio::co_spawn(
      ioc,
      []() -> awaitable<void>
      {
        std::cout << "-- unbuffered (rendezvous) --\n";
        co_await rendezvous_demo();
        std::cout << "-- buffered (capacity 3) --\n";
        co_await buffered_demo();
        std::cout << "-- close + range --\n";
        co_await range_demo();
      },
      boost::asio::detached);

  ioc.run();
}

}  // namespace modules::csp_channel
