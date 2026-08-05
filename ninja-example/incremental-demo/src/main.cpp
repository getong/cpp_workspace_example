#include <iostream>

#include "greeting/greeting.hpp"
#include "mathlib/add.hpp"
#include "mathlib/mul.hpp"
#include "version.hpp"

auto main() -> int
{
  std::cout << make_greeting() << '\n';
  std::cout << "add(2, 3) = " << add(2, 3) << '\n';
  std::cout << "mul(4, 5) = " << mul(4, 5) << '\n';
  std::cout << "version   = " << demo_version << '\n';
  return 0;
}
