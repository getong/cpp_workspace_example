cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    onetbb_demo
    VERSION 0.1.0
    DESCRIPTION "onetbb demo"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    onetbb_demo_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    onetbb_demo_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(onetbb_demo_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
find_package(TBB CONFIG REQUIRED)
target_link_libraries(
    onetbb_demo_lib
    PUBLIC
    TBB::tbb
    PRIVATE
    fmt::fmt
)

# ---- Declare executable ----

add_executable(onetbb_demo_exe source/main.cpp)
add_executable(onetbb_demo::exe ALIAS onetbb_demo_exe)

set_property(TARGET onetbb_demo_exe PROPERTY OUTPUT_NAME onetbb_demo)

target_compile_features(onetbb_demo_exe PRIVATE cxx_std_20)

target_link_libraries(onetbb_demo_exe PRIVATE onetbb_demo_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT onetbb_demo_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of onetbb_demo"
  )
endif()

include(cmake/dev-mode.cmake)
