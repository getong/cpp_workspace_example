cmake_minimum_required(VERSION 3.20)

project(
    incremental-demo
    VERSION 0.1.0
    DESCRIPTION "Demonstrates incremental compilation with CMake + Ninja"
    LANGUAGES CXX
)

# 每个 .cpp 都是一个独立的编译单元（translation unit）。
# Ninja 会为每个编译单元生成一条 build edge，并通过编译器输出的
# 依赖文件（.d depfile，-MD -MF）精确记录它包含了哪些头文件。
# 这就是增量编译的基础：只有输入（源文件或其包含的头文件）
# 变化了的编译单元才会被重新编译。
add_library(
    mathlib STATIC
    src/mathlib/add.cpp
    src/mathlib/mul.cpp
)
target_include_directories(mathlib PUBLIC src)
target_compile_features(mathlib PUBLIC cxx_std_20)

add_library(
    greeting STATIC
    src/greeting/greeting.cpp
)
target_include_directories(greeting PUBLIC src)
target_link_libraries(greeting PUBLIC mathlib)
target_compile_features(greeting PUBLIC cxx_std_20)

add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE greeting mathlib)
