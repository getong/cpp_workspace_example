#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

#include "actor_genserver.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// Erlang 的 gen_server 抽象了两种消息模式：
//
//   gen_server:cast(Pid, Msg)  -- 单向投递，不等结果
//   gen_server:call(Pid, Msg)  -- 请求-应答，调用方挂起等回执
//
// call 的实现手法照搬 Erlang：请求消息里带上"回信地址"。Erlang 里是
// 调用方的 Pid + 唯一引用，这里是一个容量为 1 的一次性 reply channel。
//
// 关键收益与 Erlang 相同：counter 状态只被 server actor 一个协程读写，
// 两个并发客户端各做 100 次自增也不需要任何锁——mailbox 就是同步点。

namespace modules::actor_genserver
{

namespace
{

using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

using reply_channel = boost::asio::experimental::channel<void(
    boost::system::error_code, std::int64_t)>;

// cast：只管发，不要结果。
struct increment
{
  std::int64_t by;
};

// call：带上回执通道。
struct get_value
{
  std::shared_ptr<reply_channel> reply;
};

struct stop_msg
{
};

using message = std::variant<increment, get_value, stop_msg>;
using mailbox = boost::asio::experimental::channel<void(
    boost::system::error_code, message)>;
using mailbox_ptr = std::shared_ptr<mailbox>;

constexpr int mailbox_capacity = 16;
constexpr int increments_per_client = 100;

// server actor：唯一持有 value 的"进程"，逐条处理邮箱消息。
auto counter_server(mailbox_ptr mbox) -> awaitable<void>
{
  std::int64_t value = 0;
  std::int64_t handled = 0;

  for (;;) {
    auto [err, msg] = co_await mbox->async_receive(as_tuple(use_awaitable));
    if (err) {
      co_return;
    }
    ++handled;
    if (const auto* inc = std::get_if<increment>(&msg)) {
      value += inc->by;
    } else if (const auto* get = std::get_if<get_value>(&msg)) {
      co_await get->reply->async_send({}, value, use_awaitable);
    } else if (std::holds_alternative<stop_msg>(msg)) {
      std::cout << "server: stopping after " << handled
                << " messages, final value = " << value << '\n';
      co_return;
    }
  }
}

// 客户端封装：gen_server:call 的等价物。
auto call_get(mailbox_ptr mbox) -> awaitable<std::int64_t>
{
  auto executor = co_await boost::asio::this_coro::executor;
  auto reply = std::make_shared<reply_channel>(executor, 1);
  co_await mbox->async_send({}, get_value {reply}, use_awaitable);
  co_return co_await reply->async_receive(use_awaitable);
}

// 客户端 actor：先 cast 一半自增，call 读一次中间值，再 cast 剩下一半。
auto client(std::string name, mailbox_ptr mbox, int total) -> awaitable<void>
{
  for (int i = 0; i < total / 2; ++i) {
    co_await mbox->async_send({}, increment {1}, use_awaitable);
  }
  const std::int64_t midway = co_await call_get(mbox);
  std::cout << "client " << name << ": midway call sees value " << midway
            << '\n';
  for (int i = total / 2; i < total; ++i) {
    co_await mbox->async_send({}, increment {1}, use_awaitable);
  }
}

// 编排协程：并发跑两个客户端，读最终值，然后让 server 停机。
auto orchestrator(mailbox_ptr mbox) -> awaitable<void>
{
  using boost::asio::experimental::awaitable_operators::operator&&;
  // 两个客户端并发地向同一个 server 发消息。
  co_await (client("A", mbox, increments_per_client)
            && client("B", mbox, increments_per_client));

  const std::int64_t final_value = co_await call_get(mbox);
  std::cout << "main: final call sees value " << final_value
            << " (expected 200, no locks anywhere)\n";
  co_await mbox->async_send({}, stop_msg {}, use_awaitable);
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  auto mbox = std::make_shared<mailbox>(ioc, mailbox_capacity);

  boost::asio::co_spawn(ioc, counter_server(mbox), boost::asio::detached);
  boost::asio::co_spawn(ioc, orchestrator(mbox), boost::asio::detached);

  ioc.run();
}

}  // namespace modules::actor_genserver
