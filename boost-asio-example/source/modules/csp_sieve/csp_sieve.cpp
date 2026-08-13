#include <iostream>
#include <memory>

#include "csp_sieve.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// 并发素数筛：CSP 论文时代流传下来、被 Go 官网选为招牌演示的例子。
// 结构是一条动态生长的进程链：
//
//   generate(2,3,4,...) --> filter(2) --> filter(3) --> filter(5) --> main
//
// generate 产出自然数流；main 每从链尾读到一个数，它必然是素数
// （能被已知素数整除的都被沿途滤掉了），于是为它 spawn 一个新的
// filter 接到链尾。素数越筛越多，进程链越长——**进程和 channel 都是
// 运行期动态创建的**，这是 CSP "进程即值" 的直观展示。
//
// Go 原版（golang.org 首页曾经的演示）：
//
//   func filter(in, out chan int, prime int) {
//     for { i := <-in; if i%prime != 0 { out <- i } }
//   }
//
// Go 版靠进程退出时 goroutine 泄漏兜底；这里补全了关停协议：
// generate 发完 close，每级 filter 在上游关闭后关闭下游，
// EOF 沿链级联，所有协程干净退出。

namespace modules::csp_sieve
{

namespace
{

using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

using int_channel =
    boost::asio::experimental::channel<void(boost::system::error_code, int)>;
using int_channel_ptr = std::shared_ptr<int_channel>;

constexpr int sieve_limit = 30;

// 源头：把 2..limit 灌进 channel。
auto generate(int_channel_ptr out) -> awaitable<void>
{
  for (int value = 2; value <= sieve_limit; ++value) {
    co_await out->async_send({}, value, use_awaitable);
  }
  out->close();
}

// 一级筛子：滤掉 prime 的倍数，其余转发给下游。
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto filter(int prime, int_channel_ptr input, int_channel_ptr out)
    -> awaitable<void>
{
  for (;;) {
    auto [err, value] = co_await input->async_receive(as_tuple(use_awaitable));
    if (err) {
      out->close();  // 上游断流，关停向下游级联
      co_return;
    }
    if (value % prime != 0) {
      co_await out->async_send({}, value, use_awaitable);
    }
  }
}

// 链尾：每读到一个素数，就为它孵化一级新筛子接上去。
auto sieve() -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;

  auto chain_end = std::make_shared<int_channel>(executor, 0);
  boost::asio::co_spawn(executor, generate(chain_end), boost::asio::detached);

  int prime_count = 0;
  for (;;) {
    auto [err, prime] =
        co_await chain_end->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    ++prime_count;
    std::cout << "prime: " << prime << '\n';

    auto next = std::make_shared<int_channel>(executor, 0);
    boost::asio::co_spawn(
        executor, filter(prime, chain_end, next), boost::asio::detached);
    chain_end = next;  // main 之后从新筛子的出口读
  }
  std::cout << "sieve: " << prime_count << " primes up to " << sieve_limit
            << ", chain of " << prime_count << " filter processes drained\n";
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;
  boost::asio::co_spawn(ioc, sieve(), boost::asio::detached);
  ioc.run();
}

}  // namespace modules::csp_sieve
