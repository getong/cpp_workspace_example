#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is onetbb_demo", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "onetbb_demo");
}

TEST_CASE("parallel_sum sums values with oneTBB", "[library]")
{
  auto const values = std::vector<int> {1, 2, 3, 4, 5, 6};
  REQUIRE(parallel_sum(values) == 21);
}
