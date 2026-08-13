#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-"${repo_root}/build/dev"}"
build_type="${BUILD_TYPE:-Release}"

if command -v ninja >/dev/null 2>&1; then
  generator="${CMAKE_GENERATOR:-Ninja}"
else
  generator="${CMAKE_GENERATOR:-Unix Makefiles}"
fi

if [[ -z "${VCPKG_ROOT:-}" ]]; then
  if [[ -d "${HOME}/vcpkg" ]]; then
    VCPKG_ROOT="${HOME}/vcpkg"
  else
    echo "VCPKG_ROOT is not set and ${HOME}/vcpkg was not found." >&2
    exit 1
  fi
fi

toolchain_file="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
if [[ ! -f "${toolchain_file}" ]]; then
  echo "vcpkg toolchain file not found: ${toolchain_file}" >&2
  exit 1
fi

cmake_args=(
  -S "${repo_root}"
  -B "${build_dir}"
  -G "${generator}"
  -D "CMAKE_BUILD_TYPE=${build_type}"
  -D "CMAKE_TOOLCHAIN_FILE=${toolchain_file}"
)

if [[ "${generator}" == Ninja* ]] && command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-D "CMAKE_MAKE_PROGRAM=$(command -v ninja)")
fi

cmake "${cmake_args[@]}"
cmake --build "${build_dir}" --config "${build_type}"
