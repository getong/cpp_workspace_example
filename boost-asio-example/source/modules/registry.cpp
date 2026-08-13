#include "module.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include "coroutine/coroutine.hpp"
#include "echo_tcp/echo_tcp.hpp"
#include "hello/hello.hpp"
#include "post_dispatch/post_dispatch.hpp"
#include "resolver/resolver.hpp"
#include "strand/strand.hpp"
#include "timer/timer.hpp"
#include "udp_echo/udp_echo.hpp"

namespace modules
{

namespace
{

// 新增功能模块时，在这里追加一行即可。
constexpr std::array registry {
    module {"hello", "Print the project greeting from the core library",
            &hello::run},
    module {"timer",
            "steady_timer: sync wait, async_wait, periodic re-arm and "
            "cancellation",
            &timer::run},
    module {"post_dispatch",
            "Submitting work to an executor: post vs dispatch vs defer",
            &post_dispatch::run},
    module {"strand",
            "Serializing handlers on a multi-threaded thread_pool with a "
            "strand (no locks needed)",
            &strand::run},
    module {"coroutine",
            "C++20 coroutines: co_spawn, awaitable, and || / && awaitable "
            "operators",
            &coroutine::run},
    module {"echo_tcp",
            "Coroutine-based TCP echo server and client over loopback",
            &echo_tcp::run},
    module {"udp_echo",
            "Callback-style UDP echo over loopback datagrams",
            &udp_echo::run},
    module {"resolver",
            "Asynchronous name resolution with ip::tcp::resolver",
            &resolver::run},
};

}  // namespace

auto all() -> std::span<const module>
{
  return registry;
}

auto find(std::string_view name) -> const module*
{
  const auto* iter = std::ranges::find(registry, name, &module::name);
  return iter != registry.end() ? iter : nullptr;
}

}  // namespace modules
