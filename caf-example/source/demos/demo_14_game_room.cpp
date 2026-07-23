// 14 综合示例: 游戏房间 —— "一个玩家一个 actor" 的微型游戏服务器。
//
// 把前面演示过的构件拼在一起:
// * 玩家 = 有状态 actor: 私有状态(名字), 无锁 (demo 03)
// * 房间 = 有状态 actor: 持有成员列表, 逐个 send 实现聊天广播
// * monitor(player, 回调): 玩家 actor 终止(掉线)时自动移出并广播 (demo 08)
// * mail().delay(): 房间给自己发定时 tick, 模拟游戏世界时钟 (demo 06)
//
// 房间收到消息的顺序就是"服务器仲裁"的顺序: 多个玩家并发发言,
// 以房间 actor 处理的先后为准, 不需要任何锁。

#include <chrono>
#include <map>
#include <string>
#include <thread>

#include <caf/actor_from_state.hpp>
#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

using namespace std::literals;

// ---- 玩家 actor -----------------------------------------------------------
// 每个在线玩家对应一个这样的 actor: 收到 say 就向房间发言,
// 收到房间的广播就打印(真实服务器里这一步是写回网络连接)。
auto player_impl(caf::event_based_actor* self,
                 const std::string& name,
                 const caf::actor& room) -> caf::behavior
{
  // 出生即进房: 把自己的句柄交给房间
  self->mail(demo::join_atom_v, name, caf::actor_cast<caf::actor>(self))
      .send(room);
  return {
      [self, name, room](demo::say_atom, const std::string& text)
      { self->mail(demo::chat_atom_v, name, text).send(room); },
      [self, name](
          demo::chat_atom, const std::string& from, const std::string& text)
      { self->println("    [{} 的客户端] {}: {}", name, from, text); },
  };
}

// ---- 房间 actor -----------------------------------------------------------
struct room_state
{
  explicit room_state(caf::event_based_actor* self_ptr)
      : self(self_ptr)
  {
  }

  // 广播 = 遍历成员逐个发送; 对每个接收方消息保持有序
  void broadcast(const std::string& from, const std::string& text)
  {
    for (auto& [name, player] : members) {
      self->mail(demo::chat_atom_v, from, text).send(player);
    }
  }

  auto make_behavior() -> caf::behavior
  {
    return {
        [this](
            demo::join_atom, const std::string& name, const caf::actor& player)
        {
          members.emplace(name, player);
          // 掉线感知: 玩家 actor 终止时回调触发, 自动移出并通知全房间
          self->monitor(player,
                        [this, name](const caf::error& /*reason*/)
                        {
                          members.erase(name);
                          broadcast("系统",
                                    name + " 掉线, 已移出房间 (在线 "
                                        + std::to_string(members.size())
                                        + " 人)");
                        });
          broadcast("系统", name + " 进入了房间");
        },
        [this](
            demo::chat_atom, const std::string& from, const std::string& text)
        { broadcast(from, text); },
        [this](demo::start_atom, int32_t rounds)
        {
          ticks_left = rounds;
          self->mail(demo::tick_atom_v)
              .delay(150ms)
              .send(caf::actor_cast<caf::actor>(self));
        },
        [this](demo::tick_atom)
        {
          --ticks_left;
          broadcast("世界时钟", "第 " + std::to_string(tick_id++) + " 次刷新");
          if (ticks_left > 0) {
            self->mail(demo::tick_atom_v)
                .delay(150ms)
                .send(caf::actor_cast<caf::actor>(self));
          }
        },
    };
  }

  caf::event_based_actor* self;
  std::map<std::string, caf::actor> members;
  int32_t ticks_left = 0;
  int32_t tick_id = 1;
};

}  // namespace

namespace demo
{

void game_room(caf::actor_system& sys)
{
  sys.println("=== 14 game_room: 一个玩家一个 actor 的游戏房间 ===");
  auto room = sys.spawn(caf::actor_from_state<room_state>);
  // 三位玩家上线, 各自是独立的 actor
  auto xiaoming = sys.spawn(player_impl, "小明", room);
  auto xiaohong = sys.spawn(player_impl, "小红", room);
  auto xiaogang = sys.spawn(player_impl, "小刚", room);
  caf::scoped_actor self {sys};
  // 演示脚本是异步驱动的, 用短暂 sleep 分隔各阶段、让输出成组
  auto pause = [](std::chrono::milliseconds dur)
  { std::this_thread::sleep_for(dur); };
  pause(100ms);
  self->mail(demo::say_atom_v, std::string {"大家好, 开黑吗?"}).send(xiaoming);
  pause(100ms);
  // 世界时钟: 房间广播 2 次定时 tick
  self->mail(demo::start_atom_v, int32_t {2}).send(room);
  pause(400ms);
  // 小刚掉线(终止他的 actor); 房间的 monitor 回调会自动善后
  self->send_exit(xiaogang, caf::exit_reason::user_shutdown);
  pause(100ms);
  self->mail(demo::say_atom_v, std::string {"小刚怎么掉了..."}).send(xiaohong);
  pause(100ms);
  // 散场
  self->send_exit(room, caf::exit_reason::user_shutdown);
  self->send_exit(xiaoming, caf::exit_reason::user_shutdown);
  self->send_exit(xiaohong, caf::exit_reason::user_shutdown);
}

}  // namespace demo
