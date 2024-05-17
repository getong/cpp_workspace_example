# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(v8)

# Include directories for V8
include_directories(${CMAKE_SOURCE_DIR}/v8/include)

# Add the V8 library
add_library(v8 STATIC IMPORTED)

# Set the properties for the V8 library
set_target_properties(v8 PROPERTIES
  IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/v8/libv8_monolith.a
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/v8/include
)
