#pragma once

#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief The core implementation of the executable
 *
 * This class makes up the library part of the executable, which means that the
 * main logic is implemented here. This kind of separation makes it easy to
 * test the implementation for the executable, because the logic is nicely
 * separated from the command-line logic implemented in the main function.
 */
struct library
{
  /**
   * @brief Simply initializes the name member to the name of the project
   */
  library();

  std::string name;
};

int parallel_sum(std::vector<int> const& values);

struct CoroutinePipelineResult
{
  std::vector<int> values;
  long long sum;
};

CoroutinePipelineResult run_coroutine_pipeline(std::size_t item_count,
                                               std::size_t channel_capacity = 4,
                                               int max_concurrency = 2);
