# stackless-coroutine —— C++20 无栈协程教学项目

一个可编译、可运行、带测试的小项目，从零实现三个最核心的协程组件，
把 C++20 无栈协程的每个概念都落到可观测的代码上：

| 组件 | 文件 | 演示的概念 |
|---|---|---|
| `coro::generator<T>` | [`source/coro/generator.hpp`](source/coro/generator.hpp) | `co_yield`、惰性求值、promise_type 协议、range-for |
| `coro::task<T>` | [`source/coro/task.hpp`](source/coro/task.hpp) | `co_await` / `co_return`、任务组合、对称转移、异常传播 |
| `coro::scheduler` | [`source/coro/scheduler.hpp`](source/coro/scheduler.hpp) | 事件循环、定时器 awaitable、单线程协作式并发 |
| `coro::frame_stats` | [`source/coro/frame_stats.hpp`](source/coro/frame_stats.hpp) | 协程帧的堆分配：何时分配、多大、何时释放 |
| 示例集合 | [`source/examples.cpp`](source/examples.cpp) | 上述组件的具体用法，`main` 运行全部演示 |
| 测试 | [`test/source/stackless-coroutine_test.cpp`](test/source/stackless-coroutine_test.cpp) | 对每个行为的断言（惰性、异常、帧计数、交错顺序） |

运行 demo 的输出（节选）：

```text
=== 1. co_yield 与协程帧 ===
  调用 fibonacci(10) 后：帧数 = 1，帧大小 = 88 字节（堆分配，惰性，尚未执行函数体）
  前 10 个斐波那契数：0 1 1 2 3 5 8 13 21 34
  generator 离开作用域后：帧数 = 0（RAII 销毁协程帧）

=== 4. 单线程协作式并发（事件循环 + 定时器）===
  slow 每 50ms 打点 2 次，fast 每 20ms 打点 3 次，同一个线程交错执行：
  fast#1 fast#2 slow#1 fast#3 slow#2
```

---

## 1. 什么是无栈协程

**协程 = 可以暂停和恢复的函数。** 普通函数只有"调用"和"返回"两种控制流；
协程多了两种：**挂起**（保存现场、交出控制权）和**恢复**（回到挂起点继续执行）。

C++20 采用的是**无栈（stackless）**方案：

- 协程**没有自己的调用栈**。挂起时需要保存的状态（跨越挂起点的局部变量、
  形参副本、"挂起在第几号暂停点"的编号）保存在一块叫**协程帧
  （coroutine frame）**的内存里，默认在堆上分配。
- 编译器把协程函数体**改写成状态机**：每个 `co_await` / `co_yield` 是一个
  状态编号，`resume()` 本质上是"调用一个普通函数 + 按编号 `switch` 跳转到
  上次的挂起点"。因此挂起/恢复的开销与一次普通函数调用相当，
  远低于有栈协程（fiber）保存/恢复整套寄存器和栈指针的上下文切换。
- 对比有栈协程（如 boost::fiber、goroutine）：有栈方案给每个协程分配独立
  栈（几 KB 到几 MB），可以在任意调用深度挂起；无栈方案每个协程只占一个
  帧（本项目的 `fibonacci` 帧只有 88 字节），百万级并发也不成问题，
  代价是**只能在协程函数体内挂起**（见 §6 传染性）。

## 2. 三个关键字

函数体内只要出现任意一个关键字，这个函数就"变成"协程：

| 关键字 | 作用 | 编译器改写为 | 本项目示例 |
|---|---|---|---|
| `co_yield v` | 挂起并向消费者产出一个值 | `co_await promise.yield_value(v)` | `examples::fibonacci` |
| `co_await e` | 挂起并等待 e 完成 | awaitable 协议三件套（§4） | `examples::async_sum` |
| `co_return v` | 结束协程并交付最终结果 | `promise.return_value(v)` 后走 `final_suspend` | `examples::async_value` |

注意：协程的"返回值"（如 `generator<int>`、`task<int>`）**不是** `co_return`
的值，而是在协程**创建时**就返回给调用者的**句柄包装对象**——真正的结果要等
协程执行完后再从 promise 里取。

## 3. promise_type：编译器与你的协议

写一个协程返回类型，本质是回答编译器在固定时机提出的固定问题。
编译器看到 `generator<int> f() { co_yield 1; }` 时生成的伪代码：

```cpp
generator<int> f()
{
  // 1. 分配协程帧（走 promise_type::operator new，见 frame_stats.hpp）
  auto* frame = new __f_frame{...};

  // 2. 构造 promise，向调用者返回包装对象
  generator<int> ret = frame->promise.get_return_object();

  // 3. co_await promise.initial_suspend();
  //    suspend_always => 立刻挂起返回，函数体一行都不执行（惰性）
  //    suspend_never  => 继续执行函数体（急切）

  // ... 函数体，被切分成以挂起点为界的状态机 ...
  // 出口一：co_return   => promise.return_value(v) / return_void()
  // 出口二：异常逃逸    => promise.unhandled_exception()

  // 4. co_await promise.final_suspend();
  //    suspend_always => 帧保留，由包装对象析构时 destroy()（本项目做法）
  //    suspend_never  => 帧立即自毁（fire-and-forget 风格）
}
```

对照 [`generator.hpp`](source/coro/generator.hpp) 里的 `promise_type`，
每个成员函数上都有注释说明它在上面哪一步被调用。

## 4. awaitable 协议：co_await 到底做了什么

`co_await expr` 要求 expr（或其转换结果）提供三个函数，
demo 2 用一个会打印日志的 awaiter 展示了调用顺序：

```cpp
struct logging_awaiter
{
  bool await_ready();                              // ① 问：需要挂起吗？
  auto await_suspend(std::coroutine_handle<> h);   // ② 已挂起，现场已存进协程帧
  void await_resume();                             // ③ 恢复了，产出 co_await 的结果
};
```

- **`await_ready()`** 返回 `true` 则跳过挂起（快路径，比如数据已就绪）。
- **`await_suspend(h)`** 拿到的 `h` 就是**当前协程的句柄**——这是整个协程
  机制的枢纽：把 `h` 交给事件循环/线程池/回调，将来谁调 `h.resume()`，
  协程就在哪里复活。返回值决定接下来做什么：
  - `void` / `true`：保持挂起，控制权回到调用方；
  - `false`：立即恢复自己；
  - **另一个协程句柄**：**对称转移（symmetric transfer）**，直接跳去执行
    那个协程，不增长线程栈——`task` 用它实现"co_await 子任务"
    （见 `task.hpp` 的 `awaiter` 与 `final_awaiter`），任务链再深也不会栈溢出。
- **`await_resume()`** 的返回值就是 `co_await` 表达式的值。

[`scheduler.hpp`](source/coro/scheduler.hpp) 的 `sleep_awaiter` 是最典型的
真实用法：`await_suspend` 把句柄登记进定时器队列，事件循环在到期时
`resume()`。demo 4 里两个 ticker 就这样在**一个线程**上交错执行。

## 5. 协程帧：内存到底在哪

`frame_stats.hpp` 给所有 promise_type 加了记账的 `operator new/delete`
（编译器分配协程帧时优先使用 promise_type 的分配函数），于是可以直接观测：

- 调用 `fibonacci(10)` 的瞬间：**帧数 +1**，本机上帧大小 88 字节——
  里面装着 promise（`optional<uint64_t>` + `exception_ptr`）、
  局部变量 `a`、`b`、`i` 和状态编号；
- 挂起期间线程栈上不留任何东西，值都活在帧里；
- `generator` 对象析构 → `handle.destroy()` → **帧数归零**。

测试 `协程帧在堆上分配并随包装对象销毁` 对此有断言。

> 编译器在能证明帧生命周期不逃逸时可以把堆分配优化掉（HALO 优化），
> 但语义上永远应当按"帧在堆上"来理解。

## 6. 传染性与限制

- **只能在协程函数体内挂起。** `co_await` 不能出现在协程调用的普通函数里——
  因为挂起要靠编译器在**当前函数**里生成状态机，普通函数没有帧可以保存现场。
  于是异步会"传染"：底层函数变成协程后，想等它的调用者也得变成协程，
  一路传染到顶层，最后由 `sync_wait` / 事件循环这类"桥"收口
  （见 `main.cpp` → `run_all_demos` → `sched.run()`）。
- `main`、构造/析构函数、`constexpr` 函数不能是协程。
- 语言只提供机制不提供设施：标准库直到 C++23 才有 `std::generator`，
  `task`、调度器、`when_all` 等都要自己写或用库（cppcoro、libcoro、
  asio、folly::coro）。这正是本项目手写这三件套的原因——它们就是
  那些库的最小骨架。

## 7. 常见陷阱（本项目代码里的对应防御）

1. **悬垂引用**：协程按引用捕获/接收的对象必须活得比协程久。
   `examples::ticker` 按值接收 `std::string name`（副本进帧，安全），
   按引用接收 `log`（由调用者保证生命周期，注释里有标注）。
   lambda 协程尤其危险：lambda 对象本体**不在**协程帧里，
   `[x]() -> task<> {...}` 的临时 lambda 析构后，协程里再用 `x` 就是悬垂。
2. **重复销毁**：协程帧只能 `destroy()` 一次，所以 `generator` / `task`
   都是 move-only，移动后原对象句柄置空。
3. **忘记启动**：`initial_suspend = suspend_always` 意味着创建即挂起，
   `[[nodiscard]]` 提醒你别丢弃任务；顶层任务需要 `start()` 或被 `co_await`。
4. **在错误的时机取结果**：`task::result()` 只在完成后有效，
   `sync_wait` 里用断言防守。

## 8. 构建与运行

依赖：CMake ≥ 3.14、支持 C++20 协程的编译器（GCC 11+ / Clang 14+ /
AppleClang 13+ / MSVC 19.29+）、vcpkg（提供 fmt 与 Catch2）。

```sh
# 配置（用 vcpkg 工具链；开发者模式会启用测试）
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_MANIFEST_FEATURES=test \
  -Dstackless-coroutine_DEVELOPER_MODE=ON

# 编译
cmake --build build -j

# 运行演示
./build/stackless-coroutine

# 运行测试
ctest --test-dir build --output-on-failure
```

也可以使用仓库自带的 CMake preset：`cmake --preset=dev && cmake --build --preset=dev`
（细节见 [BUILDING.md](BUILDING.md) 与 [HACKING.md](HACKING.md)）。

## 9. 建议的阅读顺序

1. `source/coro/frame_stats.hpp` —— 先建立"协程帧"的物理直觉；
2. `source/coro/generator.hpp` —— 最小完整的 promise_type 协议；
3. `source/examples.cpp` 的 `fibonacci` + demo 1 —— co_yield 实战；
4. `source/coro/task.hpp` —— awaitable 协议、对称转移、惰性任务；
5. `source/coro/scheduler.hpp` + `ticker` —— 挂起的协程如何被外部事件唤醒；
6. 最后跑一遍测试，改改参数验证自己的理解。

---

贡献指南见 [CONTRIBUTING.md](CONTRIBUTING.md)，构建细节见 [BUILDING.md](BUILDING.md)。
