cmake_minimum_required(VERSION 3.29)
project(folly_example VERSION 1.0.0 LANGUAGES CXX)

# Find Folly package
find_package(Boost REQUIRED)
find_package(folly REQUIRED)
find_package(Threads REQUIRED)
find_package(gflags REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

#include_directories(${Boost_INCLUDE_DIRS})
#include_directories(${folly_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD_REQUIRED ON)


if (NOT TARGET Folly::folly)
  include("${FOLLY_CMAKE_DIR}/folly-targets.cmake")
endif()


set(FOLLY_LIBRARIES Folly::folly)

if (NOT folly_FIND_QUIETLY)
  message(STATUS "Found folly: ${PACKAGE_PREFIX_DIR}")
endif()

# Add executable
add_executable(folly_example src/main.cpp)

# Link Folly library
target_link_libraries(folly_example PRIVATE Folly::folly)
