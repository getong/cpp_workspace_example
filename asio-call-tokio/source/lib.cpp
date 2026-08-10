#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "lib.hpp"

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <fmt/format.h>

#include "bridge_callbacks.hpp"
#include "rust_bridge/lib.h"

library::library()
    : name {fmt::format("{}", "asio-call-tokio")}
{
}

namespace
{

/// A C++ coroutine suspended on a Rust tokio task.
///
/// The initiation lambda type-erases the completion handler and parks it
/// here together with a work guard, so io_context::run() cannot return
/// while the result is still being computed on a tokio thread. Ownership
/// travels through the FFI as a raw pointer (`ctx`) and comes back exactly
/// once via the matching tokio_ffi::complete_* callback.
template<typename Result>
struct pending_op
{
  asio::any_completion_handler<void(Result)> handler;
  asio::executor_work_guard<asio::any_io_executor> work;
};

/// Parks `handler` in a heap-allocated pending_op and returns the opaque
/// context to pass across the FFI.
template<typename Result, typename Handler>
auto park(Handler&& handler) -> std::size_t
{
  auto executor =
      asio::any_io_executor {asio::get_associated_executor(handler)};
  auto op = std::make_unique<pending_op<Result>>(
      pending_op<Result> {std::forward<Handler>(handler),
                          asio::make_work_guard(std::move(executor))});
  return reinterpret_cast<std::size_t>(op.release());
}

/// Reclaims the pending_op behind `ctx` and resumes its coroutine with
/// `result` on the executor it was suspended on. Called from tokio threads,
/// hence the post: completion handlers must not run on foreign threads.
template<typename Result>
void resume(std::size_t ctx, Result result)
{
  auto op = std::unique_ptr<pending_op<Result>> {
      reinterpret_cast<pending_op<Result>*>(ctx)};
  asio::post(
      op->work.get_executor(),
      [handler = std::move(op->handler), result = std::move(result)]() mutable
      { std::move(handler)(std::move(result)); });
}

}  // namespace

void tokio_ffi::complete_add(std::size_t ctx, std::int32_t value)
{
  resume<std::int32_t>(ctx, value);
}

void tokio_ffi::complete_greet(std::size_t ctx, ::rust::String value)
{
  resume<std::string>(ctx, std::string {value.data(), value.size()});
}

auto rust_tokio::sleep_then_add(std::int32_t lhs,
                                std::int32_t rhs,
                                std::chrono::milliseconds delay)
    -> asio::awaitable<std::int32_t>
{
  return asio::async_initiate<asio::use_awaitable_t<> const,
                              void(std::int32_t)>(
      [lhs, rhs, delay](auto&& handler)
      {
        auto const ctx =
            park<std::int32_t>(std::forward<decltype(handler)>(handler));
        tokio_ffi::sleep_then_add(
            lhs, rhs, static_cast<std::uint64_t>(delay.count()), ctx);
      },
      asio::use_awaitable);
}

auto rust_tokio::fetch_greeting(std::string_view name,
                                std::chrono::milliseconds delay)
    -> asio::awaitable<std::string>
{
  return asio::async_initiate<asio::use_awaitable_t<> const, void(std::string)>(
      [name = std::string {name}, delay](auto&& handler)
      {
        auto const ctx =
            park<std::string>(std::forward<decltype(handler)>(handler));
        tokio_ffi::fetch_greeting(::rust::Str {name.data(), name.size()},
                                  static_cast<std::uint64_t>(delay.count()),
                                  ctx);
      },
      asio::use_awaitable);
}
