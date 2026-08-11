#include "module.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include "concepts/concepts.hpp"
#include "coroutine/coroutine.hpp"
#include "designated_init/designated_init.hpp"
#include "format/format.hpp"
#include "hello/hello.hpp"
#include "ranges/ranges.hpp"
#include "span_usage/span_usage.hpp"
#include "three_way_compare/three_way_compare.hpp"

namespace modules
{

namespace
{

// 新增功能模块时，在这里追加一行即可。
constexpr std::array registry {
    module {"hello", "Print the project greeting from the core library",
            &hello::run},
    module {"span_usage",
            "std::span as a unified view over arrays and vectors",
            &span_usage::run},
    module {"concepts", "Constrain templates with concepts and requires",
            &concepts::run},
    module {"ranges", "Lazy view pipelines and range-based algorithms",
            &ranges::run},
    module {"coroutine", "A co_yield based lazy generator", &coroutine::run},
    module {"three_way_compare",
            "Defaulted operator<=> gives all six comparisons",
            &three_way_compare::run},
    module {"designated_init",
            "Designated initializers for aggregate types",
            &designated_init::run},
    module {"format", "Type-safe text formatting with std::format",
            &format::run},
};

}  // namespace

auto all() -> std::span<const module>
{
  return registry;
}

auto find(std::string_view name) -> const module*
{
  const auto* iter = std::ranges::find(registry, name, &module::name);
  return iter != registry.end() ? iter : nullptr;
}

}  // namespace modules
