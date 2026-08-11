#include "concepts.hpp"

#include <concepts>
#include <cstddef>
#include <iostream>
#include <string>

namespace modules::concepts
{

namespace
{

// concept 是对模板参数的“编译期谓词”，可以直接组合标准库里的概念。
template<typename T>
concept numeric = std::integral<T> || std::floating_point<T>;

// requires 表达式：要求类型必须有 .size()，且返回值可转换为 std::size_t。
template<typename T>
concept has_size = requires(const T& value) {
  { value.size() } -> std::convertible_to<std::size_t>;
};

// 用法一：concept 直接放在模板参数列表里。
template<numeric T>
auto twice(T value) -> T
{
  return value + value;
}

// 用法二：简写函数模板，concept 修饰 auto 参数。
void print_size(const has_size auto& container)
{
  std::cout << "size = " << container.size() << '\n';
}

}  // namespace

void run()
{
  std::cout << "twice(21) = " << twice(21) << '\n';
  std::cout << "twice(1.5) = " << twice(1.5) << '\n';
  // twice(std::string {"a"});  // 编译错误：std::string 不满足 numeric 约束

  print_size(std::string {"hello"});

  // concept 本身就是一个 bool 常量表达式，可以直接求值。
  std::cout << std::boolalpha;
  std::cout << "numeric<int> = " << numeric<int> << '\n';
  std::cout << "numeric<std::string> = " << numeric<std::string> << '\n';
}

}  // namespace modules::concepts
