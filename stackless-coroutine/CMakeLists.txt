cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    stackless-coroutine
    VERSION 0.1.0
    DESCRIPTION "A tutorial project for C++20 stackless coroutines"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    stackless-coroutine_lib OBJECT
    source/examples.cpp
)

target_include_directories(
    stackless-coroutine_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(stackless-coroutine_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(stackless-coroutine_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(stackless-coroutine_exe source/main.cpp)
add_executable(stackless-coroutine::exe ALIAS stackless-coroutine_exe)

set_property(TARGET stackless-coroutine_exe PROPERTY OUTPUT_NAME stackless-coroutine)

target_compile_features(stackless-coroutine_exe PRIVATE cxx_std_20)

target_link_libraries(stackless-coroutine_exe PRIVATE stackless-coroutine_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT stackless-coroutine_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of stackless-coroutine"
  )
endif()

include(cmake/dev-mode.cmake)
