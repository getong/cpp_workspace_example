// 07 错误处理 —— 错误也是消息:
//
// * handler 返回 caf::result<T>: 既可以返回值, 也可以返回 caf::error
// * 错误会自动传播给请求方, 由 request 的错误分支统一处理
// * caf::sec 是 CAF 内置的错误码枚举, 也可以注册自定义错误类型
// * 没有异常跨线程传播的问题: 错误沿着消息流走, 路径清晰可查

#include <chrono>
#include <cmath>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>
#include <caf/typed_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

using divider_actor =
    caf::typed_actor<caf::result<double>(demo::div_atom, double, double)>;

auto divider_impl() -> divider_actor::behavior_type
{
  return {
      [](demo::div_atom, double dividend, double divisor) -> caf::result<double>
      {
        if (std::fpclassify(divisor) == FP_ZERO) {
          // 返回 error 而不是值, 请求方会走到错误分支
          return caf::make_error(caf::sec::invalid_argument, "除数不能为零");
        }
        return dividend / divisor;
      },
  };
}

}  // namespace

namespace demo
{

void error_handling(caf::actor_system& sys)
{
  sys.println("=== 07 error_handling: 用 result<T> 传播错误 ===");
  auto divider = sys.spawn(divider_impl);
  caf::scoped_actor self {sys};
  auto divide = [&self, &divider](double dividend, double divisor)
  {
    self->mail(demo::div_atom_v, dividend, divisor)
        .request(divider, std::chrono::seconds(1))
        .receive(
            [&](double res)
            { self->println("  {} / {} = {}", dividend, divisor, res); },
            [&](const caf::error& err)
            {
              self->println(
                  "  {} / {} 失败: {}", dividend, divisor, caf::to_string(err));
            });
  };
  divide(84.0, 2.0);  // 正常返回值
  divide(1.0, 0.0);  // 返回 caf::error
  self->send_exit(divider, caf::exit_reason::user_shutdown);
}

}  // namespace demo
