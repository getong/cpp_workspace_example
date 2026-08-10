#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is asio-call-tokio", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "asio-call-tokio");
}
