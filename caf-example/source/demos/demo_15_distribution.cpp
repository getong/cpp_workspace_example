// 15 网络分布 —— CAF 的 Erlang 式分布式通信:
//
// CAF 通过 caf::io 模块(middleman)提供与 Erlang 分布(distributed Erlang)
// 同类的能力:
//
// * 位置透明: 拿到远端 actor 的句柄后, mail/request/monitor 的用法与本地
//   actor 完全一致, 业务代码不感知网络的存在
// * publish(actor, port):   把 actor 绑定到 TCP 端口, 相当于 Erlang 节点
//   对外暴露一个注册进程
// * remote_actor(host, port): 连接远端节点并取回一个代理(proxy)句柄,
//   相当于 {name, node@host} 式的远程引用
// * 节点间走 CAF 自带的 BASP 二进制协议, 消息用 types.hpp 里注册的
//   inspect() 自动序列化/反序列化 —— 这正是 type ID 注册的意义所在
// * 跨节点同样支持 monitor/link 语义: 连接断开时代理 actor 会以
//   exit/down 消息通知本地, 与 Erlang 的 noconnection 信号对应
//
// 与 Erlang 的差异: CAF 没有全局注册表和 epmd, 需要显式指定 host:port;
// 也没有内置的热代码升级。另有更新的 caf::net 模块提供 TCP/WebSocket/
// HTTP 等传输, 这里演示的是经典的 caf::io middleman。
//
// 本演示在同一进程内完成"发布 -> 远程连接 -> 通信"闭环: 消息真实经过
// 序列化和 127.0.0.1 的 TCP 回环, 与跨机器部署走同一条代码路径。
// 真正跨进程/跨机器时, 只需服务端调 publish、客户端调 remote_actor,
// 用 --caf.middleman.* 或命令行参数传入各自的 host/port 即可。

#include <chrono>
#include <cstdint>

#include <caf/all.hpp>
#include <caf/io/all.hpp>
#include <caf/scoped_actor.hpp>

#include "demos/demos.hpp"
#include "demos/types.hpp"

namespace
{

// "远端"服务: 写法与本地 actor 毫无区别 —— 这就是位置透明。
// 提供两个接口: 加法(内置类型), 下单(自定义 struct, 走网络序列化)
auto remote_shop_impl(caf::event_based_actor* self) -> caf::behavior
{
  return {
      [self](demo::add_atom, int32_t lhs, int32_t rhs) -> int32_t
      {
        self->println("  [服务端] 收到加法请求: {} + {}", lhs, rhs);
        return lhs + rhs;
      },
      [self](demo::buy_atom, const demo::order& incoming) -> demo::receipt
      {
        self->println("  [服务端] 收到网络订单: {}",
                      caf::deep_to_string(incoming));
        return demo::receipt {incoming.id,
                              incoming.unit_price * incoming.quantity};
      },
  };
}

}  // namespace

namespace demo
{

void distribution(caf::actor_system& sys)
{
  sys.println("=== 15 distribution: publish/remote_actor 网络分布 ===");
  auto& mman = sys.middleman();  // io 模块的入口, 由 CAF_MAIN 加载
  auto server = sys.spawn(remote_shop_impl);
  // 端口传 0 表示让操作系统挑一个空闲端口, 返回实际绑定的端口号
  auto port = mman.publish(server, 0, "127.0.0.1");
  if (!port) {
    sys.println("  发布失败: {}", caf::to_string(port.error()));
    caf::scoped_actor self {sys};
    self->send_exit(server, caf::exit_reason::user_shutdown);
    return;
  }
  sys.println("  服务端: actor 已发布在 127.0.0.1:{}", *port);
  // 客户端视角: 连接远端节点, 取回代理句柄。此后对 remote 的一切操作
  // 都会被序列化后经 TCP 发往对端, 应答原路返回
  auto remote = mman.remote_actor("127.0.0.1", *port);
  if (!remote) {
    sys.println("  连接失败: {}", caf::to_string(remote.error()));
  } else {
    sys.println("  客户端: 已连接, 代理句柄与本地句柄用法一致");
    caf::scoped_actor self {sys};
    self->mail(demo::add_atom_v, int32_t {40}, int32_t {2})
        .request(*remote, std::chrono::seconds(2))
        .receive([&self](int32_t sum)
                 { self->println("  [客户端] 40 + 2 = {} (经 TCP 往返)", sum); },
                 [&self](const caf::error& err)
                 { self->println("  [客户端] 加法失败: {}", caf::to_string(err)); });
    auto my_order = order {7, "分布式系统(网络下单)", 2, 128.0};
    self->mail(demo::buy_atom_v, my_order)
        .request(*remote, std::chrono::seconds(2))
        .receive([&self](const receipt& bill)
                 { self->println("  [客户端] 收到回执: {}",
                                 caf::deep_to_string(bill)); },
                 [&self](const caf::error& err)
                 { self->println("  [客户端] 下单失败: {}", caf::to_string(err)); });
  }
  // 清理: 关闭监听端口, 再终止服务 actor
  mman.unpublish(server);
  caf::scoped_actor self {sys};
  self->send_exit(server, caf::exit_reason::user_shutdown);
  self->wait_for(server);
}

}  // namespace demo
