#!/bin/sh

mkdir -p build
cd build
#vcpkg install folly
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B . -S .. "-DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build .
mv compile_commands.json ../..