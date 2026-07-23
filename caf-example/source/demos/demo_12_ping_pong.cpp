// 12 乒乓协议 —— actor 之间纯异步的来回通信:
//
// * 两个 actor 只靠"发后不管"的异步消息互相驱动, 没有任何 request/阻塞
// * current_sender() 可以拿到当前消息的发送者, 用于回信
// * 打满回合数后, ping 方通知双方退出 —— 协议自我终结
//
// 这是 actor 系统的经典入门基准(Erlang 同款), 也直观展示了
// "计算由消息驱动"的编程模型。

#include <cstdint>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// pong 方: 收到 ping 就原样回一个 pong 给发送者
auto pong_impl(caf::event_based_actor* self) -> caf::behavior
{
  return {
      [self](demo::ping_atom, int32_t round)
      {
        self->println("  pong <- 第 {} 回合", round);
        auto sender = caf::actor_cast<caf::actor>(self->current_sender());
        self->mail(demo::pong_atom_v, round).send(sender);
      },
  };
}

// ping 方: 发出第一拍, 之后每收到一个 pong 就开启下一回合
auto ping_impl(caf::event_based_actor* self,
               const caf::actor& pong,
               int32_t max_rounds) -> caf::behavior
{
  self->mail(demo::ping_atom_v, int32_t {1}).send(pong);  // 开球
  return {
      [self, pong, max_rounds](demo::pong_atom, int32_t round)
      {
        self->println("  ping <- 第 {} 回合", round);
        if (round >= max_rounds) {
          self->println("  打满 {} 回合, 双方退出", max_rounds);
          self->send_exit(pong, caf::exit_reason::user_shutdown);
          self->quit();
        } else {
          self->mail(demo::ping_atom_v, round + 1).send(pong);
        }
      },
  };
}

}  // namespace

namespace demo
{

void ping_pong(caf::actor_system& sys)
{
  sys.println("=== 12 ping_pong: 纯异步消息驱动的协议 ===");
  auto pong = sys.spawn(pong_impl);
  auto ping = sys.spawn(ping_impl, pong, int32_t {3});
  caf::scoped_actor self {sys};
  self->wait_for(ping, pong);
}

}  // namespace demo
