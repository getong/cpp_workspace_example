#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is cmake_project_demo", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "cmake_project_demo");
}
