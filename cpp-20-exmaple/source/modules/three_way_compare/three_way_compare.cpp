#include "three_way_compare.hpp"

#include <algorithm>
#include <compare>
#include <iostream>
#include <vector>

namespace modules::three_way_compare
{

namespace
{

struct version
{
  int major;
  int minor;
  int patch;

  // = default 让编译器按成员声明顺序做字典序比较，
  // 同时自动获得 ==、!=、<、<=、>、>= 六种运算符。
  auto operator<=>(const version&) const = default;
};

void print(const version& ver)
{
  std::cout << ver.major << '.' << ver.minor << '.' << ver.patch;
}

}  // namespace

void run()
{
  const version lhs {.major = 1, .minor = 2, .patch = 3};
  const version rhs {.major = 1, .minor = 10, .patch = 0};

  // <=> 返回的不是 bool，而是序类别（这里是 std::strong_ordering）。
  const auto order = lhs <=> rhs;
  std::cout << "1.2.3 <=> 1.10.0 : ";
  if (order < 0) {
    std::cout << "less\n";
  } else if (order > 0) {
    std::cout << "greater\n";
  } else {
    std::cout << "equal\n";
  }

  std::cout << std::boolalpha;
  std::cout << "lhs < rhs  = " << (lhs < rhs) << '\n';
  std::cout << "lhs == lhs = "
            << (lhs == version {.major = 1, .minor = 2, .patch = 3}) << '\n';

  // 有了 <=>，标准算法的排序、比较都直接可用。
  std::vector<version> versions {
      {.major = 2, .minor = 0, .patch = 0},
      {.major = 1, .minor = 2, .patch = 3},
      {.major = 1, .minor = 10, .patch = 0},
  };
  std::ranges::sort(versions);
  std::cout << "sorted:";
  for (const auto& ver : versions) {
    std::cout << ' ';
    print(ver);
  }
  std::cout << '\n';
}

}  // namespace modules::three_way_compare
