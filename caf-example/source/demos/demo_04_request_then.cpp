// 04 异步 request/then —— 事件驱动 actor 之间的非阻塞请求-应答:
//
// * event_based_actor 不阻塞线程: request(..).then(..) 注册一个回调,
//   然后立刻返回, 应答到达时调度器再唤醒它
// * 成千上万个这样的 actor 可以复用少量线程(CAF 默认按 CPU 核数建线程池)
// * then() 之外还有 await(): await 会推迟处理其他消息直到应答到达,
//   then 则允许 actor 在等待期间继续处理别的消息

#include <chrono>
#include <cstdint>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"

namespace
{

// 平方计算服务
auto squarer_impl() -> caf::behavior
{
  return {
      [](int32_t value) { return value * value; },
  };
}

// 客户端: 一个函数式定义的事件驱动 actor。
// 参数 self 是 actor 自身指针, 后面的参数由 spawn 传入。
void client_impl(caf::event_based_actor* self, const caf::actor& squarer)
{
  self->println("  客户端: 发送 7, 等待平方结果(不阻塞线程)...");
  self->mail(int32_t {7})
      .request(squarer, std::chrono::seconds(1))
      .then(
          [self](int32_t result)
          {
            self->println("  客户端: 收到 7 * 7 = {}", result);
            self->quit();  // 工作完成, 主动退出
          },
          [self](caf::error& err)
          {
            self->println("  客户端: 请求失败: {}", caf::to_string(err));
            self->quit(std::move(err));
          });
}

}  // namespace

namespace demo
{

void request_then(caf::actor_system& sys)
{
  sys.println("=== 04 request_then: 非阻塞的异步请求 ===");
  auto squarer = sys.spawn(squarer_impl);
  auto client = sys.spawn(client_impl, squarer);
  // wait_for 阻塞等待 client 终止, 保证本演示的输出集中在一起
  caf::scoped_actor self {sys};
  self->wait_for(client);
  self->send_exit(squarer, caf::exit_reason::user_shutdown);
}

}  // namespace demo
