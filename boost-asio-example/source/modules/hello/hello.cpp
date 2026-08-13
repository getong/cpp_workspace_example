#include <iostream>
#include <string>

#include "hello.hpp"

#include "lib.hpp"

namespace modules::hello
{

void run()
{
  auto const lib = library {};
  auto const message = "Hello from " + lib.name + "!";
  std::cout << message << '\n';
}

}  // namespace modules::hello
