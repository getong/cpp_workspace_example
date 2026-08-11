#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is cpp-20-exmaple", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "cpp-20-exmaple");
}
