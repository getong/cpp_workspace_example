// 05 行为切换 —— actor 是天然的状态机:
//
// * become() 用一组新的消息处理器替换当前行为
// * 同一个消息(比如 status)在不同状态下可以有不同的处理方式
// * 相比手写 switch/enum 状态机, 每个状态的可响应消息集合一目了然
//
// 本例实现一扇门: 关闭状态只接受"开门", 打开状态只接受"关门"。

#include <chrono>
#include <string>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

auto door_open(caf::event_based_actor* self) -> caf::behavior;

// 状态一: 门是关的
auto door_closed(caf::event_based_actor* self) -> caf::behavior
{
  return {
      [self](demo::open_atom) -> std::string
      {
        self->become(door_open(self));  // 切换到"打开"状态
        return "吱呀~ 门开了";
      },
      [](demo::status_atom) -> std::string { return "门当前是: 关闭"; },
  };
}

// 状态二: 门是开的
auto door_open(caf::event_based_actor* self) -> caf::behavior
{
  return {
      [self](demo::close_atom) -> std::string
      {
        self->become(door_closed(self));  // 切换回"关闭"状态
        return "咔哒~ 门关了";
      },
      [](demo::status_atom) -> std::string { return "门当前是: 打开"; },
  };
}

}  // namespace

namespace demo
{

void behavior_switching(caf::actor_system& sys)
{
  sys.println("=== 05 behavior_switching: become() 实现状态机 ===");
  auto door = sys.spawn(door_closed);
  caf::scoped_actor self {sys};
  auto tell = [&self, &door](auto atom)
  {
    self->mail(atom)
        .request(door, std::chrono::seconds(1))
        .receive([&self](const std::string& reply)
                 { self->println("  {}", reply); },
                 [&self](const caf::error& err)
                 { self->println("  出错: {}", caf::to_string(err)); });
  };
  tell(demo::status_atom_v);  // 关闭
  tell(demo::open_atom_v);  // 开门 -> become(door_open)
  tell(demo::status_atom_v);  // 打开
  tell(demo::close_atom_v);  // 关门 -> become(door_closed)
  tell(demo::status_atom_v);  // 关闭
  self->send_exit(door, caf::exit_reason::user_shutdown);
}

}  // namespace demo
