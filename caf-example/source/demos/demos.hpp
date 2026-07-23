#pragma once

// CAF (C++ Actor Framework) 功能演示合集。
//
// 每个函数演示 CAF 的一个核心特性, 各自独立、可单独阅读,
// 由 main.cpp 依次调用。所有演示都在函数返回前结束自己创建的 actor,
// 因此输出是按顺序分组的。

#include <caf/fwd.hpp>

namespace demo
{

/// 01 最小示例: spawn 一个 actor, 发送消息并接收回复
void hello_world(caf::actor_system& sys);

/// 02 强类型 actor: 用 typed_actor 声明消息协议, 编译期检查收发双方
void typed_calculator(caf::actor_system& sys);

/// 03 有状态 actor: actor_from_state 把"状态 + 行为"封装在一个类里,
///    状态只属于这个 actor, 无锁也无数据竞争
void stateful_counter(caf::actor_system& sys);

/// 04 异步 request/then: 事件驱动 actor 之间非阻塞的请求-应答
void request_then(caf::actor_system& sys);

/// 05 行为切换: 通过 become() 实现状态机(门的开/关)
void behavior_switching(caf::actor_system& sys);

/// 06 定时消息 + 响应承诺: mail().delay() 定时发送,
///    response_promise 延迟应答(火箭倒计时)
void delayed_messages(caf::actor_system& sys);

/// 07 错误处理: 用 caf::result<T> 返回 caf::error, 错误自动传播给请求方
void error_handling(caf::actor_system& sys);

/// 08 监控与生命周期: monitor 另一个 actor, 在它终止时收到 down_msg
void monitoring(caf::actor_system& sys);

/// 09 自定义消息类型: 传递带 inspect() 的 struct(订单 -> 回执)
void custom_types(caf::actor_system& sys);

/// 10 并行分发/汇聚: fan_out_request 把任务广播给一组 worker,
///    并收集全部结果(map-reduce 风格)
void fan_out(caf::actor_system& sys);

/// 11 数据流: 用 flow API(observable)在 actor 内做响应式流水线
void data_flow(caf::actor_system& sys);

/// 12 乒乓协议: 两个 actor 互发消息若干回合后自行退出
void ping_pong(caf::actor_system& sys);

/// 13 请求转交: delegate() 把请求转给后端 actor, 由后端直接答复客户端
void delegation(caf::actor_system& sys);

/// 14 综合示例: 游戏房间 —— 一个房间 actor + 若干玩家 actor,
///    演示加入/聊天广播/定时 tick/掉线自动移除
void game_room(caf::actor_system& sys);

}  // namespace demo
