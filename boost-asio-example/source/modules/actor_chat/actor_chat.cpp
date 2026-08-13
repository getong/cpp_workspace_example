#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "actor_chat.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/system_error.hpp>

// Erlang 网络服务的标志性架构：**每个连接一个进程**。这里完整复刻：
//
//   - accept 循环对每个新连接 co_spawn 一个 session actor
//     （Erlang: 每个 accept 后 spawn 一个连接进程）；
//   - 一个 room actor 独占成员表，负责入会/退会/广播
//     （Erlang: 聊天室注册进程），成员表没有锁，因为只有它能碰；
//   - session 与 room 之间只通过消息往来：socket 归 session 管，
//     成员关系归 room 管，职责清晰。
//
// 消息流：socket 读到一行 -> chat 消息发给 room -> room 广播到
// 各成员 inbox -> 各 session 的 writer 把 inbox 写回自己的 socket。
//
// 本例在同一进程内起 2 个回环客户端（alice/bob）演出一段对话，
// 每一步都等到上一步的字节真正到达后才继续，输出是确定的。

namespace modules::actor_chat
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

// 每个 session 的收件箱：room 广播文本行到这里。
using text_channel = boost::asio::experimental::channel<void(
    boost::system::error_code, std::string)>;
using text_channel_ptr = std::shared_ptr<text_channel>;

// room 的邮箱消息：入会（带上自己的收件箱，即"Pid"）、退会、发言。
struct join_msg
{
  int session_id;
  text_channel_ptr inbox;
};

struct leave_msg
{
  int session_id;
};

struct chat_msg
{
  int session_id;
  std::string text;
};

using room_message = std::variant<join_msg, leave_msg, chat_msg>;
using room_channel = boost::asio::experimental::channel<void(
    boost::system::error_code, room_message)>;
using room_channel_ptr = std::shared_ptr<room_channel>;

constexpr int mailbox_capacity = 16;
constexpr int session_count = 2;

// room actor：唯一持有成员表的"进程"。所有成员操作都在这一个协程里
// 顺序执行，天然无竞争。服务完预期数量的会话后自然退出。
auto room_actor(room_channel_ptr mbox) -> awaitable<void>
{
  std::map<int, text_channel_ptr> members;
  int total_joined = 0;

  for (;;) {
    auto [err, msg] = co_await mbox->async_receive(as_tuple(use_awaitable));
    if (err) {
      co_return;
    }

    if (const auto* join = std::get_if<join_msg>(&msg)) {
      ++total_joined;
      co_await join->inbox->async_send(
          {},
          "welcome, you are session " + std::to_string(join->session_id),
          use_awaitable);
      for (const auto& [member_id, inbox] : members) {
        co_await inbox->async_send(
            {},
            "* session " + std::to_string(join->session_id) + " joined",
            use_awaitable);
      }
      members[join->session_id] = join->inbox;
      std::cout << "room: session " << join->session_id << " joined ("
                << members.size() << " online)\n";
    } else if (const auto* leave = std::get_if<leave_msg>(&msg)) {
      // 关闭离开者的收件箱，它的 writer 协程随之退出。
      members[leave->session_id]->close();
      members.erase(leave->session_id);
      for (const auto& [member_id, inbox] : members) {
        co_await inbox->async_send(
            {},
            "* session " + std::to_string(leave->session_id) + " left",
            use_awaitable);
      }
      std::cout << "room: session " << leave->session_id << " left ("
                << members.size() << " online)\n";
    } else if (const auto* chat = std::get_if<chat_msg>(&msg)) {
      // 广播给除发言者外的所有成员。
      for (const auto& [member_id, inbox] : members) {
        if (member_id != chat->session_id) {
          co_await inbox->async_send(
              {},
              "session " + std::to_string(chat->session_id) + ": " + chat->text,
              use_awaitable);
        }
      }
    }

    if (total_joined == session_count && members.empty()) {
      std::cout << "room: everyone left, closing\n";
      co_return;
    }
  }
}

// 读一行（不含换行符）。async_read_until 可能多读，余量留在 buf 里。
auto read_line(tcp::socket& socket, std::string& buf) -> awaitable<std::string>
{
  const auto len = co_await boost::asio::async_read_until(
      socket, boost::asio::dynamic_buffer(buf), '\n', use_awaitable);
  std::string line = buf.substr(0, len - 1);
  buf.erase(0, len);
  co_return line;
}

// session 的读半边：socket 的行 -> room；对端关闭时向 room 报退会。
// 引用参数由父协程 session_actor 持有并保证存活（结构化并发）。
// NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
auto session_reader(tcp::socket& socket, int session_id, room_channel_ptr room)
    -> awaitable<void>
{
  std::string buf;
  try {
    for (;;) {
      auto line = co_await read_line(socket, buf);
      co_await room->async_send(
          {}, chat_msg {session_id, std::move(line)}, use_awaitable);
    }
  } catch (const boost::system::system_error& error) {
    if (error.code() != boost::asio::error::eof) {
      throw;
    }
  }
  co_await room->async_send({}, leave_msg {session_id}, use_awaitable);
}

// session 的写半边：inbox 的广播 -> socket；inbox 被 room 关闭即结束。
// NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
auto session_writer(tcp::socket& socket, text_channel_ptr inbox)
    -> awaitable<void>
{
  for (;;) {
    auto [err, text] = co_await inbox->async_receive(as_tuple(use_awaitable));
    if (err) {
      co_return;
    }
    text += '\n';
    co_await boost::asio::async_write(
        socket, boost::asio::buffer(text), use_awaitable);
  }
}

// session actor：一个连接一个。先入会，然后读写两个子协程并行伺候
// 这条连接，双双结束后 session 整体退出。
auto session_actor(tcp::socket socket, int session_id, room_channel_ptr room)
    -> awaitable<void>
{
  using boost::asio::experimental::awaitable_operators::operator&&;

  auto inbox =
      std::make_shared<text_channel>(socket.get_executor(), mailbox_capacity);
  co_await room->async_send({}, join_msg {session_id, inbox}, use_awaitable);

  co_await (session_reader(socket, session_id, room)
            && session_writer(socket, inbox));
}

// accept 循环：Erlang 服务器的骨架——每来一个连接 spawn 一个进程。
auto server(tcp::acceptor acceptor, room_channel_ptr room) -> awaitable<void>
{
  for (int session_id = 1; session_id <= session_count; ++session_id) {
    auto socket = co_await acceptor.async_accept(use_awaitable);
    boost::asio::co_spawn(acceptor.get_executor(),
                          session_actor(std::move(socket), session_id, room),
                          boost::asio::detached);
  }
}

// ---- 演出剧本：两个回环客户端 ----

auto alice(tcp::endpoint server_endpoint) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  tcp::socket socket {executor};
  co_await socket.async_connect(server_endpoint, use_awaitable);

  // 注意：不能写成 `std::cout << "[alice] " << co_await read_line(...)`。
  // 那样前缀先输出、表达式中途挂起，其他协程的输出会插进来。
  std::string buf;
  auto line = co_await read_line(socket, buf);
  std::cout << "[alice] " << line << '\n';
  line = co_await read_line(socket, buf);
  std::cout << "[alice] " << line << '\n';

  const std::string greeting = "hi bob\n";
  co_await boost::asio::async_write(
      socket, boost::asio::buffer(greeting), use_awaitable);

  line = co_await read_line(socket, buf);
  std::cout << "[alice] " << line << '\n';
  // 协程结束，socket 析构关闭连接 -> session 1 的 reader 读到 EOF。
}

auto bob(tcp::endpoint server_endpoint) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  // 稍等片刻再连接，保证 alice 拿到 session 1、bob 拿到 session 2。
  boost::asio::steady_timer delay {executor, 20ms};
  co_await delay.async_wait(use_awaitable);

  tcp::socket socket {executor};
  co_await socket.async_connect(server_endpoint, use_awaitable);

  std::string buf;
  auto line = co_await read_line(socket, buf);
  std::cout << "[bob]   " << line << '\n';
  line = co_await read_line(socket, buf);
  std::cout << "[bob]   " << line << '\n';

  const std::string reply = "hi alice\n";
  co_await boost::asio::async_write(
      socket, boost::asio::buffer(reply), use_awaitable);

  line = co_await read_line(socket, buf);
  std::cout << "[bob]   " << line << '\n';
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  auto room = std::make_shared<room_channel>(ioc, mailbox_capacity);

  tcp::acceptor acceptor {
      ioc, tcp::endpoint {boost::asio::ip::address_v4::loopback(), 0}};
  const auto server_endpoint = acceptor.local_endpoint();

  boost::asio::co_spawn(ioc, room_actor(room), boost::asio::detached);
  boost::asio::co_spawn(
      ioc, server(std::move(acceptor), room), boost::asio::detached);
  boost::asio::co_spawn(ioc, alice(server_endpoint), boost::asio::detached);
  boost::asio::co_spawn(ioc, bob(server_endpoint), boost::asio::detached);

  ioc.run();
}

}  // namespace modules::actor_chat
