#include "lambda_demos.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 1. 基本语法
// ---------------------------------------------------------------------------
void demo_basic_syntax()
{
  std::cout << "\n===== 1. 基本语法 =====\n";

  // 最简单的 lambda：[] 空捕获列表，() 无参数，{} 函数体。
  // 定义后存入变量 hello，之后像普通函数一样用 () 调用。
  auto hello = []() { std::cout << "hello lambda\n"; };
  hello();

  // 带参数和返回值：返回类型由 return 语句自动推导（这里是 int）。
  auto add = [](int a, int b) { return a + b; };
  std::cout << "add(3, 4) = " << add(3, 4) << '\n';

  // 用 -> 显式指定返回类型：当函数体里有多条 return 语句、
  // 推导结果不一致（如 int 与 double 混用）时必须显式写出。
  auto divide = [](int a, int b) -> double {
    if (b == 0) { return 0; }  // int 字面量
    return static_cast<double>(a) / b;  // double
  };
  std::cout << "divide(7, 2) = " << divide(7, 2) << '\n';

  // 立即调用（IIFE, Immediately Invoked Function Expression）：
  // 定义完马上加 () 调用，常用于"用一段逻辑初始化 const 变量"。
  const int table_size = [] {
    int n = 1;
    while (n < 100) { n *= 2; }
    return n;
  }();
  std::cout << "IIFE 计算出的 table_size = " << table_size << '\n';
}

// ---------------------------------------------------------------------------
// 2. 值捕获
// ---------------------------------------------------------------------------
void demo_capture_by_value()
{
  std::cout << "\n===== 2. 值捕获 =====\n";

  int base = 10;

  // [base]：把 base 拷贝一份存进闭包对象。
  // 捕获发生在"定义 lambda 的那一刻"，之后外部再怎么改都影响不到副本。
  auto add_base = [base](int x) { return base + x; };

  base = 999;  // 修改外部变量，不影响 lambda 里的副本
  std::cout << "add_base(5) = " << add_base(5) << "  (仍用捕获时的 base=10)\n";

  // [=]：按值捕获函数体里用到的"所有"外部变量（只拷贝用到的）。
  int a = 1;
  int b = 2;
  auto sum_all = [=]() { return a + b + base; };
  std::cout << "sum_all() = " << sum_all() << '\n';
}

// ---------------------------------------------------------------------------
// 3. 引用捕获
// ---------------------------------------------------------------------------
void demo_capture_by_reference()
{
  std::cout << "\n===== 3. 引用捕获 =====\n";

  int counter = 0;

  // [&counter]：闭包里存的是 counter 的引用，
  // lambda 内的修改直接作用于外部变量——常用来做累加器。
  auto increment = [&counter]() { ++counter; };
  increment();
  increment();
  increment();
  std::cout << "调用 increment 三次后 counter = " << counter << '\n';

  // [&]：按引用捕获用到的所有外部变量。
  std::vector<int> nums {1, 2, 3, 4, 5};
  int total = 0;
  std::for_each(nums.begin(), nums.end(), [&](int n) { total += n; });
  std::cout << "for_each 求和 total = " << total << '\n';

  // 注意：引用捕获的生命周期风险！
  // 如果 lambda 的存活时间比被捕获的局部变量长（例如存入成员变量、
  // 传给另一个线程、从函数 return 出去），引用就会悬垂（dangling），
  // 造成未定义行为。这种场景应改用值捕获或 move 捕获。
}

// ---------------------------------------------------------------------------
// 4. mutable
// ---------------------------------------------------------------------------
void demo_mutable_lambda()
{
  std::cout << "\n===== 4. mutable =====\n";

  // 闭包类的 operator() 默认是 const 成员函数，
  // 所以值捕获的副本在函数体内默认"只读"。
  // 加上 mutable 后 operator() 不再是 const，副本就可以修改了。
  // 注意：修改的只是闭包内部的副本，外部变量不受影响。
  int start = 100;
  auto ticket = [start]() mutable { return start++; };

  std::cout << "ticket() = " << ticket() << '\n';  // 100
  std::cout << "ticket() = " << ticket() << '\n';  // 101（闭包内副本在自增）
  std::cout << "外部 start 仍是 " << start << '\n';  // 100
}

// ---------------------------------------------------------------------------
// 5. 初始化捕获（C++14，又叫广义捕获 / generalized capture）
// ---------------------------------------------------------------------------
void demo_init_capture()
{
  std::cout << "\n===== 5. 初始化捕获 =====\n";

  // [名字 = 表达式]：在捕获列表里"就地定义"一个闭包成员变量，
  // 表达式可以是任意计算，不要求外部存在同名变量。
  int x = 4;
  auto squared_plus = [sq = x * x](int y) { return sq + y; };
  std::cout << "squared_plus(1) = " << squared_plus(1) << '\n';

  // 最重要的用途：move 捕获。
  // 普通值捕获只能"拷贝"，而 unique_ptr 这类只移动类型无法拷贝，
  // C++11 里没法捕获；C++14 用 [p = std::move(ptr)] 把所有权
  // 转移进闭包，lambda 就"独占"了这个资源。
  auto ptr = std::make_unique<std::string>("独占资源");
  auto owner = [p = std::move(ptr)]() { std::cout << "闭包持有: " << *p << '\n'; };
  owner();
  std::cout << "move 之后外部 ptr 为空: " << std::boolalpha << (ptr == nullptr)
            << '\n';
}

// ---------------------------------------------------------------------------
// 6. 泛型 lambda
// ---------------------------------------------------------------------------
void demo_generic_lambda()
{
  std::cout << "\n===== 6. 泛型 lambda =====\n";

  // C++14：参数写 auto，编译器把 operator() 生成为模板，
  // 一个 lambda 就能接受任意可相加的类型。
  auto plus = [](auto a, auto b) { return a + b; };
  std::cout << "plus(1, 2) = " << plus(1, 2) << '\n';
  std::cout << "plus(1.5, 2.25) = " << plus(1.5, 2.25) << '\n';
  std::cout << "plus(string) = " << plus(std::string {"foo"}, std::string {"bar"})
            << '\n';

  // C++20：lambda 也能写模板参数列表，比纯 auto 表达力更强——
  // 这里约束两个参数必须是"同一个类型 T"。
  auto same_type_max = []<typename T>(T a, T b) { return a > b ? a : b; };
  std::cout << "same_type_max(3, 7) = " << same_type_max(3, 7) << '\n';
}

// ---------------------------------------------------------------------------
// 7. 与 STL 算法配合 —— lambda 最高频的实战场景
// ---------------------------------------------------------------------------
void demo_with_stl_algorithms()
{
  std::cout << "\n===== 7. 配合 STL 算法 =====\n";

  std::vector<int> nums {5, 2, 8, 1, 9, 3};

  // 自定义排序规则：lambda 作为比较器，替代手写仿函数（functor）类。
  // 在 lambda 出现之前，要为 sort 写比较逻辑必须单独定义一个
  // 带 operator() 的结构体；lambda 让逻辑直接内联在调用点，可读性大增。
  std::sort(nums.begin(), nums.end(), [](int a, int b) { return a > b; });
  std::cout << "降序排序: ";
  for (int n : nums) { std::cout << n << ' '; }
  std::cout << '\n';

  // 条件计数：谓词（predicate）lambda，可捕获外部阈值形成"参数化的条件"。
  int threshold = 4;
  auto big_count = std::count_if(
      nums.begin(), nums.end(), [threshold](int n) { return n > threshold; });
  std::cout << "大于 " << threshold << " 的元素个数: " << big_count << '\n';

  // 逐元素变换：transform + lambda 实现 map 语义。
  std::vector<int> doubled(nums.size());
  std::transform(
      nums.begin(), nums.end(), doubled.begin(), [](int n) { return n * 2; });
  std::cout << "每个元素翻倍: ";
  for (int n : doubled) { std::cout << n << ' '; }
  std::cout << '\n';
}

// ---------------------------------------------------------------------------
// 8. 作为回调传递
// ---------------------------------------------------------------------------

// std::function 是"类型擦除"的可调用对象容器：
// 任何签名匹配的 lambda（无论捕获了什么）都能存进来。
// 适合做成员变量、容器元素等需要统一类型的场合；代价是可能有堆分配
// 和间接调用开销。
static void run_with_callback(int value, const std::function<void(int)>& on_done)
{
  on_done(value * value);
}

// 函数指针版本：只有"无捕获"的 lambda 能隐式转换成函数指针，
// 因为无捕获意味着闭包没有状态，可退化为普通函数。
// 这也是 lambda 能直接传给 C 风格 API（如 qsort、信号处理）的原因。
static void run_with_fn_pointer(int value, void (*on_done)(int))
{
  on_done(value + 1);
}

void demo_as_callback()
{
  std::cout << "\n===== 8. 作为回调 =====\n";

  std::string tag = "[callback]";

  // 有捕获的 lambda → 只能用 std::function（或模板参数）接收
  run_with_callback(
      6, [tag](int result) { std::cout << tag << " 收到结果: " << result << '\n'; });

  // 无捕获的 lambda → 可以隐式转换为函数指针
  run_with_fn_pointer(
      41, [](int result) { std::cout << "[fn ptr] 收到结果: " << result << '\n'; });
}

// ---------------------------------------------------------------------------
// 9. 递归 lambda
// ---------------------------------------------------------------------------
void demo_recursive_lambda()
{
  std::cout << "\n===== 9. 递归 lambda =====\n";

  // lambda 没有名字，函数体内无法直接"叫自己"，所以递归需要技巧。

  // 方法一：存入 std::function，再按引用捕获它——
  // 定义时 factorial 这个名字已存在，函数体里就能调用它了。
  std::function<long long(int)> factorial = [&factorial](int n) -> long long {
    return n <= 1 ? 1 : n * factorial(n - 1);
  };
  std::cout << "factorial(10) = " << factorial(10) << '\n';

  // 方法二：把"自己"作为参数传进去（泛型 lambda 技巧），
  // 没有 std::function 的类型擦除开销，可被编译器完全内联。
  // （C++23 的 deducing this 让这一写法更简洁：this auto self）
  auto fib = [](auto self, int n) -> long long {
    return n < 2 ? n : self(self, n - 1) + self(self, n - 2);
  };
  std::cout << "fib(20) = " << fib(fib, 20) << '\n';
}
