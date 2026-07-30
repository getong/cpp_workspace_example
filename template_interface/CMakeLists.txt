cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    template_interface
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    template_interface_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    template_interface_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(template_interface_lib PUBLIC cxx_std_23)

find_package(fmt REQUIRED)
target_link_libraries(template_interface_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(template_interface_exe source/main.cpp)
add_executable(template_interface::exe ALIAS template_interface_exe)

set_property(TARGET template_interface_exe PROPERTY OUTPUT_NAME template_interface)

target_compile_features(template_interface_exe PRIVATE cxx_std_23)

target_link_libraries(template_interface_exe PRIVATE template_interface_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT template_interface_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of template_interface"
  )
endif()

include(cmake/dev-mode.cmake)
