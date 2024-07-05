#include <iostream>
#include <thread>

void threadFunction() { std::cout << "Hello from the thread!" << std::endl; }

int main() {
  std::thread t{threadFunction};
  t.join();
  return 0;
}

// g++ -std=c++20 cpp_basic_thread.cpp
// clang++ -std=c++20 cpp_basic_thread.cpp