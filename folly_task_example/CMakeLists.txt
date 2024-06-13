cmake_minimum_required(VERSION 3.29)
project(folly_task_example VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Find dependencies
find_package(Boost REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(glog CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Link against Folly and its dependencies
# target_link_libraries(main PRIVATE Folly::folly Folly::folly_deps Folly::follybenchmark Folly::folly_test_util)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Boost::boost
  Folly::folly
  Folly::folly_deps  # Ensure all dependencies are linked
  Folly::follybenchmark
  Folly::folly_test_util
  gflags::gflags
  glog::glog
  Threads::Threads
)
