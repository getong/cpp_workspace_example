#pragma once

#include <coroutine>
#include <cstddef>
#include <exception>
#include <optional>
#include <utility>

#include "coro/frame_stats.hpp"

namespace coro
{

/**
 * @brief generator<T>：用 co_yield 惰性产生值序列的协程返回类型
 *
 * 用法：
 * @code
 *   auto naturals() -> coro::generator<int>
 *   {
 *     for (int i = 0;; ++i) {
 *       co_yield i;  // 挂起，把 i 交给消费者；下次 resume 从这里继续
 *     }
 *   }
 *
 *   for (int n : naturals()) { ... }
 * @endcode
 *
 * 关键点：
 *  - 函数体里出现 co_yield，编译器就把整个函数改写成状态机；
 *  - 局部变量 i 存活在堆上的协程帧中，而不是线程栈上，所以挂起后
 *    栈帧可以完全弹出，值却不会丢；
 *  - initial_suspend 返回 suspend_always，因此生成器是惰性的：
 *    调用 naturals() 只分配协程帧，一行函数体都不会执行。
 */
template <typename T>
class [[nodiscard]] generator
{
public:
  /**
   * promise_type 是编译器与协程之间的"协议"：编译器在固定的时机调用
   * 这些固定名字的函数，我们通过实现它们来定制协程的行为。
   */
  struct promise_type : frame_counted
  {
    /// 最近一次 co_yield 出来的值（存活在协程帧里）
    std::optional<T> current;
    /// 协程体内逃逸的异常，暂存后在消费者一侧重新抛出
    std::exception_ptr exception;

    /// 协程帧创建后，编译器用它构造返回给调用者的 generator 对象
    auto get_return_object() -> generator
    {
      return generator {std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    /// 惰性启动：调用生成器函数只创建协程帧，不执行函数体
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    /// 结束时挂起在最终暂停点，协程帧由 generator 的析构函数统一销毁
    auto final_suspend() noexcept -> std::suspend_always { return {}; }

    /// co_yield v 会被编译器改写为 co_await promise.yield_value(v)
    auto yield_value(T value) -> std::suspend_always
    {
      current.emplace(std::move(value));
      return {};  // suspend_always：交出控制权，回到消费者
    }

    /// 协程体自然走完（或 co_return;）时调用
    void return_void() noexcept {}

    /// 协程体抛出异常且未捕获时调用
    void unhandled_exception() { exception = std::current_exception(); }
  };

  using handle_type = std::coroutine_handle<promise_type>;

  generator() = default;

  explicit generator(handle_type handle)
      : handle_ {handle}
  {
  }

  // 协程帧只能销毁一次，generator 独占句柄所有权，因此只允许移动
  generator(generator const&) = delete;
  auto operator=(generator const&) -> generator& = delete;

  generator(generator&& other) noexcept
      : handle_ {std::exchange(other.handle_, {})}
  {
  }

  auto operator=(generator&& other) noexcept -> generator&
  {
    if (this != &other) {
      destroy();
      handle_ = std::exchange(other.handle_, {});
    }
    return *this;
  }

  ~generator() { destroy(); }

  /**
   * @brief 恢复协程执行到下一个 co_yield
   * @return false 表示协程体已执行完毕，没有更多值
   */
  auto next() -> bool
  {
    if (!handle_ || handle_.done()) {
      return false;
    }
    handle_.resume();  // 从上一个挂起点继续执行状态机
    if (handle_.promise().exception) {
      std::rethrow_exception(handle_.promise().exception);
    }
    return !handle_.done();
  }

  /// 读取最近一次 co_yield 出来的值
  auto value() const -> T const& { return *handle_.promise().current; }

  // ---- range-for 支持：让生成器可以直接用在范围 for 循环里 ----

  struct sentinel
  {
  };

  class iterator
  {
  public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;

    explicit iterator(generator& gen)
        : gen_ {&gen}
    {
    }

    auto operator*() const -> T const& { return gen_->value(); }

    auto operator++() -> iterator&
    {
      done_ = !gen_->next();
      return *this;
    }

    void operator++(int) { ++*this; }

    friend auto operator==(iterator const& iter, sentinel /*end*/) -> bool
    {
      return iter.done_;
    }

  private:
    generator* gen_;
    bool done_ = false;
  };

  auto begin() -> iterator
  {
    iterator iter {*this};
    ++iter;  // 执行到第一个 co_yield（或直接结束）
    return iter;
  }

  auto end() -> sentinel { return {}; }

private:
  void destroy()
  {
    if (handle_) {
      handle_.destroy();  // 调用帧内对象的析构函数并释放协程帧
      handle_ = {};
    }
  }

  handle_type handle_;
};

}  // namespace coro
