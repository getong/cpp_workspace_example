#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is template_interface", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "template_interface");
}
