#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <boost/asio/awaitable.hpp>

// The project uses Boost.Asio (the vcpkg standalone asio port is stale), but
// it is the same codebase as standalone asio, so a namespace alias keeps the
// code portable between the two flavors.
namespace asio = boost::asio;

/**
 * @brief The core implementation of the executable
 *
 * This class makes up the library part of the executable, which means that the
 * main logic is implemented here. This kind of separation makes it easy to
 * test the implementation for the executable, because the logic is nicely
 * separated from the command-line logic implemented in the main function.
 */
struct library
{
  /**
   * @brief Simply initializes the name member to the name of the project
   */
  library();

  std::string name;
};

/**
 * @brief asio coroutine wrappers around the Rust tokio core
 *
 * The Rust implementation lives in rust/src/lib.rs and runs on a global
 * tokio runtime. The boundary is a cxx bridge (https://cxx.rs) with a
 * completion-callback shape: each wrapper suspends the calling asio
 * coroutine, hands Rust an opaque pointer to the pending operation, and is
 * resumed on its own executor once the tokio task calls back with the
 * result (see bridge_callbacks.hpp). From the caller's perspective a plain
 * `co_await` awaits Rust async code.
 */
namespace rust_tokio
{

/// Awaits a tokio task that sleeps `delay` and then adds the two integers.
auto sleep_then_add(std::int32_t lhs,
                    std::int32_t rhs,
                    std::chrono::milliseconds delay)
    -> asio::awaitable<std::int32_t>;

/// Awaits a tokio task that composes a greeting for `name` after `delay`.
auto fetch_greeting(std::string_view name, std::chrono::milliseconds delay)
    -> asio::awaitable<std::string>;

}  // namespace rust_tokio
