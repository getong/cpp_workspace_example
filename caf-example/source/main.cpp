// CAF (C++ Actor Framework) 功能演示程序入口。
//
// CAF_MAIN 宏负责:
// * 注册 types.hpp 中声明的自定义类型块(caf::id_block::caf_example)
// * 解析命令行参数/配置文件, 构造 actor_system
// * 调用下面的 caf_main(), 结束后干净地关闭调度器
//
// actor_system 析构时会等待所有仍存活的 actor 结束, 因此每个演示
// 都负责在返回前终止自己创建的 actor。

#include <caf/actor_system.hpp>
#include <caf/caf_main.hpp>
#include <caf/io/middleman.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

auto caf_main(caf::actor_system& sys) -> int
{
  // 依次运行全部演示; 每个函数独立展示 CAF 的一个特性
  demo::hello_world(sys);  // 01 spawn + 消息收发
  demo::typed_calculator(sys);  // 02 强类型 actor
  demo::stateful_counter(sys);  // 03 有状态 actor
  demo::request_then(sys);  // 04 异步 request/then
  demo::behavior_switching(sys);  // 05 become() 状态机
  demo::delayed_messages(sys);  // 06 定时消息 + 响应承诺
  demo::error_handling(sys);  // 07 错误传播
  demo::monitoring(sys);  // 08 监控与生命周期
  demo::custom_types(sys);  // 09 自定义消息类型
  demo::fan_out(sys);  // 10 并行分发/汇聚
  demo::data_flow(sys);  // 11 响应式数据流
  demo::ping_pong(sys);  // 12 纯异步乒乓协议
  demo::delegation(sys);  // 13 请求转交
  demo::game_room(sys);  // 14 综合: 游戏房间
  demo::distribution(sys);  // 15 网络分布(publish/remote_actor)
  sys.println("全部演示运行完毕。");
  return 0;
}

}  // namespace

// 生成 main(), 注册自定义类型 ID 块, 并加载 io 模块(middleman)
// 以启用网络分布能力(demo 15 的 publish/remote_actor 依赖它)
CAF_MAIN(caf::id_block::caf_example, caf::io::middleman)
