cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    cobalt-example
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    cobalt-example_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    cobalt-example_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(cobalt-example_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
find_package(boost_cobalt CONFIG REQUIRED)
target_link_libraries(cobalt-example_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(cobalt-example_exe source/main.cpp)
add_executable(cobalt-example::exe ALIAS cobalt-example_exe)

set_property(TARGET cobalt-example_exe PROPERTY OUTPUT_NAME cobalt-example)

target_compile_features(cobalt-example_exe PRIVATE cxx_std_20)

target_link_libraries(cobalt-example_exe PRIVATE cobalt-example_lib Boost::cobalt)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT cobalt-example_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of cobalt-example"
  )
endif()

include(cmake/dev-mode.cmake)
