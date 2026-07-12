#include <iostream>
#include <vector>

#include "lib.hpp"

int main()
{
  auto const data = std::vector<int>(100, 1);
  auto const pipeline = run_coroutine_pipeline(10, 2);

  std::cout << "Total Sum: " << parallel_sum(data) << '\n';
  std::cout << "Coroutine received: " << pipeline.values.size() << " values\n";
  std::cout << "Coroutine sum: " << pipeline.sum << '\n';
  return 0;
}
