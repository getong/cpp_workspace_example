// 01 最小示例 —— CAF 的三个最基本概念:
//
// * actor_system: 运行时环境, 管理调度器/线程池/actor 注册表
// * spawn: 创建 actor。函数式风格里, actor 就是一个返回 behavior 的函数
// * behavior: 一组消息处理器(lambda), 定义"收到什么消息 -> 做什么"
//
// actor 之间不共享内存, 只通过消息通信, 因此天然避免了数据竞争。

#include <chrono>
#include <string>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"

namespace
{

// 一个"打招呼" actor: 收到 string 就返回一句问候。
// 返回值会自动作为应答消息发送给请求方。
auto greeter_impl() -> caf::behavior
{
  return {
      [](const std::string& name) -> std::string
      { return "你好, " + name + "!"; },
  };
}

}  // namespace

namespace demo
{

void hello_world(caf::actor_system& sys)
{
  sys.println("=== 01 hello_world: spawn + 消息收发 ===");
  // 创建 actor, 得到一个轻量的句柄(handle), 句柄可以随意拷贝、传递
  auto greeter = sys.spawn(greeter_impl);
  // scoped_actor 是一个"寄生"在当前线程里的阻塞式 actor,
  // 常用于在普通线程(如 main)里与 actor 世界交互
  caf::scoped_actor self {sys};
  // mail(..).request(..) 发起请求, receive 阻塞等待应答或错误
  self->mail("CAF")
      .request(greeter, std::chrono::seconds(1))
      .receive([&self](const std::string& reply)
               { self->println("  收到应答: {}", reply); },
               [&self](const caf::error& err)
               { self->println("  请求失败: {}", caf::to_string(err)); });
  // 演示结束, 通知 greeter 退出(否则 actor_system 析构时会一直等它)
  self->send_exit(greeter, caf::exit_reason::user_shutdown);
}

}  // namespace demo
