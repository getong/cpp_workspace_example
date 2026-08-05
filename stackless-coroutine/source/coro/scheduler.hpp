#pragma once

#include <chrono>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

namespace coro
{

/**
 * @brief 单线程事件循环：演示"挂起的协程如何被外部事件恢复"
 *
 * 这是所有异步运行时（asio、libuv、tokio…）的最小骨架：
 *  - 协程 co_await 某个事件（这里是定时器）时，awaiter 把协程句柄
 *    登记到调度器里，然后协程保持挂起，线程去做别的事；
 *  - run() 循环不断取出到期/就绪的句柄并 resume()，协程便从
 *    挂起点原地继续执行。
 *
 * 因为协程状态保存在堆上的协程帧里（无栈特性），一个线程可以同时
 * "挂着"任意多个协程，实现协作式并发。
 */
class scheduler
{
public:
  using clock = std::chrono::steady_clock;

  /**
   * co_await sched.sleep_for(...) 用到的 awaitable。
   * 展示 awaitable 协议最典型的用法：把自己（协程句柄）交给外部系统，
   * 由外部系统在未来某个时刻恢复自己。
   */
  struct sleep_awaiter
  {
    scheduler* sched;
    clock::time_point due;

    /// 已经到期就不必挂起，直接继续执行
    auto await_ready() const -> bool { return clock::now() >= due; }

    /// handle 就是当前协程；登记到定时器队列后返回，协程保持挂起
    void await_suspend(std::coroutine_handle<> handle) const
    {
      sched->add_timer(due, handle);
    }

    void await_resume() const noexcept {}
  };

  /// 挂起当前协程，duration 之后由事件循环恢复
  auto sleep_for(std::chrono::milliseconds duration) -> sleep_awaiter
  {
    return sleep_awaiter {this, clock::now() + duration};
  }

  /**
   * co_await sched.yield()：主动让出执行权，把自己排到就绪队列尾部。
   * 用于演示确定性的协作式轮转调度（不依赖真实时间）。
   */
  struct yield_awaiter
  {
    scheduler* sched;

    auto await_ready() const noexcept -> bool { return false; }

    void await_suspend(std::coroutine_handle<> handle) const
    {
      sched->ready_.push_back(handle);
    }

    void await_resume() const noexcept {}
  };

  auto yield() -> yield_awaiter { return yield_awaiter {this}; }

  /// 运行事件循环，直到没有任何就绪或计时中的协程
  void run()
  {
    while (!ready_.empty() || !timers_.empty()) {
      if (ready_.empty()) {
        // 无事可做：睡到最近的定时器到期（真实运行时在这里 epoll/kqueue）
        std::this_thread::sleep_until(timers_.top().due);
      }
      // 把所有到期的定时器搬进就绪队列
      while (!timers_.empty() && timers_.top().due <= clock::now()) {
        ready_.push_back(timers_.top().handle);
        timers_.pop();
      }
      if (!ready_.empty()) {
        auto handle = ready_.front();
        ready_.pop_front();
        handle.resume();  // 协程从挂起点继续，直到下一次挂起或执行完毕
      }
    }
  }

private:
  struct timer_entry
  {
    clock::time_point due;
    std::uint64_t seq;  // 同一时刻到期时按登记顺序恢复
    std::coroutine_handle<> handle;

    friend auto operator>(timer_entry const& lhs, timer_entry const& rhs) -> bool
    {
      return std::tie(lhs.due, lhs.seq) > std::tie(rhs.due, rhs.seq);
    }
  };

  void add_timer(clock::time_point due, std::coroutine_handle<> handle)
  {
    timers_.push(timer_entry {due, next_seq_++, handle});
  }

  std::deque<std::coroutine_handle<>> ready_;
  std::priority_queue<timer_entry, std::vector<timer_entry>, std::greater<>> timers_;
  std::uint64_t next_seq_ = 0;
};

}  // namespace coro
