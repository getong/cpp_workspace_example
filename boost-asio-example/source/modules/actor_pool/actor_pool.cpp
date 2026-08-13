#include <chrono>
#include <iostream>
#include <memory>

#include "actor_pool.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// 工作者池（Erlang 的 poolboy / jobs 队列模式）：
//
//   dispatcher --> [jobs 队列] --> worker 1..3 --> [results 队列] --> collector
//
// 多个 worker actor 从同一条 jobs 队列领活（fan-out），结果汇聚到
// 一条 results 队列（fan-in）。空闲的 worker 挂起在 async_receive 上，
// 谁先空闲谁先领到下一个任务——负载均衡是队列语义自带的。
//
// 关停协议：dispatcher 发完所有任务后 close(jobs)；每个 worker 在
// receive 失败时退出。这相当于 Erlang 池子的 drain-then-stop。

namespace modules::actor_pool
{

namespace
{

using namespace std::chrono_literals;
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

struct job_result
{
  int worker_id;
  int job;
  int square;
};

using job_channel =
    boost::asio::experimental::channel<void(boost::system::error_code, int)>;
using result_channel = boost::asio::experimental::channel<void(
    boost::system::error_code, job_result)>;
using job_channel_ptr = std::shared_ptr<job_channel>;
using result_channel_ptr = std::shared_ptr<result_channel>;

constexpr int worker_count = 3;
constexpr int job_count = 8;
constexpr int queue_capacity = 4;
constexpr auto work_duration = 10ms;

// worker actor：循环领任务、干活、交结果；队列关闭即下班。
auto worker(int worker_id, job_channel_ptr jobs, result_channel_ptr results)
    -> awaitable<void>
{
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer busy {executor};

  for (;;) {
    auto [err, job] = co_await jobs->async_receive(as_tuple(use_awaitable));
    if (err) {
      std::cout << "worker " << worker_id << ": queue closed, exiting\n";
      co_return;
    }
    busy.expires_after(work_duration);  // 模拟计算耗时
    co_await busy.async_wait(use_awaitable);
    co_await results->async_send(
        {}, job_result {worker_id, job, job * job}, use_awaitable);
  }
}

// dispatcher actor：投递所有任务，然后关闭队列宣告"没有更多活了"。
auto dispatcher(job_channel_ptr jobs) -> awaitable<void>
{
  for (int job = 1; job <= job_count; ++job) {
    co_await jobs->async_send({}, job, use_awaitable);
  }
  jobs->close();
  std::cout << "dispatcher: all " << job_count << " jobs queued, closed\n";
}

// collector actor：收满预期数量的结果后汇总。
auto collector(result_channel_ptr results) -> awaitable<void>
{
  int sum = 0;
  for (int received = 0; received < job_count; ++received) {
    const auto result = co_await results->async_receive(use_awaitable);
    std::cout << "collector: worker " << result.worker_id << " computed "
              << result.job << "^2 = " << result.square << '\n';
    sum += result.square;
  }
  std::cout << "collector: sum of squares 1.." << job_count << " = " << sum
            << '\n';
}

}  // namespace

void run()
{
  boost::asio::io_context ioc;

  auto jobs = std::make_shared<job_channel>(ioc, queue_capacity);
  auto results = std::make_shared<result_channel>(ioc, job_count);

  for (int worker_id = 1; worker_id <= worker_count; ++worker_id) {
    boost::asio::co_spawn(
        ioc, worker(worker_id, jobs, results), boost::asio::detached);
  }
  boost::asio::co_spawn(ioc, dispatcher(jobs), boost::asio::detached);
  boost::asio::co_spawn(ioc, collector(results), boost::asio::detached);

  ioc.run();
}

}  // namespace modules::actor_pool
