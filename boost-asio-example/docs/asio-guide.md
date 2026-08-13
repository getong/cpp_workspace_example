# Boost.Asio 模块讲解

本文档按模块讲解本项目演示的 Boost.Asio 核心概念。每个模块都可以单独运行：

```sh
./build/dev/boost-asio-example <模块名>
```

## 核心概念速览

在读各模块之前，先建立几个贯穿全局的概念：

- **`io_context`（事件循环）**：Asio 的调度核心。所有异步操作完成后，其回调
  都由某个正在执行 `io_context::run()` 的线程调用。`run()` 一直运行到没有
  未完成的工作才返回——这也是各模块示例能自然退出的原因。
- **执行器（executor）**：描述"处理器在哪里、以什么规则执行"的轻量句柄。
  `io_context`、`thread_pool`、`strand` 都能提供执行器。
- **完成处理器（completion handler）**：异步操作完成时被调用的回调。Asio 的
  统一签名习惯是第一个参数为 `boost::system::error_code`。
- **完成令牌（completion token）**：决定异步操作以什么形式交付结果。传入
  lambda 就是回调风格；传入 `use_awaitable` 就变成可 `co_await` 的协程风格。
  同一个 `async_read`，两种风格都能用——这是 Asio 通用异步模型的关键设计。

## timer — 定时器

`steady_timer` 基于单调时钟（不受系统改时间影响），演示四种用法：

1. **同步等待**：`timer.wait()` 阻塞当前线程，不需要事件循环。
2. **异步等待**：`timer.async_wait(回调)` 立即返回，到期后回调在
   `run()` 所在线程执行。
3. **周期定时**：回调里用 `expires_at(expiry() + 周期)` 重新武装自己。
   相比 `expires_after(周期)`，以"上次到期点"为基准不会累积回调本身的
   执行耗时，长期运行不漂移。
4. **取消**：`cancel()` 让未到期的等待立即完成，回调收到
   `operation_aborted` 错误码，可据此区分"到期"与"被取消"。

注意输出顺序：三个异步等待按各自的到期时间交错完成，与登记顺序无关。

## post_dispatch — 向执行器提交工作

三个提交函数的差别只在"何时执行"：

| 函数 | 语义 |
|---|---|
| `post` | 无条件排队，绝不在调用处执行 |
| `dispatch` | 若当前线程正运行着目标执行器，就地同步执行；否则排队 |
| `defer` | 同 `post`，但提示执行器"这是当前工作的延续"，允许调度优化 |

示例的输出编号 1→5 展示了：在处理器内部 `dispatch` 会立刻内联执行（编号 2
先于外层处理器返回的编号 3），而 `post`/`defer` 一定排到外层处理器返回之后。

`dispatch` 的就地执行是把双刃剑：省一次排队，但可能造成意外的递归和重入，
不确定时用 `post`。

## strand — 无锁串行化

**strand 是 Asio 世界的互斥手段**。它包装一个底层执行器，保证经它提交的
处理器串行执行（任意时刻最多一个在运行），即使底层是多线程 `thread_pool`。

示例向 4 线程池经 strand 提交 10000 个 `++counter`，共享的普通 `int` 不加锁
也不丢更新；对照组直接 post 到线程池则必须用 `std::atomic`。输出还展示了
strand 的处理器实际分布在多个池线程上——串行 ≠ 固定在一个线程。

典型用途：一个连接（socket）的所有处理器挂在同一个 strand 上，连接内天然
串行、连接间充分并行，整个服务器不出现一把显式的锁。

## coroutine — C++20 协程

`awaitable<T>` 是 Asio 的协程返回类型。`co_await` 一个异步操作时协程挂起，
线程可以去跑别的处理器；操作完成后事件循环恢复协程——**异步代码写成了
同步的样子**。

- `co_spawn(执行器, 协程, detached)`：把协程作为顶层任务挂到执行器上，
  `detached` 表示不关心返回值与异常。
- `co_await boost::asio::this_coro::executor`：取得当前协程绑定的执行器，
  用它构造 timer/socket，保证回调回到同一执行上下文。
- **`||` 竞速**：`co_await (fast() || slow())` 谁先完成谁胜出，另一个被
  自动取消。这是给任意异步操作加超时的惯用写法（一边真操作、一边定时器）。
- **`&&` 并联**：两个都完成才返回，结果打包成 tuple，耗时取决于较慢者。

## echo_tcp — 协程版 TCP echo

服务端与客户端两个协程跑在**同一个单线程事件循环**上，互不阻塞：

- 服务端绑定 `127.0.0.1:0`——0 号端口由系统分配空闲端口，示例不会与任何
  程序冲突；`local_endpoint()` 取回实际端口告诉客户端。
- `async_read_some`：读到任意长度就返回，适合"来多少回多少"的 echo；
  `async_write` 则保证写完整个缓冲区才完成（内部循环处理短写）。
- 客户端用 `async_read` 读满与消息等长的缓冲区，即完整回显。
- 客户端 socket 析构关闭连接，服务端读到 EOF（以 `system_error` 异常抛出，
  错误码 `eof`）结束会话，事件循环随之自然退出。

真实服务器的扩展方式：在循环里 `async_accept`，对每个连接
`co_spawn(echo_session)`，即可并发服务多个客户端。

## udp_echo — 回调版 UDP echo

与 echo_tcp 形成两点对照：

1. **回调风格 vs 协程风格**：链式异步在回调风格下写成"回调里发起下一步"，
   嵌套层次随步骤增加而加深（回调地狱的雏形）——对比协程版的平铺直叙。
2. **UDP vs TCP**：UDP 无连接，收发都带对端地址
   （`async_receive_from` / `async_send_to`，即 recvfrom/sendto 语义），
   一个数据报要么完整到达要么丢失，没有字节流的"短读"问题。

## resolver — 异步域名解析

名字解析（`getaddrinfo`）可能阻塞很久，Asio 把它也纳入异步模型：
`async_resolve` 在内部线程做解析，完成后把结果投递回事件循环。

结果是一组 endpoint（`localhost` 通常同时解析出 `::1` 和 `127.0.0.1`）。
实际连接时通常配合 `async_connect(socket, results, token)` 依次尝试每个
endpoint 直到成功。示例解析 `localhost`，离线可运行。

## Actor 模型系列（Erlang 概念对照）

`actor_*` 六个模块把 Erlang 的并发模型逐个映射到 asio 协程上。
基础对应关系贯穿全系列：

| Erlang | 本项目 |
|---|---|
| 进程 (process) | 协程 `awaitable<void>` |
| 邮箱 (mailbox) | `asio::experimental::channel` |
| Pid（进程句柄） | `std::shared_ptr<channel>`（可复制、可放进消息转发） |
| `spawn(Fun)` | `co_spawn(executor, coro, detached)` |
| `Pid ! Msg` | `co_await mbox->async_send(...)` |
| `receive ... end` | `co_await mbox->async_receive(...)` |
| 不同形状的消息元组 | `std::variant` + `std::get_if` |

actor 模型的两条根本规则由此自动成立：**actor 之间不共享状态**（只通过
channel 传消息），**每个 actor 顺序处理自己的邮箱**（一个协程一次只
处理一条消息）。于是所有 `actor_*` 示例里没有一把锁。

### actor_pingpong — 消息往返

Erlang 教程的第一课。ping 主动发起、pong 应答，往复三轮后 ping 发
`stop` 通知对方退出。看点：消息用 `std::variant<ping_msg, pong_msg,
stop_msg>` 建模；两个"进程"实际跑在同一个单线程事件循环上。

### actor_genserver — call 与 cast

gen_server 的两种消息模式：

- **cast**（`increment`）：单向投递，发完即走；
- **call**（`get_value`）：请求消息里带上"回信地址"——一个容量为 1 的
  一次性 reply channel，调用方发出请求后挂起等回执。Erlang 里的回信
  地址是调用方 Pid + 唯一引用，手法完全一致。

两个客户端并发地各自增 100 次，最终恰好 200——counter 只被 server
一个协程读写，mailbox 就是同步点。

### actor_supervisor — let it crash

Erlang 哲学：worker 不写防御代码，出错直接崩溃，由 supervisor 按策略
重启。映射关键点：`co_spawn(..., use_awaitable)` 时子协程的异常会在
`co_await` 处重新抛出——等价于 monitor 收到 `{'DOWN', Reason}` 消息。
supervisor 在循环里 catch、计数、退避、重启（one_for_one 策略 +
max_restarts 上限）。

语言细节：C++ 不允许在 catch 块内 `co_await`，所以 catch 里只记录
崩溃原因，重启延迟放在 try/catch 之外等。

### actor_pipeline — 背压与关停传播

producer → square → sink 三级流水线，用容量为 2 的有界 channel 串联。
两个"自动"性质：

1. **背压**：下游慢时 `async_send` 挂起上游，producer 只领先一个缓冲区
   的距离。Erlang 邮箱无界、过载需另行兜底；有界 channel 把保护内建了。
2. **关停传播**：上游 `close()` 输出通道，下游 receive 得到
   `channel_closed` 后关闭再下一级，EOF 逐级级联。

### actor_pool — 工作者池

fan-out / fan-in：三个 worker 从同一条 jobs 队列领活，结果汇入同一条
results 队列。空闲 worker 挂起在 `async_receive` 上，谁先空闲谁领活——
负载均衡是队列语义自带的。dispatcher 发完任务后 `close(jobs)`，
worker 领不到活自然下班（drain-then-stop）。

### actor_chat — 每个连接一个 actor

Erlang 网络服务的标志性架构，也是本系列的综合应用：

- accept 循环对每个新连接 `co_spawn` 一个 session actor（Erlang：每个
  accept 后 spawn 一个连接进程）；
- room actor 独占成员表，处理入会/退会/广播——成员表无锁，因为只有
  room 一个协程能碰它；
- session 内部再分读写两个子协程（`&&` 结构化并发）：reader 把 socket
  的行转成消息发给 room，writer 把自己 inbox 里的广播写回 socket；
- 退会流程展示了 actor 间的优雅关停：reader 读到 EOF → 向 room 发
  `leave` → room 关闭该成员 inbox → writer 退出 → session 结束。

示例内置 alice/bob 两个回环客户端演一段对话，每步都等字节真正到达
后才继续，输出是确定的。

一个值得记住的坑：`std::cout << "x" << co_await f()` 会先输出前缀、
在表达式中途挂起，其他协程的输出会插进来。协程里要"先 await 到变量，
再一次性打印"。

## CSP 系列（Go 风格）：C++ 的 CSP 编程模型综述

### CSP 是什么

CSP（Communicating Sequential Processes，Hoare 1978）与 actor 是并发
建模的两大流派，Go 的 goroutine + channel 就是 CSP 的工程化。核心口号
是 Go 谚语："**不要通过共享内存来通信，要通过通信来共享内存**"——
数据的所有权随消息在进程间移交，于是不需要锁。

CSP 与 actor 的分野在"中心"不同：

| | Actor（Erlang） | CSP（Go） |
|---|---|---|
| 第一等公民 | 进程（Pid） | 信道（channel） |
| 消息发给谁 | 指名道姓发给某个 Pid | 匿名——只认 channel 不认对方 |
| 收发耦合 | 邮箱属于进程，一对一收 | channel 独立于进程，可多发多收 |
| 缓冲 | 邮箱无界、异步 | 无缓冲（会合）或有界缓冲 |
| 多路等待 | selective receive（按模式挑消息） | select（按 channel 挑分支） |

两者在本项目里**同根同源**：`asio::experimental::channel` 既能当
actor 的私有邮箱（`actor_*` 系列），也能当 CSP 的共享信道（`csp_*`
系列）——区别只是用法约定，这也说明两种模型可以在同一程序里混用。

### Go ↔ C++（asio 协程）对照表

| Go | 本项目 |
|---|---|
| `go f(ch)` | `co_spawn(executor, f(chan), detached)` |
| `make(chan T)`（无缓冲） | `make_shared<channel>(ex, 0)` |
| `make(chan T, n)`（有缓冲） | `make_shared<channel>(ex, n)` |
| `ch <- v` | `co_await ch->async_send({}, v, use_awaitable)` |
| `v := <-ch` | `co_await ch->async_receive(use_awaitable)` |
| `close(ch)` | `ch->close()` |
| `for v := range ch` | 循环 receive 直到 `channel_closed` |
| `select { case ... }` | `co_await (a.async_receive(...) \|\| b.async_receive(...))` |
| `case <-time.After(d)` | `\|\| timer.async_wait(...)` |
| `<-done` / `ctx.Done()` | 等一条 `channel<void(error_code)>`，`close` 即广播 |
| `sync.WaitGroup` + `Wait()` | `co_await (a() && b())`（结构化并发，还不会泄漏） |

goroutine 与这里的协程有一个实现差异值得知道：goroutine 由 Go 运行时
抢占式调度到多个线程上；`co_spawn` 的协程默认协作式地跑在你给的
executor 上（本项目都是单线程 `io_context`，因此示例输出确定）。要用
多核，换 `thread_pool` 执行器并给共享 channel 换 `concurrent_channel`
即可，代码形状不变。

### csp_channel — channel 三要素

1. **无缓冲 = 会合（rendezvous）**：send 挂起直到 receive 到场，数据
   当面交接。输出里 sender 等了 30ms 才完成 send——**通信同时就是
   同步**，这是 CSP 的灵魂，也是它和 actor 异步邮箱最大的语义差别。
2. **有缓冲**：容量 3 时前 3 个 send 立即返回，第 4 个挂起等消费者
   腾位——缓冲解耦节奏，容量就是背压阈值。
3. **close + range**：生产者 close 宣告"没有了"；消费者 receive 到
   `channel_closed` 结束循环，缓冲区里的存货仍会先取完。

### csp_duo — 两个协程互相通信的作用与效果

四个最小示例，每个都恰好是**两个 `co_spawn` 出来的顶层协程**通过
channel 对话，分别回答"双协程通信能得到什么"：

1. **交替打印 —— 通信就是同步**。经典题"两个执行流交替打印数字"
   用线程做要 mutex + condition_variable + 共享 flag 三件套；这里
   一根"接力棒" channel 就够：数字本身是棒，收到才有打印权，打完
   递回去。send 同时完成了"解锁 + 通知"，没有共享变量，也就没有
   竞争、没有虚假唤醒。结束信号也是 channel 语义：`close()` 即散场。
2. **全双工对话 —— 状态封装**。requests/responses 两根单向 channel
   组成进程内的一对"微服务"。累加器 `total` 是 calculator 协程帧里
   的局部变量，外界物理上无法触碰，user 只能发消息问、收消息知——
   "通过通信来共享内存"的字面演示：内存本身从不共享。
3. **所有权移交 —— 数据免竞争**。channel 能传 move-only 类型：
   `unique_ptr<vector<int>>`（一百万个 int）经 channel **移动**过去，
   两端打印的 buffer 地址相同（零拷贝），发送方指针变 null。数据
   自始至终只有一个持有者，数据竞争在类型系统层面就不可能发生。
4. **反向流控 —— 控制流逆着数据流走**。数据由 producer 流向
   consumer，而"要多少"的配额（credit）由 consumer 逆向授予，
   producer 只在有配额时生产。两根方向相反的 channel 构成闭环，
   生产节奏完全由消费方决定——Reactive Streams `request(n)` 背压
   协议的最小实现。

共同的底层机制：`co_await send/receive` 挂起的是协程而不是线程，
两个协程在同一个单线程 `io_context` 上交错推进，每次 channel 会合
都是一次确定的控制权交接——所以示例输出可逐字节复现。

### csp_select — 多路等待

`awaitable_operators` 的 `||` 近似 Go 的 `select`：谁先就绪走谁的
分支，用返回的 `variant::index()` 区分。三个惯用法：

- **双 channel select 循环**：tick/tock 两个节奏的生产者，select 按
  到达顺序处理；
- **超时分支**：`|| timer.async_wait(...)` 就是 `case <-time.After`；
- **done channel**：等待者都挂在 `<-done` 上，一次 `close(done)` 唤醒
  全部——Go 取消广播（`ctx.Done()`）的惯用法。

诚实声明：`||` 在胜者完成后取消败者，**不是 Go 那种原子多路等待**。
对"接收方先挂起、发送方后到"的无缓冲会合场景，被取消的 receive 尚未
匹配任何发送者，不丢数据（本模块都属此类）；若 channel 有缓冲且已
囤货，两路可能同时就绪，`||` 会取消一个已完成的接收而丢值。需要
严格 select 语义时应让所有参与 select 的 channel 无缓冲，或使用专门
的 select 实现。

### csp_fanin — fan-out / fan-in

Go 官方 "Concurrency Patterns" 的经典组合：多个 worker 从同一条 jobs
channel 抢活（fan-out），结果汇入同一条 merged channel（fan-in）。
fan-in 的经典难题是"谁来 close 出口、什么时候 close"——答案是 Go 的
`go func() { wg.Wait(); close(merged) }()`，这里直译为：

```cpp
co_await (worker(1, ...) && worker(2, ...));  // && 就是 WaitGroup
merged->close();
```

而且 `&&` 是结构化并发：作用域结束协程必然收尾，没有 goroutine 泄漏
问题。

### csp_mpsc — MPSC channel 的功能与作用

MPSC（Multi-Producer Single-Consumer）：多个生产者共享**同一根**
channel，消息汇聚到唯一的消费者。四点要义：

1. **汇聚**：把多个并发来源的事件合并成一条串行流。消费者一次只
   处理一条消息，处理逻辑不需要任何同步——回头看 actor 系列就会
   发现，**actor 的 mailbox 本质上就是一根 MPSC channel**（任何人
   都能给一个 actor 发消息，只有它自己收）。
2. **替代 mutex + queue + condition_variable 三件套**：传统工作队列/
   日志队列要手写加锁入队、条件变量唤醒、解锁出队；MPSC channel 把
   入队（自带背压）、唤醒、出队封装成两个 await 点。
3. **序保证要说准**：每个生产者各自的消息保持 FIFO（p1 的 #2 绝不会
   先于 p1 的 #1 到达），不同生产者之间按实际到达时间交错，**没有
   全局定序**。demo 2 对 4 线程 × 250 条消息逐条验证了这一点
   （violations: 0）。需要全局序时由消费者按业务字段（时间戳/序号）
   重排。
4. **关停协议是 MPSC 特有难题**：channel 是共享的，任何一个生产者都
   无权擅自 close。两种惯用解法各演示一个——demo 1 用
   join-then-close（`&&` 等全体生产者收工，由"监工"协程统一 close）；
   demo 2 用计数消费者（预期总量已知，收满即止，无须 close）。

线程模型的分界线：单线程/同一 strand 上用普通 `channel` 即可
（demo 1）；生产者分布在多个线程时必须换 **`concurrent_channel`**
（demo 2：4 个生产者协程跑在 thread_pool 上真并行，消费者跑在主线程
io_context 上）。concurrent_channel 内部保证线程安全，成为并行世界与
串行世界之间的唯一衔接点——日志收集器、事件总线、指标上报的标准形态。

顺带说明：asio 的 channel 本身不限制端点数量（MPMC 皆可），
SPSC / MPSC / MPMC 只是使用约定——`csp_duo` 是 SPSC，本模块是 MPSC，
`actor_pool` 的 jobs 队列是 SPMC（一个派活的、多个抢活的）。

### csp_sieve — 并发素数筛（CSP 招牌）

一条**运行期动态生长**的进程链：

```
generate(2,3,4,...) → filter(2) → filter(3) → filter(5) → ... → main
```

main 每从链尾读到一个数，它必然是素数（合数都被沿途的 filter 滤掉
了），于是为它孵化一级新 filter 接到链尾。10 个素数 = 10 级筛子进程。
"进程和 channel 都是可以随手创建的轻量值"——这是 CSP/Go 风格区别于
线程模型的根本体感。Go 原版靠进程退出兜底 goroutine 泄漏，这里补全
了关停协议：close 沿链级联，所有协程干净退出。

### CSP 与 Actor 如何选

- 要**请求-应答、按对象建模、监督重启**（服务器里的"会话"、"账户"）：
  actor 更顺手——状态和进程绑定，Pid 就是对象引用。
- 要**数据流、流水线、扇入扇出、多路复用**：CSP 更顺手——channel
  本身就是数据流的管道接头，select 是复用器。
- 实践中经常混用：例如 `actor_chat` 的骨架是 actor 的（room/session
  各管各的状态），而 session 内部 reader/writer 的连接方式就是 CSP 的。

## 两种风格如何选择

| | 回调风格 | 协程风格 |
|---|---|---|
| 代码形态 | 回调嵌套，控制流被切碎 | 顺序书写，控制流完整 |
| 错误处理 | 每个回调检查 `error_code` | `try/catch` 或 `as_tuple` 令牌 |
| 组合能力 | 手工串联 | `\|\|` / `&&` 直接组合、自动取消 |
| 依赖 | C++11 即可 | C++20 |

新代码建议默认协程风格；回调风格用于理解底层模型，以及维护存量代码。
