cmake_minimum_required(VERSION 3.10)  # Set the minimum version of CMake

# Define the project name and the language used
project(print_func_address VERSION 1.0.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)
