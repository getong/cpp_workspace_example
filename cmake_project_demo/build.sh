#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
VCPKG_ROOT="${VCPKG_ROOT:-${HOME}/vcpkg}"
VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-arm64-osx}"
VCPKG_PREFIX="$VCPKG_ROOT/installed/$VCPKG_TARGET_TRIPLET"
TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE:-$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake}"

if [ -n "${LDFLAGS:-}" ]; then
  sanitized_ldflags=""
  for flag in $LDFLAGS; do
    [ "$flag" = "-L/usr/local/opt/openssl@1.1/lib" ] && continue

    if [ -n "$sanitized_ldflags" ]; then
      sanitized_ldflags="$sanitized_ldflags $flag"
    else
      sanitized_ldflags=$flag
    fi
  done

  if [ -n "$sanitized_ldflags" ]; then
    LDFLAGS=$sanitized_ldflags
    export LDFLAGS
  else
    unset LDFLAGS
  fi
fi

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
  -Dcmake_project_demo_DEVELOPER_MODE=ON \
  -DVCPKG_MANIFEST_FEATURES=test \
  -DVCPKG_INSTALL_OPTIONS=--no-print-usage

cmake --build "$BUILD_DIR"

cmake -E copy_if_different \
  "$BUILD_DIR/compile_commands.json" \
  "$SCRIPT_DIR/compile_commands.json"
