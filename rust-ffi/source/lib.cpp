#include <cstdint>
#include <string>
#include <string_view>

#include "lib.hpp"

#include <fmt/format.h>

#include "rust_bridge/lib.h"

library::library()
    : name {fmt::format("{}", "rust-ffi")}
{
}

// The wrappers are implemented with qualified names instead of reopening
// `namespace rust`, because cxx also puts its support types (rust::Str,
// rust::String) into a namespace called `rust`.

auto rust::add(std::int32_t lhs, std::int32_t rhs) -> std::int32_t
{
  return rust_ffi::add(lhs, rhs);
}

auto rust::fibonacci(std::uint32_t nth) -> std::uint64_t
{
  return rust_ffi::fibonacci(nth);
}

auto rust::greet(std::string_view name) -> std::string
{
  auto const greeting = rust_ffi::greet(::rust::Str {name.data(), name.size()});
  return std::string {greeting.data(), greeting.size()};
}
