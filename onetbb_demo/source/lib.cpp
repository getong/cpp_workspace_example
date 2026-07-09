#include <cstddef>
#include <functional>

#include "lib.hpp"

#include <fmt/core.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

library::library()
    : name {fmt::format("{}", "onetbb_demo")}
{
}

int parallel_sum(std::vector<int> const& values)
{
  return tbb::parallel_reduce(
      tbb::blocked_range<std::size_t> {0, values.size()},
      0,
      [&values](tbb::blocked_range<std::size_t> const& range, int init)
      {
        for (auto index = range.begin(); index != range.end(); ++index) {
          init += values[index];
        }

        return init;
      },
      std::plus<> {});
}
