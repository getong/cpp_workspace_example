cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    cpp-20-exmaple
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    cpp-20-exmaple_lib OBJECT
    source/lib.cpp
    source/modules/registry.cpp
    source/modules/concepts/concepts.cpp
    source/modules/coroutine/coroutine.cpp
    source/modules/designated_init/designated_init.cpp
    source/modules/format/format.cpp
    source/modules/hello/hello.cpp
    source/modules/ranges/ranges.cpp
    source/modules/span_usage/span_usage.cpp
    source/modules/three_way_compare/three_way_compare.cpp
)

target_include_directories(
    cpp-20-exmaple_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(cpp-20-exmaple_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(cpp-20-exmaple_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(cpp-20-exmaple_exe source/main.cpp)
add_executable(cpp-20-exmaple::exe ALIAS cpp-20-exmaple_exe)

set_property(TARGET cpp-20-exmaple_exe PROPERTY OUTPUT_NAME cpp-20-exmaple)

target_compile_features(cpp-20-exmaple_exe PRIVATE cxx_std_20)

target_link_libraries(cpp-20-exmaple_exe PRIVATE cpp-20-exmaple_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT cpp-20-exmaple_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of cpp-20-exmaple"
  )
endif()

include(cmake/dev-mode.cmake)
