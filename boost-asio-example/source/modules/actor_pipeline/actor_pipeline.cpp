#include <chrono>
#include <iostream>
#include <memory>

#include "actor_pipeline.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// 流水线：producer -> square -> sink，三个 actor 用有界 mailbox 串联。
// 这里演示 actor 系统里两个重要的"自动"性质：
//
// 1. 背压（backpressure）：mailbox 容量有限，下游慢时 async_send 会
//    挂起上游。观察输出：producer 只领先 square 一个容量的距离就停下，
//    不需要任何显式的流控代码。Erlang 邮箱是无界的，过载要靠额外机制
//    兜底；有界 channel 把这层保护内建了。
//
// 2. 关停传播：上游 close() 自己的输出通道，下游 receive 得到
//    channel_closed 错误后关闭再下一级——EOF 沿流水线逐级传递，
//    类似 Erlang 链接进程的退出信号级联。

namespace modules::actor_pipeline
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

using int_channel =
    boost::asio::experimental::channel<void(boost::system::error_code, int)>;
using int_channel_ptr = std::shared_ptr<int_channel>;

constexpr int channel_capacity = 2;
constexpr int item_count = 6;
constexpr auto stage_delay = 15ms;

// 上游：一口气生产 6 个数。前几个 send 立即完成（进入缓冲区），
// 缓冲区满后每次 send 都要等下游腾出位置。
auto producer(int_channel_ptr out) -> awaitable<void>
{
  for (int value = 1; value <= item_count; ++value) {
    co_await out->async_send({}, value, use_awaitable);
    std::cout << "producer: sent " << value << '\n';
  }
  out->close();
  std::cout << "producer: done, closed output\n";
}

// 中游：每个数平方，模拟 15ms 的处理耗时——故意比上游慢。
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto square_stage(int_channel_ptr input, int_channel_ptr out) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer work {executor};

  for (;;) {
    auto [err, value] = co_await input->async_receive(as_tuple(use_awaitable));
    if (err) {  // 上游已关闭且缓冲区取空
      break;
    }
    work.expires_after(stage_delay);
    co_await work.async_wait(use_awaitable);
    co_await out->async_send({}, value * value, use_awaitable);
  }
  out->close();
  std::cout << "square: upstream closed, closed output\n";
}

// 下游：消费并打印。
auto sink(int_channel_ptr input) -> awaitable<void>
{
  for (;;) {
    auto [err, value] = co_await input->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    std::cout << "sink: got " << value << '\n';
  }
  std::cout << "sink: pipeline drained\n";
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  auto stage1 = std::make_shared<int_channel>(ioc, channel_capacity);
  auto stage2 = std::make_shared<int_channel>(ioc, channel_capacity);

  boost::asio::co_spawn(ioc, producer(stage1), boost::asio::detached);
  boost::asio::co_spawn(
      ioc, square_stage(stage1, stage2), boost::asio::detached);
  boost::asio::co_spawn(ioc, sink(stage2), boost::asio::detached);

  ioc.run();
}

}  // namespace modules::actor_pipeline
