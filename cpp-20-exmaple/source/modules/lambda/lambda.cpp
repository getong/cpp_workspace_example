#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "lambda.hpp"

namespace modules::lambda
{

namespace
{

// ---- 1. 基本语法与捕获方式 -------------------------------------------------

void basic_and_captures()
{
  std::cout << "-- basic & captures --\n";

  // 最简形式：无捕获、无参数。
  auto hello = [] { std::cout << "hello lambda\n"; };
  hello();

  // 显式参数与尾置返回类型（返回类型通常可省略，由 return 推导）。
  auto add = [](int a, int b) -> int { return a + b; };
  std::cout << "add(1, 2) = " << add(1, 2) << '\n';

  int base = 10;
  std::string tag = "tag";

  // 按值捕获：拷贝一份，之后外部修改不影响 lambda 内部看到的值。
  auto by_value = [base] { return base; };
  // 按引用捕获：始终看到外部变量的最新值（注意悬垂引用风险）。
  auto by_ref = [&base] { return base; };
  base = 20;
  std::cout << "by_value() = " << by_value() << ", by_ref() = " << by_ref()
            << '\n';

  // [=] 按值捕获所有用到的变量，[&] 按引用捕获所有用到的变量，也可混用：
  // [=, &base] 表示默认按值，仅 base 按引用。
  auto mixed = [=, &base] { return tag + ":" + std::to_string(base); };
  std::cout << "mixed() = " << mixed() << '\n';

  // 初始化捕获（C++14）：在捕获列表里直接定义新变量，
  // 常用于移动 move-only 对象进 lambda。
  auto owner = [ptr = std::make_unique<int>(42)] { return *ptr; };
  std::cout << "owner() = " << owner() << '\n';

  // 初始化捕获也可以做计算，捕获的名字和外部变量无关。
  auto doubled = [twice = base * 2] { return twice; };
  std::cout << "doubled() = " << doubled() << '\n';
}

// ---- 2. mutable：修改按值捕获的副本 ----------------------------------------

void mutable_lambda()
{
  std::cout << "-- mutable --\n";

  // 按值捕获默认是 const 的，加 mutable 才能修改副本（不影响外部变量）。
  int counter = 0;
  auto next = [counter]() mutable { return ++counter; };
  std::cout << "next() = " << next() << ", next() = " << next()
            << ", outer counter = " << counter << '\n';
}

// ---- 3. 泛型 lambda 与 C++20 模板 lambda -----------------------------------

void generic_and_template()
{
  std::cout << "-- generic & template lambda --\n";

  // 泛型 lambda（C++14）：auto 参数，等价于成员函数模板。
  auto plus = [](auto a, auto b) { return a + b; };
  std::cout << "plus(1, 2) = " << plus(1, 2)
            << ", plus(str) = " << plus(std::string {"a"}, std::string {"b"})
            << '\n';

  // C++20 模板 lambda：可以显式命名类型参数并加约束，
  // 还能拿到容器的嵌套类型（auto 参数做不到这一点）。
  auto first_or = []<typename T>(const std::vector<T>& vec, T fallback)
  { return vec.empty() ? fallback : vec.front(); };
  std::cout << "first_or({7, 8}, -1) = " << first_or(std::vector {7, 8}, -1)
            << ", first_or({}, -1) = " << first_or(std::vector<int> {}, -1)
            << '\n';

  // 变参模板 lambda + 完美转发。
  auto call = []<typename F, typename... Args>(F&& func, Args&&... args)
  { return std::forward<F>(func)(std::forward<Args>(args)...); };
  std::cout << "call(plus, 3, 4) = " << call(plus, 3, 4) << '\n';
}

// ---- 4. constexpr lambda 与函数指针转换 ------------------------------------

void constexpr_and_function_pointer()
{
  std::cout << "-- constexpr & function pointer --\n";

  // lambda 默认就是隐式 constexpr（只要函数体允许），可用于编译期计算。
  constexpr auto square = [](int x) { return x * x; };
  static_assert(square(9) == 81);
  std::array<int, square(3)> arr {};
  std::cout << "array size from constexpr lambda = " << arr.size() << '\n';

  // 无捕获 lambda 可以隐式转换为普通函数指针，方便对接 C 风格 API。
  int (*fptr)(int) = square;
  std::cout << "fptr(5) = " << fptr(5) << '\n';
}

// ---- 5. IIFE：立即调用的 lambda --------------------------------------------

void immediately_invoked()
{
  std::cout << "-- IIFE --\n";

  // 常用于“复杂初始化后即为 const”的场景。
  const std::vector<int> evens = []
  {
    std::vector<int> result;
    for (int i = 0; i < 10; i += 2) {
      result.push_back(i);
    }
    return result;
  }();
  std::cout << "evens.size() = " << evens.size() << '\n';
}

// ---- 6. 递归 lambda --------------------------------------------------------

void recursive_lambda()
{
  std::cout << "-- recursion --\n";

  // 经典技巧：lambda 无法直接引用自身，把自己作为参数传入即可
  // （C++23 的 deducing this 可以更直接地写递归）。
  auto fib = [](auto self, int n) -> int
  { return n < 2 ? n : self(self, n - 1) + self(self, n - 2); };
  std::cout << "fib(10) = " << fib(fib, 10) << '\n';

  // 另一种方式：std::function 有名字，可以在体内引用，但有类型擦除开销。
  std::function<int(int)> fact = [&fact](int n)
  { return n <= 1 ? 1 : n * fact(n - 1); };
  std::cout << "fact(5) = " << fact(5) << '\n';
}

// ---- 7. 与标准算法配合 -----------------------------------------------------

void with_algorithms()
{
  std::cout << "-- algorithms --\n";

  std::vector nums {5, 2, 8, 1, 9, 3};

  // 作为比较器：按降序排序。
  std::ranges::sort(nums, [](int a, int b) { return a > b; });
  std::cout << "sorted desc, first = " << nums.front() << '\n';

  // 作为谓词：统计满足条件的元素。
  const auto big = std::ranges::count_if(nums, [](int n) { return n > 4; });
  std::cout << "count > 4: " << big << '\n';

  // 捕获外部状态的谓词。
  int threshold = 3;
  const auto found =
      std::ranges::find_if(nums, [threshold](int n) { return n < threshold; });
  std::cout << "first < 3: " << (found != nums.end() ? *found : -1) << '\n';
}

// ---- 8. C++20 新特性：无状态 lambda 可默认构造 / 用于未求值语境 ------------

void cpp20_stateless()
{
  std::cout << "-- C++20 stateless lambda --\n";

  // C++20 起无捕获 lambda 可默认构造、可赋值，
  // 因此可以只用“类型”来定制容器行为，如 decltype(lambda) 作比较器。
  auto desc = [](int a, int b) { return a > b; };
  std::set<int, decltype(desc)> ordered {3, 1, 2};  // 无需传入 lambda 对象
  std::cout << "set first (desc) = " << *ordered.begin() << '\n';
}

// ---- 9. 类成员中捕获 this --------------------------------------------------

class accumulator
{
public:
  auto make_adder()
  {
    // 捕获 this：lambda 内可访问成员。C++20 要求显式写出 [=, this]
    // （隐式 [=] 捕获 this 已弃用）；[*this] 则拷贝整个对象，更安全。
    return [this](int n)
    {
      total_ += n;
      return total_;
    };
  }

  auto make_snapshot() const
  {
    return [*this] { return total_; };  // 持有对象副本，不怕悬垂
  }

private:
  int total_ = 0;
};

void capture_this()
{
  std::cout << "-- capture this --\n";

  accumulator acc;
  auto adder = acc.make_adder();
  adder(10);
  const auto snapshot = acc.make_snapshot();  // 记录此刻的副本
  adder(5);
  std::cout << "adder total = " << adder(0)
            << ", snapshot (taken at 10) = " << snapshot() << '\n';
}

}  // namespace

void run()
{
  basic_and_captures();
  mutable_lambda();
  generic_and_template();
  constexpr_and_function_pointer();
  immediately_invoked();
  recursive_lambda();
  with_algorithms();
  cpp20_stateless();
  capture_this();
}

}  // namespace modules::lambda
