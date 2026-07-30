#pragma once

#include <iostream>

/**
 * @brief 方案三：C++23 deducing this（显式对象形参）—— 简化版 CRTP
 *
 * 成员函数写成 draw(this auto&& self) 后，self 的类型由调用方推导：
 * 通过 circle 对象调用时 self 就是 circle&，直接调用 circle::draw_impl()。
 * 效果与 CRTP 完全相同（编译期绑定、零开销），但基类不再需要模板参数，
 * 也不再需要 static_cast<Derived*>(this) 的样板代码。
 */
namespace via_deducing_this
{

// 注意：基类本身不是模板了！
class shape_interface
{
public:
  // self 的类型按调用方推导：circle 对象调用时即为 circle&
  void draw(this auto& self) { self.draw_impl(); }
};

class circle : public shape_interface
{
public:
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void draw_impl() { std::cout << "deducing this: 画一个圆形 (circle)" << '\n'; }
};

class square : public shape_interface
{
public:
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void draw_impl() { std::cout << "deducing this: 画一个正方形 (square)" << '\n'; }
};

// 基类不是模板，两个实现类共享同一个基类类型；
// 但 draw() 的静态绑定要求编译期知道真实类型，所以这里仍用模板转发
template<typename T>
void render_shape(T& shape)
{
  shape.draw();
}

}  // namespace via_deducing_this
