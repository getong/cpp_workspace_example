#include "coroutine.hpp"

#include <coroutine>
#include <cstdint>
#include <exception>
#include <iostream>
#include <utility>

namespace modules::coroutine
{

namespace
{

// 一个最小可用的生成器：协程每次 co_yield 一个值后挂起，
// 调用方通过 next() 恢复协程取下一个值，整个序列按需计算。
template<typename T>
class generator
{
public:
  // promise_type 是编译器和协程之间的“协议”：
  // 函数体里出现 co_yield/co_return 时，编译器按这里的定义生成代码。
  struct promise_type
  {
    T current_value {};

    auto get_return_object() -> generator
    {
      return generator {
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    // initial_suspend 返回 suspend_always：协程创建后先挂起，不立刻执行。
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    // final_suspend 返回 suspend_always：结束后保留协程帧，由 generator 析构。
    auto final_suspend() noexcept -> std::suspend_always { return {}; }

    // co_yield value 等价于 co_await yield_value(value)。
    auto yield_value(T value) noexcept -> std::suspend_always
    {
      current_value = value;
      return {};
    }

    void return_void() noexcept {}

    void unhandled_exception() { std::terminate(); }
  };

  explicit generator(std::coroutine_handle<promise_type> handle)
      : m_handle {handle}
  {
  }

  generator(const generator&) = delete;
  auto operator=(const generator&) -> generator& = delete;

  generator(generator&& other) noexcept
      : m_handle {std::exchange(other.m_handle, nullptr)}
  {
  }

  auto operator=(generator&& other) noexcept -> generator&
  {
    if (this != &other) {
      destroy();
      m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
  }

  ~generator() { destroy(); }

  // 恢复协程执行到下一个 co_yield；返回 false 表示序列结束。
  auto next() -> bool
  {
    m_handle.resume();
    return !m_handle.done();
  }

  auto value() const -> T { return m_handle.promise().current_value; }

private:
  void destroy()
  {
    if (m_handle) {
      m_handle.destroy();
    }
  }

  std::coroutine_handle<promise_type> m_handle;
};

// 无限斐波那契数列：状态保存在协程帧里，调用方决定取多少个。
auto fibonacci() -> generator<std::int64_t>
{
  std::int64_t prev = 0;
  std::int64_t curr = 1;
  while (true) {
    co_yield prev;
    const std::int64_t next = prev + curr;
    prev = curr;
    curr = next;
  }
}

}  // namespace

void run()
{
  auto gen = fibonacci();
  std::cout << "first 10 fibonacci numbers:";
  for (int i = 0; i < 10 && gen.next(); ++i) {
    std::cout << ' ' << gen.value();
  }
  std::cout << '\n';
}

}  // namespace modules::coroutine
