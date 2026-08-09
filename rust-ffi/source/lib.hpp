#pragma once

#include <cstdint>
#include <string>
#include <string_view>

/**
 * @brief The core implementation of the executable
 *
 * This class makes up the library part of the executable, which means that the
 * main logic is implemented here. This kind of separation makes it easy to
 * test the implementation for the executable, because the logic is nicely
 * separated from the command-line logic implemented in the main function.
 */
struct library
{
  /**
   * @brief Simply initializes the name member to the name of the project
   */
  library();

  std::string name;
};

/**
 * @brief C++ wrappers around the Rust core library
 *
 * The Rust implementation lives in rust/src/lib.rs and is linked in as a
 * static library. The boundary is a cxx bridge (https://cxx.rs): the header
 * rust_bridge/lib.h is generated from the #[cxx::bridge] module at build
 * time, so the two sides cannot drift apart. These wrappers only translate
 * between cxx's rust::Str/rust::String and std types, keeping the generated
 * header a private implementation detail of lib.cpp.
 */
namespace rust
{

/// Adds two integers in Rust, saturating on overflow.
auto add(std::int32_t lhs, std::int32_t rhs) -> std::int32_t;

/// Computes the n-th Fibonacci number in Rust.
auto fibonacci(std::uint32_t nth) -> std::uint64_t;

/// Returns a greeting composed by Rust for the given name.
auto greet(std::string_view name) -> std::string;

}  // namespace rust
