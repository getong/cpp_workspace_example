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
#include <tbb/flow_graph.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>

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

FlowPipelineResult run_flow_pipeline(std::size_t item_count,
                                     std::size_t channel_capacity,
                                     int max_concurrency)
{
  if (item_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    throw std::invalid_argument {
        "item count exceeds the supported integer range"};
  }
  if (channel_capacity == 0) {
    throw std::invalid_argument {
        "channel capacity must be greater than zero"};
  }
  if (max_concurrency <= 0) {
    throw std::invalid_argument {"max concurrency must be greater than zero"};
  }

  auto arena = tbb::task_arena {max_concurrency};
  auto values = std::vector<int> {};

  arena.execute(
      [&]
      {
        auto graph = tbb::flow::graph {};

        auto producer = tbb::flow::input_node<int> {
            graph,
            [item_count, next = std::size_t {0}](
                tbb::flow_control& control) mutable
            {
              if (next == item_count) {
                control.stop();
                return 0;
              }
              return static_cast<int>(++next);
            }};

        // Mirrors the bounded channel: at most channel_capacity messages are
        // in flight until the consumer signals the decrementer.
        auto limiter = tbb::flow::limiter_node<int> {graph, channel_capacity};

        auto consumer = tbb::flow::function_node<int, tbb::flow::continue_msg> {
            graph,
            tbb::flow::serial,
            [&values](int value)
            {
              values.push_back(value);
              return tbb::flow::continue_msg {};
            }};

        tbb::flow::make_edge(producer, limiter);
        tbb::flow::make_edge(limiter, consumer);
        tbb::flow::make_edge(consumer, limiter.decrementer());

        producer.activate();
        graph.wait_for_all();
      });

  auto const sum = std::accumulate(values.begin(), values.end(), 0LL);
  return FlowPipelineResult {std::move(values), sum};
}
