// boost::cobalt 综合示例：cobalt::spawn 返回"协程句柄" + CSP channel 通信
//
// 共 13 个示例，分三部分：
//
// 第一部分：cobalt::spawn —— 从普通（非协程）代码启动 cobalt::task，
// 通过不同的 completion token 拿到不同形态的"协程句柄"：
//   1. spawn + 回调：完成时回调 void(std::exception_ptr, T)
//   2. spawn + asio::use_future：返回 std::future<T>，可跨函数传递/阻塞取结果
//   3. spawn + asio::bind_cancellation_slot：拿到可随时取消协程的句柄
//   4. spawn 多个 detached 任务：观察单线程事件循环里的交错执行
//
// 第二部分：协程内部的急切句柄：
//   5. cobalt::promise 创建即运行，本身就是句柄：可先放着、稍后 co_await
//      join，也可以 .cancel() 取消
//
// 第三部分：CSP 风格 channel 通信（cobalt::channel 类似 Go 的 chan）：
//   6.  SPSC：单生产者/单消费者，带缓冲通道
//   7.  rendezvous duo：容量 0 通道的 ping-pong，写读必须"握手"
//   8.  MPSC：多生产者单消费者，gather 等待所有生产者后关闭通道
//   9.  worker pool：jobs 通道扇出给 N 个 worker，results 通道扇入汇总
//   10. pipeline：多级流水线 gen -> square -> format -> sink
//   11. select：cobalt::race 同时等待两个通道，谁先就绪处理谁（类 Go select）
//   12. backpressure：缓冲写满后写端挂起，被慢速读端反压
//   13. 综合：spawn(use_future) 驱动一个内部用 channel 求和的 task
//
// 运行：./build/dev/spawn-channel

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/gather.hpp>
#include <boost/cobalt/join.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/race.hpp>
#include <boost/cobalt/result.hpp>
#include <boost/cobalt/spawn.hpp>
#include <boost/cobalt/task.hpp>
#include <boost/cobalt/this_coro.hpp>
#include <boost/variant2/variant.hpp>

namespace cobalt = boost::cobalt;
namespace asio = boost::asio;
using namespace std::chrono_literals;

namespace
{

// ---------- 公共小工具 ----------

auto section(std::string_view title) -> void
{
  std::cout << "\n==== " << title << " ====\n";
}

// 协程内睡眠：cobalt 没有内置 sleep，用 steady_timer + use_op 实现
auto sleep_for(std::chrono::milliseconds dur) -> cobalt::promise<void>
{
  asio::steady_timer tim {co_await cobalt::this_coro::executor, dur};
  co_await tim.async_wait(cobalt::use_op);
}

// 周期打点。promise 是急切启动的：构造出来的对象本身就是运行中协程的句柄。
// 被取消时 async_wait 以错误完成（as_result 拿到 error），协程自己收尾退出。
auto tick_promise(std::string name,
                  int count,
                  std::chrono::milliseconds interval) -> cobalt::promise<void>
{
  asio::steady_timer tim {co_await cobalt::this_coro::executor};
  for (int i = 1; i <= count; ++i) {
    tim.expires_after(interval);
    auto const ok = co_await cobalt::as_result(tim.async_wait(cobalt::use_op));
    if (!ok) {
      std::cout << "    " << name << " 在第 " << i << " 次 tick 前被取消\n";
      co_return;
    }
    std::cout << "    " << name << " tick " << i << '\n';
  }
  std::cout << "    " << name << " 正常结束\n";
}

// 同样的打点逻辑，包成 task 供 cobalt::spawn 使用（spawn 只接受 task）
auto tick_task(std::string name, int count, std::chrono::milliseconds interval)
    -> cobalt::task<void>
{
  co_await tick_promise(std::move(name), count, interval);
}

// 模拟一段异步计算，供 spawn 的各种 completion token 示例复用
auto compute_square(int x) -> cobalt::task<int>
{
  co_await sleep_for(10ms);
  co_return x* x;
}

// 把 task<void> 交给 spawn 并跑完事件循环；异常直接抛出（示例代码故意炸）
auto run_blocking(asio::io_context& ctx, cobalt::task<void> task) -> void
{
  cobalt::spawn(ctx,
                std::move(task),
                [](std::exception_ptr ep)
                {
                  if (ep) {
                    std::rethrow_exception(ep);
                  }
                });
  ctx.run();
  ctx.restart();
}

// ---------- 示例 5：promise 急切句柄 ----------

auto promise_handle_demo() -> cobalt::task<void>
{
  // 构造即启动：拿到句柄时协程已经在跑（跑到第一个挂起点让出）
  auto handle = tick_promise("eager-worker", 3, 10ms);
  std::cout << "  句柄已拿到，worker 已在后台运行，主流程先干点别的\n";
  co_await sleep_for(15ms);
  std::cout << "  主流程忙完，co_await 句柄等 worker 结束\n";
  co_await handle;  // join

  // 句柄还能取消：cancel() 会通过协程的 cancellation slot 打断挂起中的操作
  auto victim = tick_promise("victim", 1000, 20ms);
  co_await sleep_for(70ms);
  std::cout << "  对 victim 调用 cancel()\n";
  victim.cancel();
  co_await victim;  // 取消后协程正常收尾，join 立即返回
}

// ---------- 示例 6：SPSC 缓冲通道 ----------

auto spsc_producer(cobalt::channel<int>& ch, int count) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await ch.write(i);
    std::cout << "  [生产者] 写入 " << i << '\n';
  }
  ch.close();  // 关闭通道 = 广播"不再有数据"，读端会拿到 error
  std::cout << "  [生产者] 写完并关闭通道\n";
}

auto spsc_consumer(cobalt::channel<int>& ch) -> cobalt::promise<void>
{
  for (;;) {
    // as_result 把"通道已关闭"从异常变成 error，方便用返回值收敛循环
    auto const val = co_await cobalt::as_result(ch.read());
    if (!val) {
      break;
    }
    std::cout << "  [消费者] 读到 " << *val << '\n';
  }
  std::cout << "  [消费者] 通道关闭，退出\n";
}

auto spsc_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<int> ch {4, exec};  // 缓冲 4：写满前生产者不阻塞
  co_await cobalt::join(spsc_producer(ch, 8), spsc_consumer(ch));
}

// ---------- 示例 7：rendezvous ping-pong duo ----------

auto ping(cobalt::channel<std::string>& out,
          cobalt::channel<std::string>& in,
          int rounds) -> cobalt::promise<void>
{
  for (int i = 1; i <= rounds; ++i) {
    co_await out.write("ping " + std::to_string(i));  // 容量 0：等对方来读
    auto const reply = co_await in.read();
    std::cout << "  [ping] 收到回应: " << reply << '\n';
  }
  out.close();
}

auto pong(cobalt::channel<std::string>& in, cobalt::channel<std::string>& out)
    -> cobalt::promise<void>
{
  for (;;) {
    auto const msg = co_await cobalt::as_result(in.read());
    if (!msg) {
      break;
    }
    std::cout << "  [pong] 收到: " << *msg << '\n';
    co_await out.write("pong 回应 [" + *msg + "]");
  }
  out.close();
}

auto pingpong_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  // 容量 0 的 rendezvous 通道：write 一定挂起到有 read 配对，反之亦然，
  // 两个协程被通道逼着严格交替执行——最纯粹的 CSP 同步语义
  cobalt::channel<std::string> ping_to_pong {0, exec};
  cobalt::channel<std::string> pong_to_ping {0, exec};
  co_await cobalt::join(ping(ping_to_pong, pong_to_ping, 3),
                        pong(ping_to_pong, pong_to_ping));
}

// ---------- 示例 8：MPSC 多生产者单消费者 ----------

auto mpsc_producer(cobalt::channel<std::string>& ch,
                   std::string name,
                   int count,
                   std::chrono::milliseconds pace) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await sleep_for(pace);
    co_await ch.write(name + " 的第 " + std::to_string(i) + " 条消息");
  }
}

auto mpsc_consumer(cobalt::channel<std::string>& ch) -> cobalt::promise<void>
{
  int total = 0;
  for (;;) {
    auto const msg = co_await cobalt::as_result(ch.read());
    if (!msg) {
      break;
    }
    ++total;
    std::cout << "  [消费者] " << *msg << '\n';
  }
  std::cout << "  [消费者] 共收到 " << total << " 条，退出\n";
}

auto mpsc_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<std::string> ch {2, exec};

  auto consumer = mpsc_consumer(ch);  // 先启动消费者（promise 急切执行）
  // gather 并发运行三个生产者并等它们全部结束——
  // 这正是"谁来关通道"问题的答案：所有写端都退出后统一 close
  co_await cobalt::gather(mpsc_producer(ch, "P1", 3, 8ms),
                          mpsc_producer(ch, "P2", 3, 13ms),
                          mpsc_producer(ch, "P3", 3, 5ms));
  ch.close();
  co_await consumer;
}

// ---------- 示例 9：worker pool 扇出 / 扇入 ----------

auto pool_worker(int wid,
                 cobalt::channel<int>& jobs,
                 cobalt::channel<std::string>& results) -> cobalt::promise<void>
{
  for (;;) {
    auto const job = co_await cobalt::as_result(jobs.read());
    if (!job) {
      break;
    }
    co_await sleep_for(10ms * ((*job % 3) + 1));  // 模拟耗时不同的处理
    co_await results.write("worker#" + std::to_string(wid) + " 完成 job "
                           + std::to_string(*job) + " -> "
                           + std::to_string(*job * *job));
  }
  std::cout << "  worker#" << wid << " 退出 (jobs 通道已关闭)\n";
}

auto worker_pool_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<int> jobs {0, exec};  // rendezvous：任务直接递到 worker 手里
  cobalt::channel<std::string> results {6, exec};  // 容量 >= 任务数，防死锁

  auto w1 = pool_worker(1, jobs, results);
  auto w2 = pool_worker(2, jobs, results);
  auto w3 = pool_worker(3, jobs, results);

  for (int job = 1; job <= 6; ++job) {  // 扇出：空闲 worker 谁抢到谁做
    co_await jobs.write(job);
  }
  jobs.close();

  for (int i = 0; i < 6; ++i) {  // 扇入：从结果通道汇总
    // 先 co_await 拿到值再打印：若把 co_await 嵌进 << 链，
    // 打印前缀后挂起会让其他协程的日志插进半行中间
    auto const result = co_await results.read();
    std::cout << "  [汇总] " << result << '\n';
  }
  co_await cobalt::join(w1, w2, w3);
  results.close();
}

// ---------- 示例 10：多级流水线 ----------

auto stage_gen(cobalt::channel<int>& out, int count) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await out.write(i);
  }
  out.close();  // 每级只负责关自己的输出，"关闭"沿流水线逐级传播
}

auto stage_square(cobalt::channel<int>& in, cobalt::channel<int>& out)
    -> cobalt::promise<void>
{
  for (;;) {
    auto const val = co_await cobalt::as_result(in.read());
    if (!val) {
      break;
    }
    co_await out.write(*val * *val);
  }
  out.close();
}

auto stage_format(cobalt::channel<int>& in, cobalt::channel<std::string>& out)
    -> cobalt::promise<void>
{
  int idx = 1;
  for (;;) {
    auto const val = co_await cobalt::as_result(in.read());
    if (!val) {
      break;
    }
    co_await out.write(std::to_string(idx) + "^2 = " + std::to_string(*val));
    ++idx;
  }
  out.close();
}

auto stage_sink(cobalt::channel<std::string>& in) -> cobalt::promise<void>
{
  for (;;) {
    auto const line = co_await cobalt::as_result(in.read());
    if (!line) {
      break;
    }
    std::cout << "  [sink] " << *line << '\n';
  }
}

auto pipeline_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<int> nums {1, exec};
  cobalt::channel<int> squares {1, exec};
  cobalt::channel<std::string> lines {1, exec};
  // 四级并发流水线：gen -> square -> format -> sink，容量 1 的通道
  // 让相邻级可以流水化重叠，又不会无限堆积
  co_await cobalt::join(stage_gen(nums, 5),
                        stage_square(nums, squares),
                        stage_format(squares, lines),
                        stage_sink(lines));
}

// ---------- 示例 11：select（race 多路等待） ----------

auto feed_ints(cobalt::channel<int>& ch,
               int count,
               std::chrono::milliseconds pace) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await sleep_for(pace);
    co_await ch.write(i);
  }
}

auto feed_strs(cobalt::channel<std::string>& ch,
               int count,
               std::chrono::milliseconds pace) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await sleep_for(pace);
    co_await ch.write("慢速消息 " + std::to_string(i));
  }
}

auto select_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<int> fast {1, exec};
  cobalt::channel<std::string> slow {1, exec};

  auto f1 = feed_ints(fast, 4, 12ms);
  auto f2 = feed_strs(slow, 2, 35ms);

  // race 同时挂起在两个通道上，谁先有数据处理谁——等价 Go 的 select。
  // cobalt 的通道操作支持 interrupt_await：输掉 race 的那个 read 被
  // 安全中断，数据不会丢，下轮循环还能读到。
  for (int i = 0; i < 6; ++i) {
    auto which = co_await cobalt::race(fast.read(), slow.read());
    if (which.index() == 0) {
      std::cout << "  [select] fast 通道 -> " << boost::variant2::get<0>(which)
                << '\n';
    } else {
      std::cout << "  [select] slow 通道 -> " << boost::variant2::get<1>(which)
                << '\n';
    }
  }
  co_await cobalt::join(f1, f2);
  fast.close();
  slow.close();
}

// ---------- 示例 12：背压 backpressure ----------

auto bp_writer(cobalt::channel<int>& ch) -> cobalt::promise<void>
{
  for (int i = 1; i <= 4; ++i) {
    std::cout << "  [写端] 尝试写入 " << i << '\n';
    co_await ch.write(i);  // 缓冲满时这里挂起，被慢速读端反压
    std::cout << "  [写端] 写入完成 " << i << '\n';
  }
  ch.close();
}

auto bp_reader(cobalt::channel<int>& ch) -> cobalt::promise<void>
{
  for (;;) {
    co_await sleep_for(30ms);  // 故意读得慢
    auto const val = co_await cobalt::as_result(ch.read());
    if (!val) {
      break;
    }
    std::cout << "  [读端] 消费 " << *val << '\n';
  }
}

auto backpressure_demo() -> cobalt::task<void>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<int> ch {1, exec};  // 容量 1，很快被写满
  co_await cobalt::join(bp_writer(ch), bp_reader(ch));
}

// ---------- 示例 13：spawn + channel 综合 ----------

auto sum_producer(cobalt::channel<int>& ch, int count) -> cobalt::promise<void>
{
  for (int i = 1; i <= count; ++i) {
    co_await ch.write(i);
  }
  ch.close();
}

// 一个自带 CSP 内部结构的 task：生产者协程往通道里写 1..n，
// 本协程作为消费者累加，最终把结果通过 spawn 的句柄交还给普通代码
auto channel_sum(int count) -> cobalt::task<int>
{
  auto exec = co_await cobalt::this_coro::executor;
  cobalt::channel<int> ch {4, exec};
  auto producer = sum_producer(ch, count);
  int sum = 0;
  for (;;) {
    auto const val = co_await cobalt::as_result(ch.read());
    if (!val) {
      break;
    }
    sum += *val;
  }
  co_await producer;
  co_return sum;
}

}  // namespace

auto main() -> int
{
  asio::io_context ctx;
  // 关键一步：cobalt::spawn 只把 executor 绑给被 spawn 的 task 本身，
  // 不会设置线程局部的 this_thread executor。而 task 内部再创建
  // cobalt::promise / cobalt::channel（急切协程）时默认从
  // this_thread::get_executor() 取执行器，不设置就会抛 asio::bad_executor。
  // 在 cobalt::main / cobalt::thread 之外用 cobalt，都需要这句。
  cobalt::this_thread::set_executor(ctx.get_executor());

  // ======== 第一部分：cobalt::spawn 的各种"协程句柄" ========

  section("1. spawn + 回调：完成时收到 (exception_ptr, 结果)");
  cobalt::spawn(ctx,
                compute_square(7),
                [](std::exception_ptr ep, int value)
                {
                  if (ep) {
                    std::rethrow_exception(ep);
                  }
                  std::cout << "  回调收到结果: 7^2 = " << value << '\n';
                });
  ctx.run();
  ctx.restart();

  section("2. spawn + use_future：拿到 std::future 句柄");
  auto fut = cobalt::spawn(ctx, compute_square(9), asio::use_future);
  std::cout << "  future 已在手，事件循环还没跑，结果尚未就绪\n";
  ctx.run();
  ctx.restart();
  std::cout << "  future.get() = " << fut.get() << '\n';

  section("3. spawn + cancellation_slot：可取消的协程句柄");
  {
    asio::cancellation_signal cancel_sig;
    cobalt::spawn(
        ctx,
        tick_task("cancellable", 1000, 20ms),
        asio::bind_cancellation_slot(cancel_sig.slot(), asio::detached));
    asio::steady_timer stop {ctx, 75ms};
    stop.async_wait(
        [&cancel_sig](auto const&)
        {
          std::cout << "  75ms 到，通过句柄发出取消信号\n";
          cancel_sig.emit(asio::cancellation_type::all);
        });
    ctx.run();
    ctx.restart();
  }

  section("4. spawn 多个 detached 任务：单线程内交错并发");
  cobalt::spawn(ctx, tick_task("快速任务", 3, 10ms), asio::detached);
  cobalt::spawn(ctx, tick_task("中速任务", 3, 16ms), asio::detached);
  cobalt::spawn(ctx, tick_task("慢速任务", 3, 25ms), asio::detached);
  ctx.run();
  ctx.restart();

  // ======== 第二部分：协程内部的急切 promise 句柄 ========

  section("5. promise：创建即运行，句柄可 join 也可 cancel");
  run_blocking(ctx, promise_handle_demo());

  // ======== 第三部分：CSP channel 通信 ========

  section("6. SPSC：单生产者/单消费者（缓冲 4）");
  run_blocking(ctx, spsc_demo());

  section("7. rendezvous duo：容量 0 通道 ping-pong");
  run_blocking(ctx, pingpong_demo());

  section("8. MPSC：三个生产者 + gather 收口 + 统一关闭");
  run_blocking(ctx, mpsc_demo());

  section("9. worker pool：jobs 扇出 / results 扇入");
  run_blocking(ctx, worker_pool_demo());

  section("10. pipeline：gen -> square -> format -> sink");
  run_blocking(ctx, pipeline_demo());

  section("11. select：race 同时等待两个通道");
  run_blocking(ctx, select_demo());

  section("12. backpressure：慢消费者反压快生产者");
  run_blocking(ctx, backpressure_demo());

  section("13. 综合：spawn(use_future) 驱动 channel 求和 task");
  auto sum_fut = cobalt::spawn(ctx, channel_sum(100), asio::use_future);
  ctx.run();
  ctx.restart();
  std::cout << "  1..100 经通道汇总 = " << sum_fut.get() << " (期望 5050)\n";

  std::cout << "\n全部 13 个示例运行完毕\n";
  return 0;
}
