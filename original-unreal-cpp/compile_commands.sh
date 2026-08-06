#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-"${repo_root}/build/unreal"}"

BUILD_DIR="${build_dir}" BUILD_TARGET=unreal-compile-commands "${repo_root}/build.sh"

if [[ ! -s "${repo_root}/compile_commands.json" ]]; then
  echo "UnrealBuildTool did not generate ${repo_root}/compile_commands.json" >&2
  exit 1
fi

demo_source="${repo_root}/Source/UnrealCppDemo/Private/TArrayDemo.cpp"
if ! jq -e --arg source "${demo_source}" 'any(.[]; .file == $source)' \
  "${repo_root}/compile_commands.json" >/dev/null; then
  echo "Unreal compile database does not contain ${demo_source}" >&2
  exit 1
fi

if command -v clangd >/dev/null 2>&1; then
  clangd_log="$(mktemp "${TMPDIR:-/tmp}/unreal-cpp-demo-clangd.XXXXXX")"
  if ! clangd \
    --check="${demo_source}" \
    --compile-commands-dir="${repo_root}" \
    --log=error >"${clangd_log}" 2>&1; then
    cat "${clangd_log}" >&2
    rm -f "${clangd_log}"
    echo "clangd could not parse ${demo_source}" >&2
    exit 1
  fi
  rm -f "${clangd_log}"
fi

echo "Generated and validated Unreal compile database: ${repo_root}/compile_commands.json"
