#include "ranges.hpp"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <string_view>
#include <vector>

namespace modules::ranges
{

void run()
{
  const std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  // views 是惰性求值的：组合管道时不做任何计算，
  // 只有在迭代时才逐个元素拉取，不产生中间容器。
  auto pipeline = nums
      | std::views::filter([](int n) -> bool { return n % 2 == 0; })
      | std::views::transform([](int n) -> int { return n * n; })
      | std::views::take(3);

  std::cout << "even -> square -> take(3):";
  for (const int value : pipeline) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  // ranges 算法直接接受容器，不用再写 begin()/end()。
  std::vector<int> shuffled = {5, 3, 8, 1, 9, 2};
  std::ranges::sort(shuffled);
  std::cout << "sorted:";
  for (const int value : shuffled) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  // 投影（projection）：算法内部按投影结果比较，不用手写比较器。
  struct person
  {
    std::string_view name;
    int age;
  };
  std::vector<person> people = {{"alice", 32}, {"bob", 25}, {"carol", 40}};
  std::ranges::sort(people, {}, &person::age);
  std::cout << "sorted by age:";
  for (const auto& [name, age] : people) {
    std::cout << ' ' << name << '(' << age << ')';
  }
  std::cout << '\n';
}

}  // namespace modules::ranges
