#!/bin/sh

# Ensure the environment variable VCPKG_ROOT is set to your vcpkg installation directory
if [ -z "$VCPKG_ROOT" ]; then
  echo "VCPKG_ROOT is not set. Please set it to your vcpkg installation directory."
  exit 1
fi

mkdir -p build
cd build

# Run CMake with vcpkg toolchain
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B . -S .. "-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build .

# Move compile commands for code analysis tools
mv compile_commands.json ../..
