cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    caf-example
    VERSION 0.1.0
    DESCRIPTION "c++ actor framework example"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    caf-example_lib OBJECT
    source/lib.cpp
    source/demos/demo_01_hello_world.cpp
    source/demos/demo_02_typed_calculator.cpp
    source/demos/demo_03_stateful_counter.cpp
    source/demos/demo_04_request_then.cpp
    source/demos/demo_05_behavior_switching.cpp
    source/demos/demo_06_delayed_messages.cpp
    source/demos/demo_07_error_handling.cpp
    source/demos/demo_08_monitoring.cpp
    source/demos/demo_09_custom_types.cpp
    source/demos/demo_10_fan_out.cpp
    source/demos/demo_11_data_flow.cpp
    source/demos/demo_12_ping_pong.cpp
    source/demos/demo_13_delegation.cpp
    source/demos/demo_14_game_room.cpp
)

target_include_directories(
    caf-example_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(caf-example_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(caf-example_lib PRIVATE fmt::fmt)

find_package(CAF CONFIG REQUIRED)
target_link_libraries(caf-example_lib PUBLIC CAF::core)

# ---- Declare executable ----

add_executable(caf-example_exe source/main.cpp)
add_executable(caf-example::exe ALIAS caf-example_exe)

set_property(TARGET caf-example_exe PROPERTY OUTPUT_NAME caf-example)

target_compile_features(caf-example_exe PRIVATE cxx_std_20)

target_link_libraries(caf-example_exe PRIVATE caf-example_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT caf-example_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of caf-example"
  )
endif()

include(cmake/dev-mode.cmake)
