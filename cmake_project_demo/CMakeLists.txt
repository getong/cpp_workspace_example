cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    cmake_project_demo
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    cmake_project_demo_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    cmake_project_demo_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(cmake_project_demo_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(cmake_project_demo_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(cmake_project_demo_exe source/main.cpp)
add_executable(cmake_project_demo::exe ALIAS cmake_project_demo_exe)

set_property(TARGET cmake_project_demo_exe PROPERTY OUTPUT_NAME cmake_project_demo)

target_compile_features(cmake_project_demo_exe PRIVATE cxx_std_20)

target_link_libraries(cmake_project_demo_exe PRIVATE cmake_project_demo_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT cmake_project_demo_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of cmake_project_demo"
  )
endif()

include(cmake/dev-mode.cmake)
