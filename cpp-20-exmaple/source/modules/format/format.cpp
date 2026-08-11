#include "format.hpp"

#include <iostream>
#include <version>

// std::format 是 C++20 的标准特性，但部分标准库落地较晚；
// 用特性测试宏在旧环境下退回到 API 相同的 fmt 库。
#if defined(__cpp_lib_format)
#  include <format>
namespace fmtns = std;
#else
#  include <fmt/core.h>
namespace fmtns = fmt;
#endif

namespace modules::format
{

void run()
{
  constexpr double price = 1234.5678;

  // 格式串在编译期检查：占位符和实参类型不匹配会直接编译失败。
  std::cout << fmtns::format("plain: {} costs {}\n", "apple", price);

  // {:.2f} 控制精度，{:>12} 右对齐补空格，{:08.2f} 补零。
  std::cout << fmtns::format("fixed: {:.2f}\n", price);
  std::cout << fmtns::format("align: [{:>12.2f}]\n", price);
  std::cout << fmtns::format("zero pad: {:012.2f}\n", price);

  // 位置参数：同一个实参可以复用，也可以调整顺序。
  std::cout << fmtns::format("{0} {1}! ({1} {0}?)\n", "hello", "world");

  // 进制与字符输出。
  constexpr int answer = 42;
  std::cout << fmtns::format("dec={0} hex={0:#x} bin={0:#b}\n", answer);
}

}  // namespace modules::format
