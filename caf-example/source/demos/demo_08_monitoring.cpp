// 08 监控与生命周期 —— 感知其他 actor 的死亡:
//
// * monitor(who): 单向监控。被监控的 actor 终止时, 监控者收到 down_msg,
//   其中携带终止原因(正常退出/错误/被杀)
// * send_exit(): 向 actor 发送 exit 消息, 默认行为是令其终止
// * 这是构建"监督树"(supervision)的基础: 父 actor 监控子 actor,
//   发现异常退出后重启或转移工作, 实现容错

#include <string>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// 一个干完活就正常退出的临时工
void temp_worker_impl(caf::event_based_actor* self)
{
  self->println("  临时工: 完成任务, 正常下班");
  self->quit();  // 正常终止, 原因是 exit_reason::normal
}

// 一个一直待命、直到被 send_exit 终止的常驻工
auto resident_worker_impl() -> caf::behavior
{
  return {
      [](const std::string& task) { return "完成: " + task; },
  };
}

}  // namespace

namespace demo
{

void monitoring(caf::actor_system& sys)
{
  sys.println("=== 08 monitoring: down_msg 感知 actor 终止 ===");
  caf::scoped_actor self {sys};
  // 情形一: 监控一个正常退出的 actor
  auto temp = sys.spawn(temp_worker_impl);
  self->monitor(temp);
  self->receive(
      [&self](const caf::down_msg& down)
      {
        self->println("  监控者: 临时工已终止, 原因: {}",
                      caf::to_string(down.reason));
      });
  // 情形二: 用 send_exit 终止一个常驻 actor, 同样会收到 down_msg
  auto resident = sys.spawn(resident_worker_impl);
  self->monitor(resident);
  self->send_exit(resident, caf::exit_reason::user_shutdown);
  self->receive(
      [&self](const caf::down_msg& down)
      {
        self->println("  监控者: 常驻工已终止, 原因: {}",
                      caf::to_string(down.reason));
      });
}

}  // namespace demo
