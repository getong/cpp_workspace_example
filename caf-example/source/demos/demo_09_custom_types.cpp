// 09 自定义消息类型 —— 传递业务对象:
//
// * types.hpp 里的 order/receipt 提供了 inspect() 并注册了 type ID,
//   于是可以像内置类型一样出现在消息里
// * 同一套 inspect() 还让 CAF 能把对象打印成人类可读文本
//   (deep_to_string), 以及在分布式场景下做网络序列化
// * 也就是说: 定义一次, 检查/打印/序列化全都有了

#include <chrono>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// 商店 actor: 收到订单, 应答一张回执
auto shop_impl() -> caf::behavior
{
  return {
      [](demo::buy_atom, const demo::order& incoming) -> demo::receipt
      {
        auto total = incoming.unit_price * incoming.quantity;
        return demo::receipt {incoming.id, total};
      },
  };
}

}  // namespace

namespace demo
{

void custom_types(caf::actor_system& sys)
{
  sys.println("=== 09 custom_types: 自定义 struct 作为消息 ===");
  auto shop = sys.spawn(shop_impl);
  caf::scoped_actor self {sys};
  auto my_order = order {1024, "C++ Actor Framework 实战", 3, 59.9};
  // deep_to_string 借助 inspect() 打印任意可检查类型
  self->println("  下单: {}", caf::deep_to_string(my_order));
  self->mail(demo::buy_atom_v, my_order)
      .request(shop, std::chrono::seconds(1))
      .receive([&self](const receipt& bill)
               { self->println("  回执: {}", caf::deep_to_string(bill)); },
               [&self](const caf::error& err)
               { self->println("  下单失败: {}", caf::to_string(err)); });
  self->send_exit(shop, caf::exit_reason::user_shutdown);
}

}  // namespace demo
