#pragma once

#include <iostream>

/**
 * @brief 模板接口（CRTP，奇异递归模板模式）
 *
 * 基类以派生类自身作为模板参数，在编译期即可知道 this 的真实类型，
 * 因此可以用 static_cast 直接调用派生类的实现，实现"静态多态"：
 * 无虚函数表、无运行时开销，调用可被内联。
 */
template<typename Derived>
class shape_interface
{
public:
  void draw()
  {
    // 编译期把基类指针转为派生类指针，直接调用派生类的方法
    static_cast<Derived*>(this)->draw_impl();
  }

private:
  // 构造函数设为私有并只对 Derived 开放，
  // 防止写出 class square : shape_interface<circle> 这类错误继承
  shape_interface() = default;
  friend Derived;
};

/**
 * @brief 具体实现类 A：圆形
 */
class circle : public shape_interface<circle>
{
public:
  // 示例未用到成员状态，真实实现通常会访问成员，故保留非静态
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void draw_impl() { std::cout << "画一个圆形 (circle)" << '\n'; }
};

/**
 * @brief 具体实现类 B：正方形
 */
class square : public shape_interface<square>
{
public:
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void draw_impl() { std::cout << "画一个正方形 (square)" << '\n'; }
};

/**
 * @brief 统一调用函数，接收任意符合接口规范的类型
 *
 * 每个 T 会实例化出一个独立的函数版本，draw() 在编译期就绑定到
 * 对应派生类的 draw_impl()，没有任何虚调用。
 */
template<typename T>
void render_shape(shape_interface<T>& shape)
{
  shape.draw();
}
