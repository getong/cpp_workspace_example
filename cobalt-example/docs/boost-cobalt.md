# boost::cobalt 说明

本项目通过 vcpkg 引入 [boost::cobalt](https://www.boost.org/libs/cobalt)，
在 `source/main.cpp` 中演示它的核心用法。

## cobalt 是什么

cobalt 是构建在 Boost.ASIO 之上的 **C++20 协程库**（Boost 1.84 起正式收录，
最低要求 Boost 1.82 + C++20）。它的目标是：让做大量 IO 的程序既不阻塞线程，
又能写成线性、可读的代码——用 `co_await` 代替回调地狱。

它的基本假设：

- 执行环境是单线程 `io_context`（同一时刻只有一个内核线程在其中运行），
  因此协程间通信不需要加锁，开销很低；
- 协程默认急切执行（eager execution）；
- 跨线程场景使用 `cobalt::thread` 单独处理。

## 并发模型：CSP，不是 actor

cobalt 采用 **CSP（Communicating Sequential Processes）模型**，与 Go 语言一致：

| | CSP（cobalt / Go） | Actor（Erlang / Akka / CAF） |
|---|---|---|
| 通信载体 | 匿名的、类型化的 **channel** | 每个 actor 自带的**邮箱** |
| 寻址方式 | 面向管道：谁持有 channel 谁能收发 | 面向身份：按 actor 地址发消息 |
| 多路等待 | `race`（等价 Go 的 `select`） | 邮箱模式匹配 |
| 耦合关系 | 发送方与接收方解耦，通过 channel 会合 | 发送方必须知道接收方是谁 |

判断依据：cobalt 的核心通信原语是 `channel<T>`（官方文档明确参照 Go 设计），
配套 `race` 做多路复用；它没有 actor 模型的要素——没有带地址的实体、
没有每实体一个邮箱、没有按身份收发消息。
所以在 cobalt 里写并发代码，最合适的方式就是 CSP 风格：
**协程之间不共享可变状态，通过 channel 通信来传递数据。**

## 主要组件

| 组件 | 作用 |
|---|---|
| `cobalt::main` / `run` | 程序入口，在单线程 `io_context` 上运行，自动把 `Ctrl+C` 转为协程取消 |
| `promise<T>` | 急切执行的协程：创建即开始运行 |
| `task<T>` | 惰性执行的协程：被 `co_await` 时才启动 |
| `generator<T>` | 可 `co_yield` 产出值序列的协程 |
| `channel<T>` | CSP 通信管道；缓冲为 0 时读写构成 rendezvous（会合），天然提供背压 |
| `race` | 等待多个 awaitable 中**先就绪的一个**（Go 的 `select`），对 channel 使用 `interrupt_await`，落选分支不丢消息 |
| `gather` / `join` | 等待**全部**完成：`gather` 逐个收集结果/异常，`join` 任一异常即整体失败 |
| `with` / `wait_group` | 结构化并发：作用域退出时保证所有子协程完成或被取消 |
| `as_result` / `as_tuple` | 把 `co_await` 的结果转为 `system::result` / `tuple`，用返回值代替异常处理错误 |

## 示例代码讲解（`source/main.cpp`）

### 示例 1：流水线（pipeline）

```text
produce(1..5) ──channel──> square(x*x) ──channel──> co_main 打印
```

- 每个阶段是一个独立协程，只通过 channel 与相邻阶段通信；
- 上游写完后 `close()`，关闭信号沿流水线逐级传播，各阶段自然退出；
- 读取端用 `co_await cobalt::as_result(ch.read())`：channel 关闭时返回
  错误值而不是抛 `broken_pipe` 异常（直接 `read()` 配合 `is_open()`
  轮询在关闭瞬间有竞态，会抛异常）；
- 默认 channel 缓冲为 0，写和读必须会合才能继续，天然形成背压
  （生产者不会跑到消费者前面去）。

### 示例 2：race 多路复用（Go 的 select）

```cpp
auto msg = co_await cobalt::race(fast.read(), slow.read());
```

- 两个 `ticker` 协程以不同周期向各自的 channel 发消息；
- 主协程用 `race` 同时等待两个 channel，谁先就绪就处理谁；
- `race` 返回 `variant2::variant`，索引对应就绪的分支；
- 关键保证：`race` 对 channel 走 `interrupt_await` 路径，
  落选分支挂起的读操作被安全中断，**不会丢失消息**。

## 构建

依赖通过 vcpkg manifest（`vcpkg.json`）管理：`boost-cobalt` + `fmt`。

```sh
# 普通构建（输出在 build/dev）
./build.sh

# 生成 compile_commands.json（含测试目标，供 clangd / emacs lsp 使用）
./compile_commands.sh

# 运行
./build/dev/cobalt-example
```

注意：`boost-cobalt` 的 vcpkg 端口在旧 baseline 中被标记为不支持 macOS，
本项目的 `builtin-baseline` 已更新到支持 macOS 的版本（boost 1.91.0）。
