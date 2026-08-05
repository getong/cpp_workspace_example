#pragma once

#include <cstddef>
#include <new>

namespace coro
{

/**
 * @brief 协程帧分配统计
 *
 * 无栈协程的局部变量与挂起点状态不保存在线程栈上，而是保存在
 * "协程帧（coroutine frame）"里。帧的内存由编译器生成的代码在协程
 * 创建时一次性分配——如果 promise_type 定义了 operator new，就用它，
 * 否则退回全局 ::operator new（即堆分配）。
 *
 * 本项目让所有 promise_type 继承 frame_counted，从而可以在运行期
 * 直接观测到：帧确实是动态分配的、帧有多大、帧何时被销毁。
 */
struct frame_stats
{
  /// 当前存活（已分配未释放）的协程帧数量
  static inline std::size_t live_frames = 0;
  /// 累计分配次数
  static inline std::size_t total_allocations = 0;
  /// 最近一次分配的帧大小（字节），由编译器计算后传入 operator new
  static inline std::size_t last_frame_size = 0;

  static void reset()
  {
    live_frames = 0;
    total_allocations = 0;
    last_frame_size = 0;
  }
};

/**
 * @brief promise_type 的基类：接管协程帧的分配与释放并计数
 *
 * 编译器为协程分配帧时调用的是 promise_type::operator new（若存在），
 * 传入的 size 是整个协程帧的大小：promise 对象 + 形参副本 + 跨越挂起点
 * 的局部变量 + 记录"挂起在第几号暂停点"的状态编号。
 */
struct frame_counted
{
  static auto operator new(std::size_t size) -> void*
  {
    ++frame_stats::live_frames;
    ++frame_stats::total_allocations;
    frame_stats::last_frame_size = size;
    return ::operator new(size);
  }

  static void operator delete(void* ptr, std::size_t /*size*/) noexcept
  {
    --frame_stats::live_frames;
    ::operator delete(ptr);
  }
};

}  // namespace coro
