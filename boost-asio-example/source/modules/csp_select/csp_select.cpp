#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "csp_select.hpp"

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

// Go 的 select 让一个 goroutine 同时等多条 channel，谁先就绪走谁的
// 分支。这里用 awaitable_operators 的 || 近似：
//
//   Go                                这里
//   --------------------------------  --------------------------------
//   select { case v := <-a: ...       auto r = co_await (
//            case v := <-b: ... }         a->async_receive(use)
//                                          || b->async_receive(use));
//                                      按 r.index() 分支
//   case <-time.After(d):             || timer.async_wait(use)
//   <-done（关闭即广播）               done->close() 唤醒所有等待者
//
// 语义差异要点（诚实声明）：|| 在胜者完成后"取消"败者，并非 Go 那种
// 原子的多路等待。对"接收方先挂起等待、发送方后到"的无缓冲会合场景，
// 被取消的 receive 尚未匹配任何发送者，不会丢数据——本例都属此类。
// 若两条 channel 可能同时就绪（如有缓冲且已囤货），|| 可能取消一个
// 已完成的接收而丢值，真正的原子 select 需要更专门的机制。

namespace modules::csp_select
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::asio::experimental::awaitable_operators::operator&&;
using boost::asio::experimental::awaitable_operators::operator||;

using string_channel = boost::asio::experimental::channel<void(
    boost::system::error_code, std::string)>;
using string_channel_ptr = std::shared_ptr<string_channel>;

// 只传信号不传值的 channel：Go 的 chan struct{} / ctx.Done()。
using signal_channel =
    boost::asio::experimental::channel<void(boost::system::error_code)>;
using signal_channel_ptr = std::shared_ptr<signal_channel>;

constexpr auto tick_interval = 15ms;
constexpr auto tock_interval = 40ms;
constexpr int tick_count = 4;
constexpr int tock_count = 2;
constexpr auto select_timeout = 30ms;
constexpr auto close_done_after = 20ms;

// 周期性往 channel 里发消息的 goroutine。
auto producer(string_channel_ptr out,
              std::string label,
              std::chrono::milliseconds interval,
              int count) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer timer {executor};
  for (int seq = 1; seq <= count; ++seq) {
    timer.expires_after(interval);
    co_await timer.async_wait(use_awaitable);
    co_await out->async_send(
        {}, label + " " + std::to_string(seq), use_awaitable);
  }
}

// select 循环：两条 channel 谁先来处理谁，共处理 rounds 条。
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto select_loop(string_channel_ptr fast, string_channel_ptr slow, int rounds)
    -> awaitable<void>
{
  for (int round = 1; round <= rounds; ++round) {
    auto which = co_await (fast->async_receive(use_awaitable)
                           || slow->async_receive(use_awaitable));
    if (which.index() == 0) {
      std::cout << "select: <-fast  : " << std::get<0>(which) << '\n';
    } else {
      std::cout << "select: <-slow  : " << std::get<1>(which) << '\n';
    }
  }
}

// select + time.After：超时分支。
auto timeout_demo() -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  auto quiet = std::make_shared<string_channel>(executor, 0);  // 没人会写
  boost::asio::steady_timer deadline {executor, select_timeout};

  auto which = co_await (quiet->async_receive(use_awaitable)
                         || deadline.async_wait(use_awaitable));
  if (which.index() == 1) {
    std::cout << "select: timeout after 30ms (case <-time.After)\n";
  }
}

// done channel：等待者阻塞在 <-done 上。
auto done_waiter(int worker_id, signal_channel_ptr done) -> awaitable<void>
{
  std::cout << "worker " << worker_id << ": waiting on <-done\n";
  auto [err] = co_await done->async_receive(as_tuple(use_awaitable));
  std::cout << "worker " << worker_id
            << ": done closed, cleaning up and exiting\n";
}

// close(done)：一次 close 唤醒所有等待者——Go 取消广播的惯用法。
auto done_closer(signal_channel_ptr done) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer delay {executor, close_done_after};
  co_await delay.async_wait(use_awaitable);
  std::cout << "main: close(done) -- broadcast to every waiter\n";
  done->close();
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  boost::asio::co_spawn(
      ioc,
      []() -> awaitable<void>
      {
        auto executor = co_await boost::asio::this_coro::executor;

        std::cout << "-- select over two channels --\n";
        // 无缓冲 channel：select 的 receive 总是先挂起等着，
        // 发送方到场即会合，不存在"双双就绪"的丢值窗口。
        auto fast = std::make_shared<string_channel>(executor, 0);
        auto slow = std::make_shared<string_channel>(executor, 0);
        co_await (producer(fast, "tick", tick_interval, tick_count)
                  && producer(slow, "tock", tock_interval, tock_count)
                  && select_loop(fast, slow, tick_count + tock_count));

        std::cout << "-- select with timeout --\n";
        co_await timeout_demo();

        std::cout << "-- done-channel cancellation --\n";
        auto done = std::make_shared<signal_channel>(executor, 0);
        co_await (done_waiter(1, done) && done_waiter(2, done)
                  && done_closer(done));
      },
      boost::asio::detached);

  ioc.run();
}

}  // namespace modules::csp_select
