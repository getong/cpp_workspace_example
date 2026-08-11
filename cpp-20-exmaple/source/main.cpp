#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

#include "modules/module.hpp"

namespace
{

void print_usage()
{
  std::cout << "Usage: cpp-20-exmaple <module>|all\n\nAvailable modules:\n";
  for (const auto& mod : modules::all()) {
    std::cout << "  " << mod.name << " - " << mod.description << '\n';
  }
}

void run_one(const modules::module& mod)
{
  std::cout << "==== [" << mod.name << "] ====\n";
  mod.run();
  std::cout << '\n';
}

}  // namespace

auto main(int argc, char* argv[]) -> int
{
  if (argc < 2) {
    print_usage();
    return EXIT_SUCCESS;
  }

  const std::span<char*> args {argv, static_cast<std::size_t>(argc)};
  const std::string_view request {args[1]};

  if (request == "all") {
    for (const auto& mod : modules::all()) {
      run_one(mod);
    }
    return EXIT_SUCCESS;
  }

  const auto* mod = modules::find(request);
  if (mod == nullptr) {
    std::cerr << "Unknown module: " << request << "\n\n";
    print_usage();
    return EXIT_FAILURE;
  }

  run_one(*mod);
  return EXIT_SUCCESS;
}
