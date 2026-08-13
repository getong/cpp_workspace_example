#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

#include "udp_echo.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>

namespace modules::udp_echo
{

namespace
{

using boost::asio::ip::udp;

}  // namespace

// 与 echo_tcp 的协程风格对照：这里用传统回调（completion handler）风格。
// UDP 无连接，收发都要带上对端地址（recvfrom/sendto 语义）。
void run()
{
  boost::asio::io_context ioc;

  // 服务端与客户端 socket 都绑定回环地址的系统分配端口。
  udp::socket server_socket {
      ioc, udp::endpoint {boost::asio::ip::address_v4::loopback(), 0}};
  udp::socket client_socket {
      ioc, udp::endpoint {boost::asio::ip::address_v4::loopback(), 0}};
  const auto server_endpoint = server_socket.local_endpoint();

  // ---- 服务端：收到一个数据报后原样发回 ----
  std::array<char, 1024> server_buffer {};
  udp::endpoint sender;
  server_socket.async_receive_from(
      boost::asio::buffer(server_buffer),
      sender,
      [&](const boost::system::error_code& ec, std::size_t received)
      {
        if (ec) {
          return;
        }
        std::cout << "server: got " << received << " bytes from " << sender
                  << '\n';
        // 回调风格的链式调用：上一步完成后在回调里发起下一步。
        server_socket.async_send_to(
            boost::asio::buffer(server_buffer.data(), received),
            sender,
            [](const boost::system::error_code&, std::size_t) {});
      });

  // ---- 客户端：发送一条消息，然后等待回显 ----
  constexpr std::string_view message = "ping over udp";
  client_socket.async_send_to(
      boost::asio::buffer(message),
      server_endpoint,
      [](const boost::system::error_code&, std::size_t) {});

  std::array<char, 1024> client_buffer {};
  udp::endpoint replier;
  client_socket.async_receive_from(
      boost::asio::buffer(client_buffer),
      replier,
      [&](const boost::system::error_code& ec, std::size_t received)
      {
        if (ec) {
          return;
        }
        std::cout << "client: received echo '"
                  << std::string_view {client_buffer.data(), received} << "'\n";
      });

  // 所有回调执行完、没有挂起的操作后 run() 返回。
  ioc.run();
}

}  // namespace modules::udp_echo
