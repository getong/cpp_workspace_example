#pragma once

#include <span>
#include <string_view>

namespace modules
{

/**
 * @brief Describes one runnable feature module of the project.
 *
 * Every feature lives in its own directory under source/modules/ and exposes
 * a run() entry point. New modules only need to be appended to the list in
 * registry.cpp.
 */
struct module
{
  std::string_view name;
  std::string_view description;
  void (*run)();
};

/**
 * @brief All registered modules, in registration order.
 */
auto all() -> std::span<const module>;

/**
 * @brief Finds a module by name, or returns nullptr when unknown.
 */
auto find(std::string_view name) -> const module*;

}  // namespace modules
