#include "greeting/greeting.hpp"

#include "version.hpp"

auto make_greeting() -> std::string
{
  return std::string {"Hello from incremental-demo v"} + demo_version;
}
