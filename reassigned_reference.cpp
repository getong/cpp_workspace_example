#include <iostream>
#include <functional>

int main() {
  int a = 10;
  int b = 20;

  std::reference_wrapper<int> ref = a;
  ref += 5;
  ref = b; // ref now effectively references b

  ref.get() = 30; // changes b to 30

  std::cout << "a: " << a << std::endl; // Output will still be 10
  std::cout << "b: " << b << std::endl; // Output will be 30

  return 0;
}


// clang++ -std=c++20 reassigned_reference.cpp