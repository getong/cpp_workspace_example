#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-"${repo_root}/build/unreal"}"
build_target="${BUILD_TARGET:-unreal-editor-build}"

if command -v ninja >/dev/null 2>&1; then
  generator="${CMAKE_GENERATOR:-Ninja}"
else
  generator="${CMAKE_GENERATOR:-Unix Makefiles}"
fi

cmake_args=(
  -S "${repo_root}"
  -B "${build_dir}"
  -G "${generator}"
)

if [[ -n "${UNREAL_ENGINE_ROOT:-}" ]]; then
  cmake_args+=(-D "UNREAL_ENGINE_ROOT=${UNREAL_ENGINE_ROOT}")
fi

if [[ "${generator}" == Ninja* ]] && command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-D "CMAKE_MAKE_PROGRAM=$(command -v ninja)")
fi

cmake "${cmake_args[@]}"
cmake --build "${build_dir}" --target "${build_target}"

echo "Completed Unreal target: ${build_target}"
