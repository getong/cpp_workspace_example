#pragma once

#include <string>

/**
 * @brief Demo hierarchy for C++ casts across a virtual template class.
 *
 * Inheritance chain (4 levels):
 *
 *   shape                      -- polymorphic root (virtual dtor)
 *     └─ sized_shape<T>        -- *virtual template* intermediate class
 *          └─ polygon<T>       -- third level, still a template
 *               └─ rectangle   -- concrete class (T = double)
 *
 * The interesting part: given only a `shape*`, dynamic_cast can recover the
 * intermediate template class `sized_shape<double>*` or `polygon<double>*`,
 * but a cast to `sized_shape<int>*` fails at runtime (returns nullptr),
 * because a template instantiated with a different argument is an unrelated
 * type.
 */

// Level 1: polymorphic base. A virtual function (here the destructor) is
// required for dynamic_cast to work on this hierarchy (RTTI).
class shape
{
public:
  shape() = default;
  shape(shape const&) = default;
  shape(shape&&) = default;
  auto operator=(shape const&) -> shape& = default;
  auto operator=(shape&&) -> shape& = default;
  virtual ~shape() = default;

  virtual auto name() const -> std::string = 0;
};

// Level 2: the "virtual template" intermediate class. It is a class template
// that adds its own virtual interface on top of shape.
template<typename T>
class sized_shape : public shape
{
public:
  virtual auto area() const -> T = 0;
};

// Level 3: still a template, adds more virtual interface.
template<typename T>
class polygon : public sized_shape<T>
{
public:
  virtual auto sides() const -> int = 0;
};

// Level 4: concrete class, fixes the template argument to double.
class rectangle : public polygon<double>
{
public:
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  rectangle(double width, double height)
      : m_width {width}
      , m_height {height}
  {
  }

  auto name() const -> std::string override { return "rectangle"; }

  auto area() const -> double override { return m_width * m_height; }

  auto sides() const -> int override { return 4; }

private:
  double m_width;
  double m_height;
};
