#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <tbb/task_arena.h>

#include "lib.hpp"
#include "tbb_coroutine.hpp"

namespace
{
Task<int> current_tbb_worker_index()
{
  co_return tbb::this_task_arena::current_thread_index();
}
}  // namespace

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

TEST_CASE("coroutines run in a oneTBB task arena", "[coroutine]")
{
  auto scheduler = TbbScheduler {1};
  auto task = current_tbb_worker_index();

  task.start(scheduler);
  scheduler.wait();

  REQUIRE(task.result() != tbb::task_arena::not_initialized);
}

TEST_CASE("bounded channel communicates between coroutines",
          "[coroutine][channel]")
{
  auto const result = run_coroutine_pipeline(100, 1, 1);

  REQUIRE(result.values.size() == 100);
  REQUIRE(result.values.front() == 1);
  REQUIRE(result.values.back() == 100);
  REQUIRE(result.sum == 5050);
}

TEST_CASE("coroutine pipeline handles an empty input", "[coroutine][channel]")
{
  auto const result = run_coroutine_pipeline(0, 1, 1);

  REQUIRE(result.values.empty());
  REQUIRE(result.sum == 0);
}
