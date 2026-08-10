cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    asio-call-tokio
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    asio-call-tokio_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    asio-call-tokio_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(asio-call-tokio_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(asio-call-tokio_lib PRIVATE fmt::fmt)

# Boost.Asio (same codebase as standalone asio, released with each Boost);
# the vcpkg boost-asio port tracks Boost releases much more closely than the
# stale standalone asio port (1.91.0 embeds asio 1.38.1 vs standalone 1.32.0).
find_package(boost_asio CONFIG REQUIRED)
target_link_libraries(asio-call-tokio_lib PUBLIC Boost::asio)

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
    CRATE asio_tokio_core
    FILES lib.rs
)

# The bridge module declares extern "C++" completion callbacks via
# include!("bridge_callbacks.hpp"), so the generated code (and everything
# that includes rust_bridge/lib.h) needs source/ on its include path.
target_include_directories(
    rust_bridge
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

# The generated bridge code calls back into tokio_ffi::complete_* which are
# implemented in lib.cpp, and lib.cpp's initiation code needs asio; keep the
# bridge target itself self-contained header-wise.
target_link_libraries(rust_bridge PUBLIC Boost::asio)

# PRIVATE on an object library still propagates the archives to final link
# lines of consumers (asio-call-tokio_exe, tests) via $<LINK_ONLY:...>.
target_link_libraries(asio-call-tokio_lib PRIVATE rust_bridge)

# ---- Declare executable ----

add_executable(asio-call-tokio_exe source/main.cpp)
add_executable(asio-call-tokio::exe ALIAS asio-call-tokio_exe)

set_property(TARGET asio-call-tokio_exe PROPERTY OUTPUT_NAME asio-call-tokio)

target_compile_features(asio-call-tokio_exe PRIVATE cxx_std_20)

target_link_libraries(asio-call-tokio_exe PRIVATE asio-call-tokio_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT asio-call-tokio_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of asio-call-tokio"
  )
endif()

include(cmake/dev-mode.cmake)
