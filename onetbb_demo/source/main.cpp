#include <iostream>
#include <vector>

#include "lib.hpp"

int main()
{
  auto const data = std::vector<int>(100, 1);
  auto const pipeline = run_flow_pipeline(10, 2);

  std::cout << "Total Sum: " << parallel_sum(data) << '\n';
  std::cout << "Flow graph received: " << pipeline.values.size() << " values\n";
  std::cout << "Flow graph sum: " << pipeline.sum << '\n';
  return 0;
}
