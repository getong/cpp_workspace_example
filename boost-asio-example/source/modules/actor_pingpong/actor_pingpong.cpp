#include <iostream>
#include <memory>
#include <variant>

#include "actor_pingpong.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// Erlang 的经典入门例子，逐条对应：
//
//   Erlang                          这里
//   ------------------------------  --------------------------------
//   进程 (process)                   协程 (awaitable<void>)
//   邮箱 (mailbox)                   experimental::channel
//   Pid（进程句柄）                  shared_ptr<mailbox>
//   spawn(fun)                      co_spawn(executor, coro, detached)
//   Pid ! Msg                       co_await mbox->async_send(...)
//   receive ... end                 co_await mbox->async_receive(...)
//
// actor 之间不共享状态，只通过消息交流；每个 actor 顺序处理
// 自己邮箱里的消息——这正是 Erlang 并发模型的两条根本规则。

namespace modules::actor_pingpong
{

namespace
{

using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

// 消息用 variant 建模，等价于 Erlang 里不同形状的元组 {ping, N} / stop。
struct ping_msg
{
  int seq;
};

struct pong_msg
{
  int seq;
};

struct stop_msg
{
};

using message = std::variant<ping_msg, pong_msg, stop_msg>;

// mailbox：有界异步消息队列。发送方在队列满时挂起（背压），
// 接收方在队列空时挂起——两端都不阻塞线程。
using mailbox = boost::asio::experimental::channel<void(
    boost::system::error_code, message)>;
// mailbox_ptr 扮演 Erlang 的 Pid：可复制、可存进消息里转发的句柄。
using mailbox_ptr = std::shared_ptr<mailbox>;

constexpr int mailbox_capacity = 8;

// ping actor：主动方。发 ping、等 pong，往复 rounds 次后通知对方停止。
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto ping_actor(mailbox_ptr self, mailbox_ptr peer, int rounds)
    -> awaitable<void>
{
  for (int seq = 1; seq <= rounds; ++seq) {
    co_await peer->async_send({}, ping_msg {seq}, use_awaitable);
    // receive：只关心 pong，其他消息在本例中不会出现。
    auto [err, msg] = co_await self->async_receive(as_tuple(use_awaitable));
    if (err) {
      co_return;
    }
    if (const auto* pong = std::get_if<pong_msg>(&msg)) {
      std::cout << "ping: got pong " << pong->seq << '\n';
    }
  }
  co_await peer->async_send({}, stop_msg {}, use_awaitable);
  std::cout << "ping: done\n";
}

// pong actor：被动方。收到 ping 回 pong，收到 stop 退出。
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto pong_actor(mailbox_ptr self, mailbox_ptr peer) -> awaitable<void>
{
  for (;;) {
    auto [err, msg] = co_await self->async_receive(as_tuple(use_awaitable));
    if (err) {
      co_return;
    }
    if (const auto* ping = std::get_if<ping_msg>(&msg)) {
      std::cout << "pong: got ping " << ping->seq << '\n';
      co_await peer->async_send({}, pong_msg {ping->seq}, use_awaitable);
    } else if (std::holds_alternative<stop_msg>(msg)) {
      std::cout << "pong: bye\n";
      co_return;
    }
  }
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  auto ping_box = std::make_shared<mailbox>(ioc, mailbox_capacity);
  auto pong_box = std::make_shared<mailbox>(ioc, mailbox_capacity);

  // spawn 两个 actor。它们跑在同一个单线程事件循环上，
  // 却像两个独立进程一样各自推进。
  boost::asio::co_spawn(
      ioc, ping_actor(ping_box, pong_box, 3), boost::asio::detached);
  boost::asio::co_spawn(
      ioc, pong_actor(pong_box, ping_box), boost::asio::detached);

  ioc.run();
}

}  // namespace modules::actor_pingpong
