#include <iostream>
#include <string>

#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};
  auto const message = "Hello from " + lib.name + "!";
  std::cout << message << '\n';

  // Everything below is computed in Rust and reaches C++ through the C ABI
  // declared in rust_ffi.h (see rust/src/lib.rs for the implementation).
  constexpr auto augend = 40;
  constexpr auto addend = 2;
  constexpr auto fibonacci_index = 42U;
  std::cout << rust::greet(lib.name) << '\n';
  std::cout << "rust::add(" << augend << ", " << addend
            << ") = " << rust::add(augend, addend) << '\n';
  std::cout << "rust::fibonacci(" << fibonacci_index
            << ") = " << rust::fibonacci(fibonacci_index) << '\n';

  return 0;
}
