#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

#include "actor_supervisor.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

// Erlang 的核心哲学是 "let it crash"：worker 不写防御式代码，出错就
// 直接崩溃退出；由 supervisor 监视它并按策略重启，把"错误处理"和
// "业务逻辑"彻底分开。
//
//   Erlang                              这里
//   ----------------------------------  ------------------------------
//   spawn_link / monitor                co_spawn(..., use_awaitable)
//   进程崩溃 (exit signal)               协程抛出异常
//   收到 {'DOWN', ..., Reason}          co_await 处捕获该异常
//   one_for_one 重启策略                 catch 后循环重启同一个 child
//   max_restarts 重启上限                attempt 计数
//
// co_spawn 用 use_awaitable 作完成令牌时，child 的异常会在 co_await
// 处重新抛出——这正是 monitor 收到 DOWN 消息的等价物。
//
// 语言细节：catch 块内不允许 co_await，所以 catch 里只记录崩溃原因，
// 重启延迟放在 try/catch 之外等待。

namespace modules::actor_supervisor
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

constexpr auto heartbeat_interval = 10ms;
constexpr auto restart_backoff = 10ms;

// 一个不稳定的 worker：前两次运行到第 2 个心跳就崩溃，第三次才能跑完。
// 注意它完全没有 try/catch——let it crash。
auto flaky_worker(int attempt) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer timer {executor};

  for (int tick = 1; tick <= 3; ++tick) {
    timer.expires_after(heartbeat_interval);
    co_await timer.async_wait(use_awaitable);
    std::cout << "  worker(attempt " << attempt << "): tick " << tick << '\n';

    if (attempt < 3 && tick == 2) {
      throw std::runtime_error {"simulated crash on tick 2"};
    }
  }
  std::cout << "  worker(attempt " << attempt << "): finished normally\n";
}

// supervisor actor：one_for_one 策略，最多重启 max_restarts 次。
auto supervisor() -> awaitable<void>
{
  constexpr int max_restarts = 5;
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer restart_delay {executor};

  for (int attempt = 1;; ++attempt) {
    std::cout << "supervisor: starting worker (attempt " << attempt << ")\n";

    bool crashed = false;
    std::string reason;
    try {
      // 等价于 spawn_link：child 的异常传播到这里。
      co_await boost::asio::co_spawn(
          executor, flaky_worker(attempt), use_awaitable);
    } catch (const std::exception& error) {
      crashed = true;
      reason = error.what();
    }

    if (!crashed) {
      std::cout << "supervisor: worker exited normally, all done\n";
      co_return;
    }

    std::cout << "supervisor: worker DOWN, reason: " << reason << '\n';
    if (attempt >= max_restarts) {
      std::cout << "supervisor: restart limit reached, giving up\n";
      co_return;
    }
    // 真实系统里通常做指数退避，这里固定小延迟示意。
    restart_delay.expires_after(restart_backoff);
    co_await restart_delay.async_wait(use_awaitable);
  }
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;
  boost::asio::co_spawn(ioc, supervisor(), boost::asio::detached);
  ioc.run();
}

}  // namespace modules::actor_supervisor
