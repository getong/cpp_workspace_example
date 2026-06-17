#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
VCPKG_ROOT="${VCPKG_ROOT:-${HOME}/vcpkg}"
VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-arm64-osx}"
VCPKG_PREFIX="$VCPKG_ROOT/installed/$VCPKG_TARGET_TRIPLET"
TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE:-$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake}"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
  echo "vcpkg toolchain file not found: $TOOLCHAIN_FILE" >&2
  echo "Expected vcpkg under: $VCPKG_ROOT" >&2
  exit 1
fi

if [ -n "${CMAKE_PREFIX_PATH:-}" ]; then
  CMAKE_PREFIX_PATH_VALUE="$VCPKG_PREFIX;$CMAKE_PREFIX_PATH"
else
  CMAKE_PREFIX_PATH_VALUE="$VCPKG_PREFIX"
fi

cmake --fresh -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
  -DVCPKG_TARGET_TRIPLET="$VCPKG_TARGET_TRIPLET" \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH_VALUE" \
  -Dmy_project_DEVELOPER_MODE=ON \
  -DVCPKG_MANIFEST_FEATURES=test

cmake --build "$BUILD_DIR"

cmake -E copy_if_different \
  "$BUILD_DIR/compile_commands.json" \
  "$SCRIPT_DIR/compile_commands.json"
