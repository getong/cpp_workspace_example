#include "span_usage.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <span>
#include <vector>

namespace modules::span_usage
{

namespace
{

void process(std::span<const int> data)
{
  std::cout << "Size: " << data.size() << ", values:";
  for (const int value : data) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';
}

// 兼容“指针 + 长度”形式，实际处理仍由 span 版本完成。
void process(const int* ptr, std::size_t size)
{
  process(std::span<const int> {ptr, size});
}

// 兼容只接收 vector 的旧接口，构造 span 不会拷贝元素。
void process(const std::vector<int>& vec)
{
  process(std::span<const int> {vec});
}

}  // namespace

void run()
{
  const int c_arr[] = {1, 2, 3};
  const std::vector<int> vec = {4, 5, 6};
  const std::array<int, 3> arr = {7, 8, 9};

  std::cout << "pointer + size overload:\n";
  process(c_arr, std::size(c_arr));

  std::cout << "vector overload:\n";
  process(vec);

  std::cout << "span interface:\n";
  process(c_arr);  // span 可以直接接收原生数组
  process(arr);  // span 可以直接接收 std::array

  const std::span<const int> vec_view {vec};
  process(vec_view.subspan(1));  // 视图不拷贝数据
}

}  // namespace modules::span_usage
