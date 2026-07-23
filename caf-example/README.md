# caf-example

This is the caf-example project.

演示 [CAF (C++ Actor Framework)](https://www.actor-framework.org/) 核心功能的示例集。
每个特性对应 `source/demos/` 下的一个独立源文件/函数, 由 `source/main.cpp` 依次运行:

| # | 文件 | 演示的 CAF 功能 |
|---|------|----------------|
| 01 | `demo_01_hello_world.cpp` | `actor_system` / `spawn` / `behavior` / `scoped_actor` 消息收发 |
| 02 | `demo_02_typed_calculator.cpp` | `typed_actor`: 编译期检查的消息协议 |
| 03 | `demo_03_stateful_counter.cpp` | `actor_from_state`: 无锁的私有状态 |
| 04 | `demo_04_request_then.cpp` | `request(..).then(..)`: 事件驱动的非阻塞请求 |
| 05 | `demo_05_behavior_switching.cpp` | `become()`: 用行为切换实现状态机 |
| 06 | `demo_06_delayed_messages.cpp` | `mail(..).delay(..)` 定时消息 + `response_promise` 延迟应答 |
| 07 | `demo_07_error_handling.cpp` | `caf::result<T>` / `caf::error`: 错误沿消息流传播 |
| 08 | `demo_08_monitoring.cpp` | `monitor` / `down_msg` / `send_exit`: 生命周期与容错基础 |
| 09 | `demo_09_custom_types.cpp` | 类型 ID 块 + `inspect()`: 自定义 struct 作为消息 |
| 10 | `demo_10_fan_out.cpp` | `fan_out_request<select_all>`: 并行分发/汇聚 (map-reduce) |
| 11 | `demo_11_data_flow.cpp` | flow API (observable): 响应式数据流水线 |
| 12 | `demo_12_ping_pong.cpp` | 纯异步消息驱动的 actor 协议 |
| 13 | `demo_13_delegation.cpp` | `delegate()`: 把请求透明转交给后端 actor |
| 14 | `demo_14_game_room.cpp` | 综合示例: 游戏房间(一个玩家一个 actor, 加入/聊天广播/定时 tick/掉线移除) |

构建后直接运行 `caf-example` 可执行文件即可看到全部演示的输出。

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

<!--
Please go to https://choosealicense.com/licenses/ and choose a license that
fits your needs. The recommended license for a project of this type is the
GNU AGPLv3.
-->
