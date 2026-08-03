#include <iostream>
#include <memory>

#include "cast.hpp"
#include "lib.hpp"

namespace
{

// Only the root of the hierarchy is visible here: the caller has erased the
// concrete type behind a shape*.
void inspect(shape& erased)
{
  std::cout << "static type at call site: shape&, dynamic type: "
            << erased.name() << '\n';

  // (1) dynamic_cast down to the *intermediate* template class (level 2).
  // This is a checked downcast: it succeeds because the object really is a
  // rectangle, which derives (through polygon<double>) from
  // sized_shape<double>.
  if (auto* sized = dynamic_cast<sized_shape<double>*>(&erased)) {
    std::cout << "  dynamic_cast<sized_shape<double>*> ok, area = "
              << sized->area() << '\n';
  }

  // (2) dynamic_cast to level 3, one step further down the chain.
  if (auto* poly = dynamic_cast<polygon<double>*>(&erased)) {
    std::cout << "  dynamic_cast<polygon<double>*>   ok, sides = "
              << poly->sides() << '\n';

    // (3) Upcast from level 3 back to level 2 needs no cast at all — a
    // derived* converts to base* implicitly. static_cast just makes it
    // explicit; it is resolved entirely at compile time.
    sized_shape<double>* implicit_up = poly;  // implicit upcast
    auto* explicit_up = static_cast<sized_shape<double>*>(poly);
    std::cout << "  upcast polygon -> sized_shape:   " << implicit_up->area()
              << " == " << explicit_up->area() << '\n';
  }

  // (4) A different template argument is a completely unrelated type, even
  // though it comes from the same template: the cast fails and yields
  // nullptr instead of a wrong pointer.
  if (dynamic_cast<sized_shape<int>*>(&erased) == nullptr) {
    std::cout << "  dynamic_cast<sized_shape<int>*>  failed as expected "
                 "(different template argument)\n";
  }

  // (5) Reference form: failure throws std::bad_cast instead of returning
  // nullptr.
  try {
    auto& bad = dynamic_cast<sized_shape<int>&>(erased);
    static_cast<void>(bad);
  } catch (std::bad_cast const& ex) {
    std::cout << "  dynamic_cast<sized_shape<int>&>  threw " << ex.what()
              << '\n';
  }

  // (6) static_cast *down* the chain compiles without any runtime check, so
  // it is only safe when you already know the dynamic type. Here we know it
  // is a rectangle, so this is fine — but had we guessed wrong, this would
  // be undefined behavior, unlike dynamic_cast.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& rect = static_cast<rectangle&>(erased);
  std::cout << "  static_cast<rectangle&>          (unchecked) area = "
            << rect.area() << '\n';
}

}  // namespace

auto main() -> int
{
  auto const lib = library {};
  std::cout << "Hello from " << lib.name << "!\n\n";

  // The concrete object lives at level 4; we hand it around as shape* only.
  constexpr double width = 3.0;
  constexpr double height = 4.0;
  std::unique_ptr<shape> erased = std::make_unique<rectangle>(width, height);
  inspect(*erased);

  return 0;
}
