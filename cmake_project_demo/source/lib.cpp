#include "lib.hpp"

#include <fmt/core.h>

library::library()
    : name {fmt::format("{}", "cmake_project_demo")}
{
}
