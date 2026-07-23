// 13 请求转交 (delegation) —— 透明的后端路由:
//
// * mail(..).delegate(backend) 把当前请求"移交"给另一个 actor:
//   最终由 backend 直接答复原始请求方, 前台不再经手应答
// * 客户端完全感知不到转交的存在 —— 它只认识前台
// * 这是实现网关/负载均衡/分片路由的基础构件

#include <chrono>
#include <string>

#include <caf/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// 后端: 真正干活的存储服务
auto backend_impl(caf::event_based_actor* self) -> caf::behavior
{
  return {
      [self](demo::get_atom, const std::string& key) -> std::string
      {
        self->println("  后端: 收到查询 \"{}\" (直接答复客户端)", key);
        return "值<" + key + ">";
      },
  };
}

// 前台: 只负责把请求转交给后端
auto frontend_impl(caf::event_based_actor* self, const caf::actor& backend)
    -> caf::behavior
{
  return {
      [self, backend](demo::get_atom get, const std::string& key)
      {
        self->println("  前台: 把 \"{}\" 转交给后端", key);
        return self->mail(get, key).delegate(backend);
      },
  };
}

}  // namespace

namespace demo
{

void delegation(caf::actor_system& sys)
{
  sys.println("=== 13 delegation: delegate() 转交请求 ===");
  auto backend = sys.spawn(backend_impl);
  auto frontend = sys.spawn(frontend_impl, backend);
  caf::scoped_actor self {sys};
  // 客户端只向前台发请求, 应答却来自后端
  self->mail(demo::get_atom_v, std::string {"用户配置"})
      .request(frontend, std::chrono::seconds(1))
      .receive(
          [&self](const std::string& value)
          { self->println("  客户端: 收到应答 \"{}\"", value); },
          [&self](const caf::error& err)
          { self->println("  客户端: 查询失败: {}", caf::to_string(err)); });
  self->send_exit(frontend, caf::exit_reason::user_shutdown);
  self->send_exit(backend, caf::exit_reason::user_shutdown);
}

}  // namespace demo
