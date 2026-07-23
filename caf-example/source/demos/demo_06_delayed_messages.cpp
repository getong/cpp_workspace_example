// 06 定时消息 + 响应承诺(response promise):
//
// * mail(..).delay(dur).send(target) 让 CAF 的时钟在 dur 之后投递消息,
//   actor 给自己发定时消息是实现周期任务/超时的惯用手法
// * make_response_promise<T>() 允许"先收下请求, 稍后再答复":
//   handler 返回 promise 而不是值, 之后任意时刻调用 deliver() 完成应答
//
// 本例: 火箭 actor 收到发射请求后开始倒计时, 每 100ms 给自己发一个 tick,
// 数到 0 才通过 promise 把"点火!"答复给请求方。

#include <chrono>
#include <cstdint>
#include <string>

#include <caf/actor_from_state.hpp>
#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

using namespace std::literals;

struct rocket_state
{
  // actor_from_state 会把 actor 自身指针传给状态的构造函数
  explicit rocket_state(caf::event_based_actor* self_ptr)
      : self(self_ptr)
  {
  }

  auto make_behavior() -> caf::behavior
  {
    return {
        [this](demo::start_atom, int32_t from) -> caf::result<std::string>
        {
          remaining = from;
          // 不立即应答: 先拿到一个 promise, 倒计时结束后再 deliver
          launched = self->make_response_promise<std::string>();
          self->mail(demo::tick_atom_v)
              .delay(100ms)
              .send(caf::actor_cast<caf::actor>(self));
          return launched;
        },
        [this](demo::tick_atom)
        {
          self->println("  倒计时: {}", remaining);
          --remaining;
          if (remaining > 0) {
            // 继续给自己发下一个定时 tick
            self->mail(demo::tick_atom_v)
                .delay(100ms)
                .send(caf::actor_cast<caf::actor>(self));
          } else {
            launched.deliver(std::string {"点火! 发射成功"});
            self->quit();
          }
        },
    };
  }

  caf::event_based_actor* self;
  int32_t remaining = 0;
  caf::typed_response_promise<std::string> launched;
};

}  // namespace

namespace demo
{

void delayed_messages(caf::actor_system& sys)
{
  sys.println("=== 06 delayed_messages: 定时消息与延迟应答 ===");
  auto rocket = sys.spawn(caf::actor_from_state<rocket_state>);
  caf::scoped_actor self {sys};
  self->mail(demo::start_atom_v, int32_t {3})
      .request(rocket, std::chrono::seconds(5))
      .receive([&self](const std::string& msg) { self->println("  {}", msg); },
               [&self](const caf::error& err)
               { self->println("  发射失败: {}", caf::to_string(err)); });
}

}  // namespace demo
