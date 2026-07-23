#pragma once

/**
 * @brief C++ Lambda 表达式（匿名函数）示例集合
 *
 * Lambda 表达式的完整语法：
 *
 *     [捕获列表] (参数列表) mutable noexcept -> 返回类型 { 函数体 }
 *
 * 其中只有 [捕获列表] 和 { 函数体 } 是必须的，最简形式就是 [](){}。
 *
 * Lambda 的本质：编译器会为每个 lambda 生成一个独一无二的"闭包类型"
 * （closure type），捕获的变量成为该类的成员变量，函数体成为它的
 * operator() 重载。因此 lambda 是"带状态的可调用对象"，这是它与
 * 普通函数指针最大的区别。
 *
 * 每个 demo_xxx 函数演示一个独立的知识点，按由浅入深的顺序排列。
 */

/// 1. 基本语法：无捕获、有参数、显式/推导返回类型、立即调用（IIFE）
void demo_basic_syntax();

/// 2. 值捕获 [x] / [=]：拷贝一份外部变量，lambda 内外互不影响
void demo_capture_by_value();

/// 3. 引用捕获 [&x] / [&]：lambda 内可读写外部变量，注意悬垂引用风险
void demo_capture_by_reference();

/// 4. mutable：允许修改"值捕获"进来的副本（默认 operator() 是 const）
void demo_mutable_lambda();

/// 5. 初始化捕获（C++14）：在捕获列表里定义新变量，支持 move 捕获
void demo_init_capture();

/// 6. 泛型 lambda：auto 参数（C++14）与模板参数列表（C++20）
void demo_generic_lambda();

/// 7. 与 STL 算法配合：sort / count_if / transform 等的自定义逻辑
void demo_with_stl_algorithms();

/// 8. 作为回调传递：std::function、函数模板参数、无捕获转函数指针
void demo_as_callback();

/// 9. 递归 lambda：借助 std::function 或"自引用参数"技巧实现
void demo_recursive_lambda();
