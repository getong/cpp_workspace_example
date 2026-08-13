#include <chrono>
#include <functional>
#include <iostream>

#include "timer.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

namespace modules::timer
{

void run()
{
  using namespace std::chrono_literals;

  boost::asio::io_context ioc;

  // 1) 同步等待：wait() 阻塞当前线程直到到期，不需要事件循环。
  boost::asio::steady_timer sync_timer {ioc, 20ms};
  sync_timer.wait();
  std::cout << "sync: timer expired after 20ms\n";

  // 2) 异步等待：async_wait 只登记回调立即返回，
  //    回调由 ioc.run() 所在线程在到期后调用。
  boost::asio::steady_timer async_timer {ioc, 30ms};
  async_timer.async_wait(
      [](const boost::system::error_code& ec)
      {
        if (!ec) {
          std::cout << "async: timer expired after 30ms\n";
        }
      });

  // 3) 周期定时：回调里用 expires_at(旧到期点 + 周期) 重新武装自己，
  //    相比 expires_after 不会累积回调本身的耗时误差。
  boost::asio::steady_timer periodic {ioc, 10ms};
  int remaining_ticks = 3;
  std::function<void(const boost::system::error_code&)> on_tick =
      [&](const boost::system::error_code& ec)
  {
    if (ec) {
      return;
    }
    std::cout << "periodic: tick, " << --remaining_ticks << " remaining\n";
    if (remaining_ticks > 0) {
      periodic.expires_at(periodic.expiry() + 10ms);
      periodic.async_wait(on_tick);
    }
  };
  periodic.async_wait(on_tick);

  // 4) 取消：cancel() 让未到期的等待立刻完成，
  //    回调收到 operation_aborted 错误码，可据此区分“到期”与“被取消”。
  boost::asio::steady_timer cancelled {ioc, 10s};
  cancelled.async_wait(
      [](const boost::system::error_code& ec)
      {
        if (ec == boost::asio::error::operation_aborted) {
          std::cout << "cancelled: async_wait aborted by cancel()\n";
        }
      });
  cancelled.cancel();

  // run() 运行事件循环直到没有未完成的异步操作，然后返回。
  ioc.run();
}

}  // namespace modules::timer
