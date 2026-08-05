#pragma once

#include <cassert>
#include <coroutine>
#include <exception>
#include <utility>
#include <variant>

#include "coro/frame_stats.hpp"

namespace coro
{

template <typename T>
class task;

namespace detail
{

/// 存放 co_return 的结果：空（未完成）/ 值 / 异常，三选一
template <typename T>
struct task_result
{
  std::variant<std::monostate, T, std::exception_ptr> value;

  /// co_return expr; 会被编译器改写为 promise.return_value(expr)
  void return_value(T result) { value.template emplace<1>(std::move(result)); }

  void unhandled_exception() { value.template emplace<2>(std::current_exception()); }

  auto take() -> T
  {
    if (auto* exception = std::get_if<2>(&value)) {
      std::rethrow_exception(*exception);
    }
    assert(value.index() == 1 && "任务尚未完成，没有结果可取");
    return std::move(std::get<1>(value));
  }
};

/// void 特化：co_return; 对应 return_void()
template <>
struct task_result<void>
{
  std::exception_ptr exception;

  void return_void() noexcept {}

  void unhandled_exception() { exception = std::current_exception(); }

  void take() const
  {
    if (exception) {
      std::rethrow_exception(exception);
    }
  }
};

}  // namespace detail

/**
 * @brief task<T>：一次性异步任务，用 co_await 组合、用 co_return 返回结果
 *
 * 用法：
 * @code
 *   auto step() -> coro::task<int> { co_return 21; }
 *
 *   auto total() -> coro::task<int>
 *   {
 *     int lhs = co_await step();  // 挂起自己，控制权转移给子任务
 *     co_return lhs * 2;
 *   }
 * @endcode
 *
 * 关键点：
 *  - 惰性（initial_suspend = suspend_always）：创建任务不执行任何代码，
 *    直到被 co_await 或手动 start()；
 *  - co_await 子任务时通过"对称转移"（await_suspend 返回句柄）直接跳转，
 *    任务链再深也不会增长线程栈，天然免疫栈溢出；
 *  - 子任务完成时，final_awaiter 把控制权交还给等待者（continuation）。
 */
template <typename T = void>
class [[nodiscard]] task
{
public:
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  /**
   * final_suspend 返回的 awaiter：任务执行完 co_return 后，
   * 通过对称转移把控制权直接交还给等待本任务的协程。
   */
  struct final_awaiter
  {
    auto await_ready() const noexcept -> bool { return false; }

    auto await_suspend(handle_type handle) noexcept -> std::coroutine_handle<>
    {
      auto continuation = handle.promise().continuation;
      // 有等待者就跳回等待者；没有（顶层任务）就返回 noop 协程，
      // 意为"无事可做，控制权回到最初调用 resume() 的普通代码"
      return continuation ? continuation : std::noop_coroutine();
    }

    void await_resume() const noexcept {}
  };

  struct promise_type
      : frame_counted
      , detail::task_result<T>
  {
    /// 正在 co_await 本任务的协程，任务完成后要恢复它
    std::coroutine_handle<> continuation;

    auto get_return_object() -> task
    {
      return task {handle_type::from_promise(*this)};
    }

    /// 惰性启动：任务创建后先挂起，等待被 co_await 或 start()
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    auto final_suspend() noexcept -> final_awaiter { return {}; }
  };

  /**
   * co_await task 时编译器使用的 awaiter，实现 awaitable 协议三件套：
   *  - await_ready：要不要挂起？
   *  - await_suspend：挂起后做什么？（这里：记录等待者并启动子任务）
   *  - await_resume：恢复时向 co_await 表达式返回什么值？
   */
  struct awaiter
  {
    handle_type handle;

    auto await_ready() const noexcept -> bool
    {
      return !handle || handle.done();
    }

    /// 返回句柄 => 对称转移：当前协程保持挂起，立刻执行子任务，不压栈
    auto await_suspend(std::coroutine_handle<> waiting) noexcept
        -> std::coroutine_handle<>
    {
      handle.promise().continuation = waiting;
      return handle;
    }

    auto await_resume() -> T { return handle.promise().take(); }
  };

  task() = default;

  explicit task(handle_type handle)
      : handle_ {handle}
  {
  }

  // 与 generator 相同：独占协程帧所有权，只允许移动
  task(task const&) = delete;
  auto operator=(task const&) -> task& = delete;

  task(task&& other) noexcept
      : handle_ {std::exchange(other.handle_, {})}
  {
  }

  auto operator=(task&& other) noexcept -> task&
  {
    if (this != &other) {
      destroy();
      handle_ = std::exchange(other.handle_, {});
    }
    return *this;
  }

  ~task() { destroy(); }

  /// 使 task 可以被 co_await
  auto operator co_await() const noexcept -> awaiter { return awaiter {handle_}; }

  /// 手动启动顶层任务（没有别的协程 co_await 它时使用）
  void start()
  {
    if (handle_ && !handle_.done()) {
      handle_.resume();
    }
  }

  auto done() const -> bool { return !handle_ || handle_.done(); }

  /// 取出结果（任务完成后调用）；协程内的异常会在这里重新抛出
  auto result() -> T { return handle_.promise().take(); }

private:
  void destroy()
  {
    if (handle_) {
      handle_.destroy();
      handle_ = {};
    }
  }

  handle_type handle_;
};

/**
 * @brief 同步执行一个纯计算的任务链并返回结果
 *
 * 只适用于不等待外部事件的任务：整条链会在 start() 的调用栈上
 * 一口气跑完。会真正挂起等待定时器/IO 的任务需要事件循环驱动，
 * 见 scheduler.hpp。
 */
template <typename T>
auto sync_wait(task<T> pending) -> T
{
  pending.start();
  assert(pending.done() && "任务在等待外部事件，需要调度器驱动（见 scheduler.hpp）");
  return pending.result();
}

}  // namespace coro
