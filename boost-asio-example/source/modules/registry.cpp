#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include "actor_chat/actor_chat.hpp"
#include "actor_genserver/actor_genserver.hpp"
#include "actor_pingpong/actor_pingpong.hpp"
#include "actor_pipeline/actor_pipeline.hpp"
#include "actor_pool/actor_pool.hpp"
#include "actor_supervisor/actor_supervisor.hpp"
#include "coroutine/coroutine.hpp"
#include "csp_channel/csp_channel.hpp"
#include "csp_duo/csp_duo.hpp"
#include "csp_fanin/csp_fanin.hpp"
#include "csp_mpsc/csp_mpsc.hpp"
#include "csp_select/csp_select.hpp"
#include "csp_sieve/csp_sieve.hpp"
#include "echo_tcp/echo_tcp.hpp"
#include "hello/hello.hpp"
#include "module.hpp"
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
constexpr std::
    array
        registry {
            module {"hello",
                    "Print the project greeting from the core library",
                    &hello::run},
            module {
                "timer",
                "steady_timer: sync wait, async_wait, periodic re-arm and " "ca"
                                                                            "nc"
                                                                            "el"
                                                                            "la"
                                                                            "ti" "on",
                &timer::run},
            module {"post_dispatch",
                    "Submitting work to an executor: post vs dispatch vs defer",
                    &post_dispatch::run},
            module {"strand",
                    "Serializing handlers on a multi-threaded thread_pool with "
                    "a " "strand " "(no " "locks " "needed" ")",
                    &strand::run},
            module {"coroutine",
                    "C++20 coroutines: co_spawn, awaitable, and || / && "
                    "awaitable " "operat" "ors",
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
            module {
                "actor_pingpong",
                "Erlang ping-pong: two actors exchanging messages through " "ma"
                                                                            "il"
                                                                            "bo"
                                                                            "xe"
                                                                            "s " "(channels" ")",
                &actor_pingpong::run},
            module {
                "actor_genserver",
                "gen_server-style actor: cast vs call (request-reply via a " "r"
                                                                             "e"
                                                                             "p"
                                                                             "l"
                                                                             "y"
                                                                             " " "channel)," " lock-" "free " "state",
                &actor_genserver::run},
            module {"actor_supervisor",
                    "Let it crash: a supervisor restarts a crashing worker "
                    "with a " "restar" "t " "limit",
                    &actor_supervisor::run},
            module {"actor_pipeline",
                    "Actor pipeline over bounded mailboxes: automatic "
                    "backpressure " "and " "casca" "ding " "shutd" "own",
                    &actor_pipeline::run},
            module {"actor_pool",
                    "Worker pool: N workers pull jobs from a shared queue, "
                    "results " "fan " "in " "to a " "colle" "ctor",
                    &actor_pool::run},
            module {
                "actor_chat",
                "TCP chat room, one actor per connection plus a room actor " "o"
                                                                             "w"
                                                                             "n"
                                                                             "i"
                                                                             "n"
                                                                             "g"
                                                                             " " "the " "member " "list",
                &actor_chat::run},
            module {"csp_channel",
                    "Go channel basics: unbuffered rendezvous, buffered, close "
                    "and " "range-over-channel",
                    &csp_channel::run},
            module {"csp_duo",
            "Two co_spawned coroutines talking over channels: alternation, "
            "duplex dialogue, ownership handoff, demand-driven flow",
            &csp_duo::run},
    module {"csp_select",
                    "Go select: multi-channel wait, time.After timeout, "
                    "done-channel " "broadcast cancellation",
                    &csp_select::run},
            module {
                "csp_fanin",
                "Fan-out over a shared jobs channel, fan-in to one merged " "ch"
                                                                            "an"
                                                                            "ne"
                                                                            "l,"
                                                                            " &"
                                                                            "& "
                                                                            "as"
                                                                            " s"
                                                                            "yn"
                                                                            "c."
                                                                            "Wa"
                                                                            "it"
                                                                            "Gr"
                                                                            "ou"
                                                                            "p",
                &csp_fanin::run},
            module {"csp_mpsc",
            "MPSC channels: producers funneling into one consumer, on one "
            "thread and across a thread pool (concurrent_channel)",
            &csp_mpsc::run},
    module {"csp_sieve",
                    "Concurrent prime sieve: a dynamically growing chain of "
                    "filter " "processes (the CSP classic)",
                    &csp_sieve::run},
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
