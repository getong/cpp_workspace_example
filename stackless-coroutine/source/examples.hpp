#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "coro/generator.hpp"
#include "coro/scheduler.hpp"
#include "coro/task.hpp"

/**
 * @brief C++20 无栈协程的示例集合
 *
 * 每个函数演示一个概念，实现见 examples.cpp，讲解见 README.md。
 * main.cpp 调用 run_all_demos() 依次运行并打印说明；
 * test/ 下的用例则对这些函数的行为做断言。
 */
namespace examples
{

// ---- co_yield：生成器 ----

/// 惰性生成前 count 个斐波那契数（局部变量存活在协程帧中）
auto fibonacci(std::size_t count) -> coro::generator<std::uint64_t>;

/// 产出 first 后抛异常：演示异常从协程体传播到消费者
auto throwing_generator(int first) -> coro::generator<int>;

// ---- co_await / co_return：任务组合 ----

/// 模拟一个会产出结果的异步计算
auto async_value(int value) -> coro::task<int>;

/// co_await 两个子任务并把结果相加：演示任务链与对称转移
auto async_sum(int lhs, int rhs) -> coro::task<int>;

// ---- 调度器：单线程协作式并发 ----

/// 每隔 interval 打一次点，把 "name#i" 追加到 log
auto ticker(coro::scheduler& sched,
            std::string name,
            int count,
            std::chrono::milliseconds interval,
            std::vector<std::string>& log) -> coro::task<void>;

/// 用真实定时器在一个线程上并发运行快慢两个 ticker，返回交错日志
auto interleaved_tickers() -> std::vector<std::string>;

/// 不依赖时间的确定性版本：两个协程通过 yield() 轮流执行
auto round_robin(int rounds) -> std::vector<std::string>;

/// 运行全部演示并向 stdout 打印讲解
void run_all_demos();

}  // namespace examples
