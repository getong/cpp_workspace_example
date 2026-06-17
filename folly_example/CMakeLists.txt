cmake_minimum_required(VERSION 3.29)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(folly_example VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(folly CONFIG REQUIRED)

add_executable(folly_example src/main.cpp)
target_link_libraries(folly_example PRIVATE Folly::folly)
