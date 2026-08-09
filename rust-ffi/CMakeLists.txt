cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    rust-ffi
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    rust-ffi_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    rust-ffi_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(rust-ffi_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(rust-ffi_lib PRIVATE fmt::fmt)

# ---- Rust core library (via Corrosion) ----

# Corrosion drives `cargo build` from CMake and imports the resulting
# staticlib as a regular CMake target named after the crate's [lib] name.
include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5.2
)
FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(
    MANIFEST_PATH rust/Cargo.toml
    CRATE_TYPES staticlib
)

# Generate the C++ side of the #[cxx::bridge] in rust/src/lib.rs. This
# creates a static library `rust_bridge` holding the generated bridge code
# (header: rust_bridge/lib.h) which links the Rust staticlib. If no matching
# `cxxbridge` binary is on PATH, Corrosion cargo-installs one at build time.
corrosion_add_cxxbridge(
    rust_bridge
    CRATE rust_ffi_core
    FILES lib.rs
)

# PRIVATE on an object library still propagates the archives to final link
# lines of consumers (rust-ffi_exe, tests) via $<LINK_ONLY:...>.
target_link_libraries(rust-ffi_lib PRIVATE rust_bridge)

# ---- Declare executable ----

add_executable(rust-ffi_exe source/main.cpp)
add_executable(rust-ffi::exe ALIAS rust-ffi_exe)

set_property(TARGET rust-ffi_exe PROPERTY OUTPUT_NAME rust-ffi)

target_compile_features(rust-ffi_exe PRIVATE cxx_std_20)

target_link_libraries(rust-ffi_exe PRIVATE rust-ffi_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT rust-ffi_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of rust-ffi"
  )
endif()

include(cmake/dev-mode.cmake)
