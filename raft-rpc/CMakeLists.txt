cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    raft-rpc
    VERSION 0.1.0
    DESCRIPTION "Raft + brpc example based on braft"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    raft-rpc_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    raft-rpc_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(raft-rpc_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(raft-rpc_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(raft-rpc_exe source/main.cpp)
add_executable(raft-rpc::exe ALIAS raft-rpc_exe)

set_property(TARGET raft-rpc_exe PROPERTY OUTPUT_NAME raft-rpc)

target_compile_features(raft-rpc_exe PRIVATE cxx_std_20)

target_link_libraries(raft-rpc_exe PRIVATE raft-rpc_lib)

# ---- Raft example (eBay NuRaft) ----

find_package(NuRaft CONFIG REQUIRED)

add_executable(counter_server source/counter_server.cpp)
target_compile_features(counter_server PRIVATE cxx_std_20)
target_link_libraries(counter_server PRIVATE NuRaft::static_lib)

add_executable(counter_client source/counter_client.cpp)
target_compile_features(counter_client PRIVATE cxx_std_20)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT raft-rpc_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of raft-rpc"
  )
endif()

include(cmake/dev-mode.cmake)
