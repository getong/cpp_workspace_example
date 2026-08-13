cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    boost-asio-example
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    boost-asio-example_lib OBJECT
    source/lib.cpp
    source/modules/registry.cpp
    source/modules/coroutine/coroutine.cpp
    source/modules/echo_tcp/echo_tcp.cpp
    source/modules/hello/hello.cpp
    source/modules/post_dispatch/post_dispatch.cpp
    source/modules/resolver/resolver.cpp
    source/modules/strand/strand.cpp
    source/modules/timer/timer.cpp
    source/modules/udp_echo/udp_echo.cpp
)

target_include_directories(
    boost-asio-example_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(boost-asio-example_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(boost-asio-example_lib PRIVATE fmt::fmt)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(boost-asio-example_lib PUBLIC Boost::headers Threads::Threads)

# ---- Declare executable ----

add_executable(boost-asio-example_exe source/main.cpp)
add_executable(boost-asio-example::exe ALIAS boost-asio-example_exe)

set_property(TARGET boost-asio-example_exe PROPERTY OUTPUT_NAME boost-asio-example)

target_compile_features(boost-asio-example_exe PRIVATE cxx_std_20)

target_link_libraries(boost-asio-example_exe PRIVATE boost-asio-example_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT boost-asio-example_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of boost-asio-example"
  )
endif()

include(cmake/dev-mode.cmake)
