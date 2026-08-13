#include <chrono>
#include <iostream>
#include <string>

#include "coroutine.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace modules::coroutine
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

// awaitable<T> 是 asio 的协程返回类型；co_await 一个异步操作时
// 协程挂起，操作完成后由事件循环恢复——异步代码写成了同步的样子。
auto delayed_message(std::string text, std::chrono::milliseconds delay)
    -> awaitable<std::string>
{
  // this_coro::executor 取得当前协程绑定的执行器。
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer timer {executor, delay};
  co_await timer.async_wait(use_awaitable);
  co_return text;
}

auto sequential_demo() -> awaitable<void>
{
  // 顺序组合：两次 co_await 依次执行，总耗时约 10ms + 10ms。
  auto first = co_await delayed_message("first", 10ms);
  auto second = co_await delayed_message("second", 10ms);
  std::cout << "sequential: " << first << " then " << second << '\n';
}

auto race_demo() -> awaitable<void>
{
  // awaitable_operators 的 || 让两个操作赛跑：
  // 先完成者胜出，另一个被自动取消——这是实现超时的惯用写法。
  using namespace boost::asio::experimental::awaitable_operators;

  auto winner = co_await (delayed_message("fast task", 10ms)
                          || delayed_message("slow task", 5s));
  if (winner.index() == 0) {
    std::cout << "race: '" << std::get<0>(winner)
              << "' won, slow task was cancelled\n";
  }

  // && 则等两个都完成，结果打包成 tuple，总耗时取决于较慢者。
  auto [left, right] = co_await (delayed_message("left", 10ms)
                                 && delayed_message("right", 20ms));
  std::cout << "join: got '" << left << "' and '" << right << "'\n";
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  // co_spawn 把协程作为顶层任务挂到执行器上；
  // detached 表示“发射后不管”，不关心返回值和异常传播。
  boost::asio::co_spawn(
      ioc,
      []() -> awaitable<void>
      {
        co_await sequential_demo();
        co_await race_demo();
      },
      boost::asio::detached);

  ioc.run();
}

}  // namespace modules::coroutine
