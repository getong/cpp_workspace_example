// 03 有状态 actor —— actor 是"状态 + 行为"的封装:
//
// * actor_from_state<State> 用一个普通 struct 定义 actor:
//   成员变量就是状态, make_behavior() 返回消息处理器
// * 状态只被这一个 actor 访问, 消息又是逐条处理的,
//   所以计数器不需要 mutex/atomic 也是线程安全的
// * 这是 actor 模型解决并发问题的核心思路: 用消息代替共享内存

#include <chrono>
#include <cstdint>

#include <caf/actor_from_state.hpp>
#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// 计数器 actor 的状态与行为
struct counter_state
{
  auto make_behavior() -> caf::behavior
  {
    return {
        [this](demo::inc_atom) { ++value; },
        [this](demo::dec_atom) { --value; },
        [this](demo::get_atom) { return value; },
    };
  }

  int32_t value = 0;
};

}  // namespace

namespace demo
{

void stateful_counter(caf::actor_system& sys)
{
  sys.println("=== 03 stateful_counter: 无锁的私有状态 ===");
  auto counter = sys.spawn(caf::actor_from_state<counter_state>);
  caf::scoped_actor self {sys};
  // send() 是"发后不管"的异步消息; 同一对收发方之间 CAF 保证消息有序,
  // 因此后面的 get 请求一定能看到前面所有 inc/dec 的效果
  self->mail(demo::inc_atom_v).send(counter);
  self->mail(demo::inc_atom_v).send(counter);
  self->mail(demo::inc_atom_v).send(counter);
  self->mail(demo::dec_atom_v).send(counter);
  self->mail(demo::get_atom_v)
      .request(counter, std::chrono::seconds(1))
      .receive([&self](int32_t value)
               { self->println("  3 次 inc + 1 次 dec 后, 计数 = {}", value); },
               [&self](const caf::error& err)
               { self->println("  出错: {}", caf::to_string(err)); });
  self->send_exit(counter, caf::exit_reason::user_shutdown);
}

}  // namespace demo
