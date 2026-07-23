cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    lambda_example
    VERSION 0.1.0
    DESCRIPTION "cpp lambda example"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    lambda_example_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    lambda_example_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(lambda_example_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(lambda_example_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(lambda_example_exe source/main.cpp source/lambda_demos.cpp)
add_executable(lambda_example::exe ALIAS lambda_example_exe)

set_property(TARGET lambda_example_exe PROPERTY OUTPUT_NAME lambda_example)

target_compile_features(lambda_example_exe PRIVATE cxx_std_20)

target_link_libraries(lambda_example_exe PRIVATE lambda_example_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT lambda_example_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of lambda_example"
  )
endif()

include(cmake/dev-mode.cmake)
