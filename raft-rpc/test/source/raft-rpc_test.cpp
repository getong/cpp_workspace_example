#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is raft-rpc", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "raft-rpc");
}
