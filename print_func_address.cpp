#include <iostream>

// 定义三个简单的函数

void functionA() { std::cout << "函数A" << std::endl; }

void functionB(int x) { std::cout << "函数B,x=" << x << std::endl; }

void functionC(int x, int y) {
  std::cout << "函数C,x=" << x << ",y=" << y << std::endl;
}

int main() {
  // 将三个简单的函数转换为 void* 指针，然后打印其地址
  std::cout << "函数A，内存地址：" << reinterpret_cast<void *>(functionA)
            << std::endl;
  std::cout << "函数B，内存地址：" << reinterpret_cast<void *>(functionB)
            << std::endl;
  std::cout << "函数C，内存地址：" << reinterpret_cast<void *>(functionC)
            << std::endl;

  return 0;
}

// g++ -std=c++20 print_func_address.cpp
// clang++ -std=c++20 print_func_address.cpp
// copy from https://zhuanlan.zhihu.com/p/688548252
