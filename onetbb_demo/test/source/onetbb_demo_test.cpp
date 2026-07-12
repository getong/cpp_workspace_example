#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <tbb/flow_graph.h>
#include <tbb/task_arena.h>

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

TEST_CASE("flow graph nodes run in a oneTBB task arena", "[flow]")
{
  auto arena = tbb::task_arena {1};
  auto thread_index = static_cast<int>(tbb::task_arena::not_initialized);

  arena.execute(
      [&]
      {
        auto graph = tbb::flow::graph {};
        auto node = tbb::flow::continue_node<tbb::flow::continue_msg> {
            graph,
            [&thread_index](tbb::flow::continue_msg)
            {
              thread_index = tbb::this_task_arena::current_thread_index();
              return tbb::flow::continue_msg {};
            }};

        node.try_put(tbb::flow::continue_msg {});
        graph.wait_for_all();
      });

  REQUIRE(thread_index != tbb::task_arena::not_initialized);
}

TEST_CASE("bounded flow graph pipeline delivers all values", "[flow]")
{
  auto const result = run_flow_pipeline(100, 1, 1);

  REQUIRE(result.values.size() == 100);
  REQUIRE(result.values.front() == 1);
  REQUIRE(result.values.back() == 100);
  REQUIRE(result.sum == 5050);
}

TEST_CASE("flow graph pipeline handles an empty input", "[flow]")
{
  auto const result = run_flow_pipeline(0, 1, 1);

  REQUIRE(result.values.empty());
  REQUIRE(result.sum == 0);
}
