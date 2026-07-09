#include <iostream>
#include <vector>

#include "lib.hpp"

int main()
{
  auto const data = std::vector<int>(100, 1);

  std::cout << "Total Sum: " << parallel_sum(data) << '\n';
  return 0;
}
