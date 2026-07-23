#include <iostream>
#include <string>

#include "lambda_demos.hpp"
#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};
  auto const message = "Hello from " + lib.name + "!";
  std::cout << message << '\n';

  // 按由浅入深的顺序依次运行各个 lambda 示例
  demo_basic_syntax();
  demo_capture_by_value();
  demo_capture_by_reference();
  demo_mutable_lambda();
  demo_init_capture();
  demo_generic_lambda();
  demo_with_stl_algorithms();
  demo_as_callback();
  demo_recursive_lambda();

  return 0;
}
