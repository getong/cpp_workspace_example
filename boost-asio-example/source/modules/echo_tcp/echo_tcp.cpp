#include <array>
#include <iostream>
#include <string>
#include <string_view>

#include "echo_tcp.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/system_error.hpp>

namespace modules::echo_tcp
{

namespace
{

using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

// 服务端会话：把收到的每一段数据原样写回，直到对端关闭连接（EOF）。
auto echo_session(tcp::socket socket) -> awaitable<void>
{
  std::array<char, 1024> data {};
  try {
    for (;;) {
      // async_read_some：读到任意长度就返回，适合“来多少回多少”的 echo。
      const auto count = co_await socket.async_read_some(
          boost::asio::buffer(data), use_awaitable);
      // async_write：与 write_some 不同，保证写完整个缓冲区才完成。
      co_await boost::asio::async_write(
          socket, boost::asio::buffer(data.data(), count), use_awaitable);
    }
  } catch (const boost::system::system_error& error) {
    if (error.code() != boost::asio::error::eof) {
      throw;
    }
    std::cout << "server: client closed the connection\n";
  }
}

// 服务端：接受一个连接并为其服务（示例只处理一个客户端；
// 真实服务器会在循环里 accept 并对每个连接 co_spawn 一个会话）。
auto server(tcp::acceptor acceptor) -> awaitable<void>
{
  auto socket = co_await acceptor.async_accept(use_awaitable);
  std::cout << "server: accepted connection from " << socket.remote_endpoint()
            << '\n';
  co_await echo_session(std::move(socket));
}

// 客户端：连接、发送一条消息、读回同样长度的回显。
auto client(tcp::endpoint server_endpoint) -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  tcp::socket socket {executor};
  co_await socket.async_connect(server_endpoint, use_awaitable);

  constexpr std::string_view message = "hello, boost.asio!";
  co_await boost::asio::async_write(
      socket, boost::asio::buffer(message), use_awaitable);

  std::string reply(message.size(), '\0');
  // async_read：读满 reply 缓冲区才完成（正好是回显的长度）。
  co_await boost::asio::async_read(
      socket, boost::asio::buffer(reply), use_awaitable);
  std::cout << "client: received echo '" << reply << "'\n";
  // client 析构关闭 socket，服务端读到 EOF 后结束会话。
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  // 绑定回环地址的 0 号端口，由系统分配空闲端口，避免端口冲突。
  tcp::acceptor acceptor {
      ioc, tcp::endpoint {boost::asio::ip::address_v4::loopback(), 0}};
  const auto server_endpoint = acceptor.local_endpoint();
  std::cout << "server: listening on " << server_endpoint << '\n';

  // 服务端和客户端两个协程跑在同一个单线程事件循环上，互不阻塞。
  boost::asio::co_spawn(
      ioc, server(std::move(acceptor)), boost::asio::detached);
  boost::asio::co_spawn(ioc, client(server_endpoint), boost::asio::detached);

  ioc.run();
}

}  // namespace modules::echo_tcp
