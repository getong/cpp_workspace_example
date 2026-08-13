#include "resolver.hpp"

#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

namespace modules::resolver
{

// 名字解析也是异步操作：async_resolve 在后台线程做 getaddrinfo，
// 完成后把结果（一组 endpoint）投递回事件循环。
// 解析 localhost 不需要外网，示例可离线运行。
void run()
{
  boost::asio::io_context ioc;

  boost::asio::ip::tcp::resolver resolver {ioc};
  resolver.async_resolve(
      "localhost", "8080",
      [](const boost::system::error_code& ec,
         const boost::asio::ip::tcp::resolver::results_type& results)
      {
        if (ec) {
          std::cout << "resolve failed: " << ec.message() << '\n';
          return;
        }
        std::cout << "localhost:8080 resolves to " << results.size()
                  << " endpoint(s):\n";
        for (const auto& entry : results) {
          const auto endpoint = entry.endpoint();
          std::cout << "  " << endpoint << " ("
                    << (endpoint.address().is_v6() ? "IPv6" : "IPv4")
                    << ")\n";
        }
      });

  ioc.run();
}

}  // namespace modules::resolver
