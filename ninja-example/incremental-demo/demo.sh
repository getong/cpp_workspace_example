#!/bin/sh
# 一键演示 CMake + Ninja 的增量编译。在 incremental-demo 目录下运行。
set -eu
cd "$(dirname "$0")"

step() {
  printf '\n\033[1;34m=== %s ===\033[0m\n' "$1"
}

step "配置（生成 build.ninja）"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

step "场景 0：全量构建（4 个编译单元 + 2 个静态库 + 1 个链接 = 7 步）"
ninja -C build
./build/demo

step "场景 1：什么都不改，再次构建 —— 期望 'no work to do'"
ninja -C build

step "场景 2：touch src/mathlib/add.cpp —— 期望只重编 add.cpp.o，共 3 步"
touch src/mathlib/add.cpp
ninja -C build

step "场景 3：touch src/version.hpp —— 期望重编 greeting.cpp.o 和 main.cpp.o，共 4 步"
touch src/version.hpp
ninja -C build -d explain 2>&1 | grep -E 'explain|Building|Linking' || true

step "ninja -t deps：greeting.cpp.o 的头文件依赖（前 5 行）"
ninja -C build -t deps CMakeFiles/greeting.dir/src/greeting/greeting.cpp.o | head -5
