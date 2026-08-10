#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>

#include "lib.hpp"

namespace
{

/// The demo: a C++ (asio) coroutine awaiting Rust (tokio) coroutines.
auto demo(library const& lib) -> asio::awaitable<void>
{
  using namespace std::chrono_literals;
  auto const started = std::chrono::steady_clock::now();
  auto const elapsed = [started]
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - started)
        .count();
  };

  std::cout << "[c++  " << elapsed() << "ms] asio coroutine on thread "
            << std::this_thread::get_id() << ", awaiting tokio...\n";

  // One await: the coroutine suspends here, tokio sleeps 200ms on its own
  // worker threads, then the result resumes us back on the io_context.
  auto const sum = co_await rust_tokio::sleep_then_add(40, 2, 200ms);
  std::cout << "[c++  " << elapsed() << "ms] rust says 40 + 2 = " << sum
            << " (still on thread " << std::this_thread::get_id() << ")\n";

  // Two tokio tasks awaited concurrently: total wait is ~300ms, not 550ms.
  using asio::experimental::awaitable_operators::operator&&;
  auto const [next_sum, greeting] =
      co_await (rust_tokio::sleep_then_add(sum, 58, 300ms)
                && rust_tokio::fetch_greeting(lib.name, 250ms));
  std::cout << "[c++  " << elapsed() << "ms] concurrent awaits done: " << sum
            << " + 58 = " << next_sum << '\n';
  std::cout << "[c++  " << elapsed() << "ms] " << greeting << '\n';
}

}  // namespace

auto main() -> int
{
  auto const lib = library {};
  std::cout << "Hello from " << lib.name << "!\n";

  auto ioc = asio::io_context {};
  asio::co_spawn(ioc,
                 demo(lib),
                 [](std::exception_ptr error)
                 {
                   if (error) {
                     std::rethrow_exception(error);
                   }
                 });
  ioc.run();

  return 0;
}
