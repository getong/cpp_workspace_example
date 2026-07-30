#include <iostream>
#include <string>

#include "lib.hpp"
#include "shape_concepts.hpp"
#include "shape_deducing_this.hpp"
#include "shape_interface.hpp"

auto main() -> int
{
  auto const lib = library {};
  auto const message = "Hello from " + lib.name + "!";
  std::cout << message << '\n';

  // 方案一：CRTP（模板接口）
  circle my_circle;
  square my_square;
  render_shape(my_circle);  // 输出：画一个圆形 (circle)
  render_shape(my_square);  // 输出：画一个正方形 (square)

  // 方案二：C++20 concepts，无继承，靠约束规范接口
  via_concepts::circle concept_circle;
  via_concepts::square concept_square;
  via_concepts::render_shape(concept_circle);
  via_concepts::render_shape(concept_square);

  // 方案三：C++23 deducing this，基类无需模板参数
  via_deducing_this::circle deduced_circle;
  via_deducing_this::square deduced_square;
  via_deducing_this::render_shape(deduced_circle);
  via_deducing_this::render_shape(deduced_square);

  return 0;
}
