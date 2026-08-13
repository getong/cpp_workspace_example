#include <array>
#include <chrono>
#include <iostream>
#include <memory>

#include "csp_mpsc.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// MPSC（Multi-Producer Single-Consumer）：多个生产者共享**同一根**
// channel，消息汇聚到唯一的消费者。
//
// 功能与作用：
//
// 1. 汇聚（fan-in 的退化形态）：把多个并发来源的事件合并成**一条
//    串行流**。消费者天然一次只处理一条消息，处理逻辑不需要任何
//    同步——actor 的 mailbox 本质上就是一根 MPSC channel。
// 2. 替代"mutex + queue + condition_variable"三件套：传统的工作队列/
//    日志队列要手写锁和唤醒，MPSC channel 把入队（含背压）、唤醒、
//    出队封装成两个 await 点。
// 3. 序保证要说清楚：**每个生产者各自的消息保持 FIFO**（p1 的 #2
//    绝不会先于 p1 的 #1 到达），不同生产者之间按实际到达交错，
//    没有全局定序。需要全局序时由消费者按业务字段重排。
// 4. 关停协议是 MPSC 特有难题：任何一个生产者都不能擅自 close
//    （channel 是大家共享的）。惯用解法有两种，本模块各演示一个：
//    join-then-close（等全体生产者收工再关）与计数消费者（消费者
//    知道预期总量，收满即止）。
//
// asio 的 channel 本身不限制端点数（MPMC 皆可），MPSC/SPSC 只是
// 使用约定。跨线程共享时必须换 concurrent_channel（demo 2）；
// 单线程/同一 strand 上普通 channel 即可（demo 1）。

namespace modules::csp_mpsc
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::asio::experimental::awaitable_operators::operator&&;

struct message
{
  int producer_id;
  int seq;
};

// ---- demo 1：单线程 MPSC 基础形态 --------------------------------
//
// 两个节奏不同的生产者协程 + 一个消费者协程，三方 co_spawn 在同一个
// io_context 上，共享一根 channel。

using channel_type =
    boost::asio::experimental::channel<void(boost::system::error_code,
                                            message)>;
using channel_ptr = std::shared_ptr<channel_type>;

constexpr int per_producer = 3;
constexpr auto cadence_p1 = 10ms;
constexpr auto cadence_p2 = 17ms;

auto producer(int producer_id, std::chrono::milliseconds cadence,
              channel_ptr out) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer timer {executor};
  for (int seq = 1; seq <= per_producer; ++seq) {
    timer.expires_after(cadence);
    co_await timer.async_wait(use_awaitable);
    // 多个生产者可以同时挂起在同一根 channel 的 send 上，互不干扰。
    co_await out->async_send({}, message {producer_id, seq}, use_awaitable);
  }
  std::cout << "producer " << producer_id << ": finished\n";
}

// 关停协议之一：join-then-close。&& 等两个生产者都收工，
// 再由这个"监工"协程统一 close——生产者自己谁都无权关共享通道。
auto producers_then_close(channel_ptr out) -> awaitable<void>
{
  co_await (producer(1, cadence_p1, out) && producer(2, cadence_p2, out));
  out->close();
  std::cout << "closer: both producers done, close(channel)\n";
}

auto consumer(channel_ptr input) -> awaitable<void>
{
  for (;;) {
    auto [err, msg] = co_await input->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    // 单消费者独享出口：这里的处理天然串行，不需要任何锁。
    std::cout << "consumer: got p" << msg.producer_id << " #" << msg.seq
              << '\n';
  }
  std::cout << "consumer: channel closed, exiting\n";
}

void single_thread_demo()
{
  boost::asio::io_context ioc;
  auto chan = std::make_shared<channel_type>(ioc, 8);

  boost::asio::co_spawn(ioc, producers_then_close(chan),
                        boost::asio::detached);
  boost::asio::co_spawn(ioc, consumer(chan), boost::asio::detached);
  ioc.run();
}

// ---- demo 2：跨线程 MPSC（concurrent_channel） --------------------
//
// 4 个生产者协程跑在 thread_pool 的多个线程上**并行**生产，消费者
// 单独跑在主线程的 io_context 上。concurrent_channel 内部做了
// 线程安全，成为并行世界与串行世界之间的唯一衔接点——
// 这是日志收集器、事件总线、指标上报等场景的标准形态。

using mt_channel =
    boost::asio::experimental::concurrent_channel<void(
        boost::system::error_code, message)>;
using mt_channel_ptr = std::shared_ptr<mt_channel>;

constexpr int mt_producers = 4;
constexpr int mt_per_producer = 250;
constexpr int mt_capacity = 64;

auto mt_producer(int producer_id, mt_channel_ptr out) -> awaitable<void>
{
  for (int seq = 1; seq <= mt_per_producer; ++seq) {
    co_await out->async_send({}, message {producer_id, seq}, use_awaitable);
  }
}

// 关停协议之二：计数消费者——预期总量已知，收满即止，无须 close。
auto mt_consumer(mt_channel_ptr input) -> awaitable<void>
{
  std::array<int, mt_producers + 1> last_seq {};
  std::array<int, mt_producers + 1> count {};
  int fifo_violations = 0;

  for (int received = 0; received < mt_producers * mt_per_producer;
       ++received)
  {
    const message msg = co_await input->async_receive(use_awaitable);
    // 每个生产者的消息必须按它发送的顺序到达。
    if (msg.seq != last_seq.at(msg.producer_id) + 1) {
      ++fifo_violations;
    }
    last_seq.at(msg.producer_id) = msg.seq;
    ++count.at(msg.producer_id);
  }

  std::cout << "consumer: received " << mt_producers * mt_per_producer
            << " messages from " << mt_producers
            << " producer threads, no locks in user code\n";
  for (int id = 1; id <= mt_producers; ++id) {
    std::cout << "  producer " << id << ": " << count.at(id)
              << " messages, per-producer FIFO "
              << (last_seq.at(id) == mt_per_producer ? "intact" : "BROKEN")
              << '\n';
  }
  std::cout << "consumer: per-producer FIFO violations: " << fifo_violations
            << " (expected 0)\n";
}

void multi_thread_demo()
{
  boost::asio::io_context ioc;
  boost::asio::thread_pool pool {mt_producers};

  auto chan = std::make_shared<mt_channel>(ioc, mt_capacity);

  // 生产者 spawn 到线程池（真并行），消费者 spawn 到主线程 ioc。
  for (int id = 1; id <= mt_producers; ++id) {
    boost::asio::co_spawn(pool, mt_producer(id, chan),
                          boost::asio::detached);
  }
  boost::asio::co_spawn(ioc, mt_consumer(chan), boost::asio::detached);

  ioc.run();  // 消费者收满即返回
  pool.join();
}

}  // namespace

void run()
{
  std::cout << "-- single-thread MPSC: 2 producers -> 1 consumer --\n";
  single_thread_demo();
  std::cout << "-- multi-thread MPSC: 4 producer threads -> 1 consumer "
               "(concurrent_channel) --\n";
  multi_thread_demo();
}

}  // namespace modules::csp_mpsc
