#pragma once

#include <iostream>

/**
 * @brief 方案二：C++20 concepts —— 用"约束"代替"继承"
 *
 * 与 CRTP 不同，实现类不需要继承任何基类，只要满足 drawable
 * 这个语义约束（有 draw() 成员函数）即可被 render_shape 接受。
 * 接口规范从"类型层级"变成了"编译期谓词"，报错信息也更友好：
 * 编译器会直接告诉你"类型 X 不满足 drawable，因为缺少 draw()"。
 */
namespace via_concepts
{

template<typename T>
concept drawable = requires(T shape) {
  { shape.draw() };
};

// 实现类完全独立，没有基类、没有模板参数，最"轻"
class circle
{
public:
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void draw() { std::cout << "concepts: 画一个圆形 (circle)" << '\n'; }
};

class square
{
public:
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void draw() { std::cout << "concepts: 画一个正方形 (square)" << '\n'; }
};

// 只有满足 drawable 约束的类型才能实例化；否则编译期报出清晰的错误
template<drawable T>
void render_shape(T& shape)
{
  shape.draw();
}

}  // namespace via_concepts
