#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "csp_fanin.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// Go 官方博客 "Go Concurrency Patterns" 里的经典组合，原文骨架：
//
//   jobs := make(chan int)            // fan-out：多个 worker 抢同一条
//   merged := make(chan string)       // fan-in ：结果汇入同一条
//   var wg sync.WaitGroup
//   for i := 0; i < 2; i++ { wg.Add(1); go worker(...) }
//   go func() { wg.Wait(); close(merged) }()   // 等全体收工再关闭出口
//   for r := range merged { fmt.Println(r) }
//
// 对应到协程：sync.WaitGroup + wg.Wait() 就是 awaitable 的 && ——
// "全部完成才继续"，而且是结构化的：不会有 goroutine 泄漏。
// close(merged) 的时机是 fan-in 的经典难题：必须等所有生产者都停笔，
// 由一个专门的"关门"协程在 join 之后执行。

namespace modules::csp_fanin
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::asio::experimental::awaitable_operators::operator&&;

using int_channel =
    boost::asio::experimental::channel<void(boost::system::error_code, int)>;
using string_channel = boost::asio::experimental::channel<void(
    boost::system::error_code, std::string)>;
using int_channel_ptr = std::shared_ptr<int_channel>;
using string_channel_ptr = std::shared_ptr<string_channel>;

constexpr int job_count = 6;
constexpr int worker_total = 2;
constexpr auto work_duration = 10ms;

// 派活：发完 close，宣告没有更多任务。
auto dispatcher(int_channel_ptr jobs) -> awaitable<void>
{
  for (int job = 1; job <= job_count; ++job) {
    co_await jobs->async_send({}, job, use_awaitable);
  }
  jobs->close();
}

// fan-out：两个 worker 从同一条 jobs channel 领活（谁空闲谁领）；
// fan-in：算好的结果都写进同一条 merged channel。
auto worker(int worker_id, int_channel_ptr jobs, string_channel_ptr merged)
    -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer busy {executor};

  for (;;) {
    auto [err, job] = co_await jobs->async_receive(as_tuple(use_awaitable));
    if (err) {
      co_return;  // jobs 关闭且取空：本 worker 收工
    }
    busy.expires_after(work_duration);
    co_await busy.async_wait(use_awaitable);
    co_await merged->async_send({},
                                "worker " + std::to_string(worker_id)
                                    + " finished job " + std::to_string(job),
                                use_awaitable);
  }
}

// 关门协程：go func() { wg.Wait(); close(merged) }() 的直译。
// && 就是 WaitGroup：两个 worker 都返回后才走到 close。
auto close_after_workers(int_channel_ptr jobs, string_channel_ptr merged)
    -> awaitable<void>
{
  co_await (worker(1, jobs, merged) && worker(2, jobs, merged));
  merged->close();
  std::cout << "closer: all " << worker_total
            << " workers done, close(merged)\n";
}

// 消费端：for r := range merged。
auto consumer(string_channel_ptr merged) -> awaitable<void>
{
  int received = 0;
  for (;;) {
    auto [err, line] = co_await merged->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    ++received;
    std::cout << "main: " << line << '\n';
  }
  std::cout << "main: merged closed after " << received << " results\n";
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  boost::asio::co_spawn(
      ioc,
      []() -> awaitable<void>
      {
        auto executor = co_await boost::asio::this_coro::executor;
        auto jobs = std::make_shared<int_channel>(executor, 0);
        auto merged = std::make_shared<string_channel>(executor, 0);

        co_await (dispatcher(jobs) && close_after_workers(jobs, merged)
                  && consumer(merged));
      },
      boost::asio::detached);

  ioc.run();
}

}  // namespace modules::csp_fanin
