cmake_minimum_required(VERSION 3.10)

project(v8_project)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add subdirectory for V8
add_subdirectory(v8)

# Add the executable
add_executable(v8_project main.cpp)

# Link the V8 library
target_link_libraries(v8_project PRIVATE v8)
