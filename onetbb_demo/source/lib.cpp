#include <cstddef>
#include <functional>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "lib.hpp"

#include <fmt/core.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#include "tbb_coroutine.hpp"

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

namespace
{
Task<void> produce_numbers(Channel<int>& channel, std::size_t item_count)
{
  for (std::size_t index = 0; index < item_count; ++index) {
    auto const value = static_cast<int>(index + 1);
    if (!(co_await channel.send(value))) {
      co_return;
    }
  }
  channel.close();
}

Task<std::vector<int>> consume_numbers(Channel<int>& channel)
{
  auto values = std::vector<int> {};
  while (auto value = co_await channel.receive()) {
    values.push_back(*value);
  }
  co_return values;
}
}  // namespace

CoroutinePipelineResult run_coroutine_pipeline(std::size_t item_count,
                                               std::size_t channel_capacity,
                                               int max_concurrency)
{
  if (item_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    throw std::invalid_argument {
        "item count exceeds the supported integer range"};
  }
  if (max_concurrency <= 0) {
    throw std::invalid_argument {"max concurrency must be greater than zero"};
  }

  auto scheduler = TbbScheduler {max_concurrency};
  auto channel = Channel<int> {scheduler, channel_capacity};
  auto producer = produce_numbers(channel, item_count);
  auto consumer = consume_numbers(channel);

  producer.start(scheduler);
  consumer.start(scheduler);
  scheduler.wait();

  producer.result();
  auto values = consumer.result();
  auto const sum = std::accumulate(values.begin(), values.end(), 0LL);
  return CoroutinePipelineResult {std::move(values), sum};
}
