#include <cstdint>
#include <limits>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is rust-ffi", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "rust-ffi");
}

TEST_CASE("Rust add saturates on overflow", "[rust-ffi]")
{
  REQUIRE(rust::add(40, 2) == 42);
  REQUIRE(rust::add(std::numeric_limits<std::int32_t>::max(), 1)
          == std::numeric_limits<std::int32_t>::max());
}

TEST_CASE("Rust fibonacci matches known values", "[rust-ffi]")
{
  REQUIRE(rust::fibonacci(0) == 0);
  REQUIRE(rust::fibonacci(1) == 1);
  REQUIRE(rust::fibonacci(10) == 55);
  REQUIRE(rust::fibonacci(42) == 267914296);
}

TEST_CASE("Rust greet embeds the name", "[rust-ffi]")
{
  auto const greeting = rust::greet("Gerald");
  REQUIRE(greeting.find("Gerald") != std::string::npos);
}
