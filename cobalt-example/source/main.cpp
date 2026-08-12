// boost::cobalt TCP echo 服务器示例，覆盖三个要点：
//
//   1. 每个 TCP 连接一个协程：listen 协程每 accept 一条连接，
//      就启动一个 session promise，并交给 wait_group 管理。
//   2. session 内用 while 死循环持续解析 TCP 字节流：
//      TCP 是字节流协议，一次 read 可能拿到半行或多行，
//      所以用"连接级缓冲 + read_until 找 '\n' 帧边界"逐行拆包。
//      连接一直保持，客户端发多少行就 echo 多少行，直到客户端主动断开。
//   3. 客户端用 curl 的 telnet 模式建立长连接，服务端像 echo 一样
//      返回相同信息并追加时间戳：
//        curl telnet://127.0.0.1:8080
//        （连接建立后逐行输入，每行回显 "原文 [时间戳]"，Ctrl+C 退出）
//      也可以用管道一次发多行，观察同一连接被循环解析：
//        { printf 'one\n'; sleep 1; printf 'two\n'; } | curl -s
//        telnet://127.0.0.1:8080

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/result.hpp>
#include <boost/cobalt/wait_group.hpp>
#include <boost/cobalt/with.hpp>

#include "lib.hpp"

namespace cobalt = boost::cobalt;
namespace asio = boost::asio;
using asio::ip::tcp;

namespace
{

// 形如 "2026-08-12 21:03:05.123" 的本地时间戳
auto timestamp() -> std::string
{
  using clock = std::chrono::system_clock;
  auto const now = clock::now();
  auto const time = clock::to_time_t(now);
  auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count()
      % 1000;
  std::tm tm {};
  localtime_r(&time, &tm);
  char date[32];
  auto const len = std::strftime(date, sizeof date, "%Y-%m-%d %H:%M:%S", &tm);
  char out[48];
  std::snprintf(out,
                sizeof out,
                "%.*s.%03lld",
                static_cast<int>(len),
                date,
                static_cast<long long>(millis));
  return out;
}

// 去掉行尾的 \r\n（curl telnet 模式发送 CRLF）
auto strip_eol(std::string line) -> std::string
{
  while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
    line.pop_back();
  }
  return line;
}

// 要点 2：每个连接的处理协程。while 死循环 + 连接级缓冲不断从流中解析：
//   a) read_until 读到 '\n' 为止拿到完整一行（多读的字节留在 buf 里，
//      供下一轮循环直接解析，不会丢）
//   b) 回写 "原文 [时间戳]"，然后继续循环等下一行
// 连接永不主动关闭，直到对端断开 / 出错 / 收到取消信号
auto session(tcp::socket sock, unsigned conn_id) -> cobalt::promise<void>
{
  std::string buf;  // 连接级缓冲：残留的半行/多行都暂存在这里
  unsigned served = 0;

  while (true) {
    // a) 定位帧边界：一行以 '\n' 结束
    auto const line_size = co_await cobalt::as_result(asio::async_read_until(
        sock, asio::dynamic_buffer(buf), '\n', cobalt::use_op));
    if (!line_size) {  // 对端关闭 / 出错 / 被取消
      break;
    }
    auto const line = strip_eol(buf.substr(0, *line_size));
    buf.erase(0, *line_size);

    // 要点 3：echo 相同信息并追加时间戳
    std::string const reply = line + " [" + timestamp() + "]\n";
    auto const written = co_await cobalt::as_result(
        asio::async_write(sock, asio::buffer(reply), cobalt::use_op));
    if (!written) {
      break;
    }

    ++served;
    std::cout << "conn #" << conn_id << " line " << served << ": " << line
              << std::endl;
  }

  std::cout << "conn #" << conn_id << " closed after " << served << " line(s)"
            << std::endl;
}

// 要点 1：accept 循环。每来一条连接就启动一个 eager session
// promise，将管理句柄移入 wait_group 后立即等待下一条连接。
auto listen(tcp::endpoint endpoint, cobalt::wait_group& sessions)
    -> cobalt::promise<void>
{
  tcp::acceptor acceptor {co_await cobalt::this_coro::executor, endpoint};
  std::cout << "listening on " << acceptor.local_endpoint() << std::endl;

  for (unsigned conn_id = 1;; ++conn_id) {
    auto sock =
        co_await cobalt::as_result(acceptor.async_accept(cobalt::use_op));
    if (!sock) {  // Ctrl+C：cobalt::main 把信号转成取消
      break;
    }
    std::cout << "conn #" << conn_id << " accepted from "
              << sock->remote_endpoint() << std::endl;
    sessions.reap();
    sessions.push_back(session(std::move(*sock), conn_id));
  }
}

}  // namespace

auto co_main(int argc, char* argv[]) -> cobalt::main
{
  auto const lib = library {};
  std::cout << "Hello from " << lib.name << "!\n";

  auto const port =
      static_cast<unsigned short>(argc > 1 ? std::stoi(argv[1]) : 8080);
  std::cout << "try:  curl telnet://127.0.0.1:" << port << '\n'
            << "      (然后逐行输入，每行回显并追加时间戳，Ctrl+C 退出)\n";

  auto const endpoint = tcp::endpoint {tcp::v4(), port};
  co_await cobalt::with(cobalt::wait_group {asio::cancellation_type::all},
                        [endpoint](cobalt::wait_group& sessions)
                        { return listen(endpoint, sessions); });
  co_return 0;
}
