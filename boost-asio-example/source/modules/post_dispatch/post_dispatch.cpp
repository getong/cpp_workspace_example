#include "post_dispatch.hpp"

#include <iostream>

#include <boost/asio/defer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace modules::post_dispatch
{

void run()
{
  boost::asio::io_context ioc;

  // 在事件循环之外提交：此时没有线程在运行 ioc，
  // post 和 dispatch 都只能排队，等 ioc.run() 时才执行。
  boost::asio::post(ioc,
                    [&ioc]
                    {
                      std::cout << "1. outer post handler runs\n";

                      // dispatch：当前线程正运行着 ioc 的事件循环，
                      // 所以处理器被“就地”同步执行，相当于函数调用。
                      boost::asio::dispatch(
                          ioc, [] { std::cout << "2. inner dispatch runs inline, before outer handler returns\n"; });

                      // post：无条件排队，绝不在调用处执行，
                      // 因此一定在当前处理器返回之后才运行。
                      boost::asio::post(
                          ioc, [] { std::cout << "4. inner post runs after outer handler returned\n"; });

                      // defer 语义同 post（排队），但向执行器提示这是
                      // “当前工作的延续”，允许其做调度优化。
                      boost::asio::defer(
                          ioc, [] { std::cout << "5. inner defer runs last\n"; });

                      std::cout << "3. outer post handler returns\n";
                    });

  ioc.run();
}

}  // namespace modules::post_dispatch
