#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

#include "jthread.hpp"

namespace modules::jthread
{

namespace
{

using namespace std::chrono_literals;

// 演示用的时间参数：worker 每个周期的睡眠时长、主线程给 worker
// 留出的运行时间。取值只为让输出可观察，同时保持整体运行足够快。
constexpr auto tick_interval = 10ms;
constexpr auto fast_tick_interval = 5ms;
constexpr auto let_worker_run = 30ms;
constexpr auto let_worker_run_longer = 50ms;
constexpr int worker_count = 3;

// ---- 1. 基本用法：协作式取消 -----------------------------------------------

void basic_cooperative_stop()
{
  std::cout << "-- basic cooperative stop --\n";

  // 若线程函数的首个参数是 std::stop_token，jthread 构造时会自动
  // 把与线程关联的停止令牌传进去，无需手动创建 stop_source 再传递。
  std::jthread worker(
      [](const std::stop_token& stoken)
      {
        int ticks = 0;
        // 取消是协作式的：线程必须主动轮询 stop_requested()，
        // 停止请求不会强行打断正在执行的代码。
        while (!stoken.stop_requested()) {
          ++ticks;
          std::this_thread::sleep_for(tick_interval);
        }
        std::cout << "worker: 收到停止请求，共运行 " << ticks << " 个周期\n";
      });

  std::this_thread::sleep_for(let_worker_run_longer);
  std::cout << "main: 请求停止子线程\n";
  worker.request_stop();
  // 无需手动 join：jthread 析构时自动 join（见第 4 节）。
}

// ---- 2. stop_source / stop_token 本身与线程无关 ----------------------------

void standalone_stop_source()
{
  std::cout << "-- standalone stop_source/stop_token --\n";

  // 停止机制是独立组件：stop_source 负责发起取消，stop_token 负责
  // 观察取消。它可以用在任何需要"协作式取消"的场合——单线程的
  // 长循环、异步任务、IO 轮询——线程只是最常见的使用者。
  std::stop_source source;
  const std::stop_token token = source.get_token();

  constexpr int total_steps = 10;
  constexpr int cancel_after_step = 4;

  int step = 0;
  for (; step < total_steps; ++step) {
    if (token.stop_requested()) {
      break;
    }
    if (step == cancel_after_step) {
      // 模拟"某个业务条件成立后取消任务"（原例中是超过 5 秒）。
      source.request_stop();
    }
  }

  if (token.stop_requested()) {
    std::cout << "任务在第 " << step << " 步被取消\n";
  } else {
    std::cout << "任务正常完成\n";
  }
}

// ---- 3. stop_callback：停止请求发出时执行回调 ------------------------------

void stop_callback_usage()
{
  std::cout << "-- stop_callback --\n";

  std::jthread worker(
      [](const std::stop_token& stoken)
      {
        // 注册回调：request_stop() 发生时由发起停止的线程同步执行。
        // 典型用途是唤醒阻塞中的操作：关闭 socket、向队列投毒、
        // notify 条件变量等，让阻塞的线程有机会看到停止请求。
        const std::stop_callback callback(
            stoken, [] { std::cout << "callback: 停止请求已发出\n"; });

        while (!stoken.stop_requested()) {
          std::this_thread::sleep_for(tick_interval);
        }
        std::cout << "worker: 退出循环\n";
      });

  std::this_thread::sleep_for(let_worker_run);
  worker.request_stop();  // 上面的回调就在这一行内部被执行
  worker.join();

  // 边界情况：注册回调时停止已经发生，回调在构造处立即执行。
  std::stop_source already_stopped;
  already_stopped.request_stop();
  const std::stop_callback late(
      already_stopped.get_token(),
      [] { std::cout << "callback: 注册时已停止，立即执行\n"; });
}

// ---- 4. RAII：析构自动 request_stop + join ---------------------------------

void raii_auto_join()
{
  std::cout << "-- RAII auto stop & join --\n";

  std::atomic<bool> finished {false};
  {
    // std::thread 析构时若既没 join 也没 detach 会直接 std::terminate，
    // 提前 return 或抛异常都可能踩中；std::jthread 析构时自动先
    // request_stop() 再 join()，作用域结束即安全收尾，异常路径同样成立。
    std::jthread worker(
        [&finished](const std::stop_token& stoken)
        {
          while (!stoken.stop_requested()) {
            std::this_thread::sleep_for(fast_tick_interval);
          }
          finished = true;
        });
    std::this_thread::sleep_for(let_worker_run);
    // 注意：这里故意不调用 request_stop() / join()。
  }  // <- 析构在此自动 request_stop() + join()
  std::cout << std::boolalpha << "作用域结束，worker 已收尾: " << finished
            << '\n';

  // 线程函数不接收 stop_token 也可以用 jthread：没有协作取消，
  // 但仍享受析构自动 join（前提是函数自己会返回）。
  {
    std::jthread plain([] { std::this_thread::sleep_for(tick_interval); });
  }
  std::cout << "不带 stop_token 的函数同样自动 join\n";
}

// ---- 5. 可中断的等待：condition_variable_any + stop_token ------------------

void interruptible_wait()
{
  std::cout << "-- interruptible wait --\n";

  // sleep_for 无法被停止请求打断：轮询 + 长睡眠意味着最坏要睡完
  // 整个周期才能响应停止。condition_variable_any 针对 stop_token
  // 重载了 wait 系列接口，request_stop() 会立即唤醒等待者——
  // 这才是"长时间等待且需要及时取消"场景的正确写法。
  std::mutex mutex;
  std::condition_variable_any condvar;

  const auto start = std::chrono::steady_clock::now();
  std::jthread waiter(
      [&](const std::stop_token& stoken)
      {
        std::unique_lock lock(mutex);
        // 谓词恒为 false，相当于"等一个永远不来的事件"；
        // 停止请求会让 wait 立即返回，返回值是谓词的结果（false）。
        const bool satisfied = condvar.wait(lock, stoken, [] { return false; });
        const auto waited =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
        std::cout << std::boolalpha
                  << "wait 被停止请求唤醒，谓词满足 = " << satisfied
                  << "，实际只等了约 " << waited.count() << "ms\n";
      });

  std::this_thread::sleep_for(let_worker_run);
  waiter.request_stop();  // 无需 notify_*()，等待者立即醒来
}

// ---- 6. 一个 stop_source 统一关停多个线程 ----------------------------------

void coordinated_shutdown()
{
  std::cout << "-- one stop_source, many threads --\n";

  // jthread 内置的停止令牌只管自己。把同一个外部 stop_source 的
  // token 显式传给多个线程（显式实参会取代自动注入的内置令牌），
  // 一次 request_stop() 即可统一关停整组任务——线程池、服务优雅
  // 退出都是这个模式。
  std::stop_source group;
  std::atomic<int> total_ticks {0};

  std::vector<std::jthread> workers;
  workers.reserve(worker_count);
  for (int i = 0; i < worker_count; ++i) {
    workers.emplace_back(
        [&total_ticks](const std::stop_token& stoken)
        {
          while (!stoken.stop_requested()) {
            ++total_ticks;
            std::this_thread::sleep_for(fast_tick_interval);
          }
        },
        group.get_token());
  }

  std::this_thread::sleep_for(let_worker_run);
  group.request_stop();  // 一次调用，所有线程同时收到
  for (auto& worker : workers) {
    worker.join();
  }
  std::cout << worker_count << " 个 worker 已全部停止，总计 tick "
            << total_ticks << " 次\n";
}

// ---- 7. 从外部访问 jthread 的停止状态 --------------------------------------

void external_stop_control()
{
  std::cout << "-- get_stop_source / get_stop_token --\n";

  std::jthread worker(
      [](const std::stop_token& stoken)
      {
        while (!stoken.stop_requested()) {
          std::this_thread::sleep_for(fast_tick_interval);
        }
      });

  // jthread 自带一个 stop_source，可以取出来交给其他组件（超时器、
  // 信号处理、上层调度器），由它们在合适的时机发起停止，而不必
  // 持有整个 jthread 对象。
  std::stop_source source = worker.get_stop_source();
  std::cout << std::boolalpha << "stop_possible = " << source.stop_possible()
            << '\n';

  source.request_stop();  // 效果等价于 worker.request_stop()
  worker.join();
  std::cout << "通过外部持有的 stop_source 停止成功，stop_requested = "
            << worker.get_stop_token().stop_requested() << '\n';
}

}  // namespace

void run()
{
  basic_cooperative_stop();
  standalone_stop_source();
  stop_callback_usage();
  raii_auto_join();
  interruptible_wait();
  coordinated_shutdown();
  external_stop_control();
}

}  // namespace modules::jthread
