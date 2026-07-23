// 02 强类型 actor (typed actor) —— 用类型系统约束消息协议:
//
// * typed_actor<...> 在类型层面声明 actor 能处理哪些消息、各自应答什么
// * 向 typed actor 发送协议之外的消息会直接编译失败, 而不是运行时丢弃
// * 这让 actor 间的接口像普通函数签名一样可被编译器检查

#include <chrono>
#include <cstdint>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>
#include <caf/typed_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// 计算器的消息协议: (add_atom, int, int) -> int, 以此类推。
// caf::result<T> 表示"应答一个 T, 或者一个 error"。
using calculator_actor =
    caf::typed_actor<caf::result<int32_t>(demo::add_atom, int32_t, int32_t),
                     caf::result<int32_t>(demo::sub_atom, int32_t, int32_t),
                     caf::result<int32_t>(demo::mul_atom, int32_t, int32_t)>;

// 实现必须精确覆盖协议中的每一个签名, 少一个或类型不匹配都无法编译
auto calculator_impl() -> calculator_actor::behavior_type
{
  return {
      [](demo::add_atom, int32_t lhs, int32_t rhs) { return lhs + rhs; },
      [](demo::sub_atom, int32_t lhs, int32_t rhs) { return lhs - rhs; },
      [](demo::mul_atom, int32_t lhs, int32_t rhs) { return lhs * rhs; },
  };
}

}  // namespace

namespace demo
{

void typed_calculator(caf::actor_system& sys)
{
  sys.println("=== 02 typed_calculator: 编译期检查的消息协议 ===");
  calculator_actor calc = sys.spawn(calculator_impl);
  caf::scoped_actor self {sys};
  auto timeout = std::chrono::seconds(1);
  auto ask = [&self, &calc, timeout](
                 auto op, const char* sign, int32_t lhs, int32_t rhs)
  {
    self->mail(op, lhs, rhs)
        .request(calc, timeout)
        .receive([&](int32_t res)
                 { self->println("  {} {} {} = {}", lhs, sign, rhs, res); },
                 [&](const caf::error& err)
                 { self->println("  出错: {}", caf::to_string(err)); });
  };
  ask(demo::add_atom_v, "+", 20, 22);
  ask(demo::sub_atom_v, "-", 50, 8);
  ask(demo::mul_atom_v, "*", 6, 7);
  // 下面这行如果取消注释, 会因为协议里没有 div_atom 而编译失败:
  // self->mail(demo::div_atom_v, 1, 2).request(calc, timeout);
  self->send_exit(calc, caf::exit_reason::user_shutdown);
}

}  // namespace demo
