#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-"${repo_root}/build/unreal"}"
build_log="$(mktemp "${TMPDIR:-/tmp}/unreal-cpp-demo-init-build.XXXXXX")"
commands_log="$(mktemp "${TMPDIR:-/tmp}/unreal-cpp-demo-init-commands.XXXXXX")"

cleanup()
{
  rm -f "${build_log}" "${commands_log}"
}
trap cleanup EXIT

fail_with_log()
{
  local message="$1"
  local log_file="$2"

  echo "${message}" >&2
  cat "${log_file}" >&2
  exit 1
}

missing_commands=()
for required_command in cmake jq clangd; do
  if ! command -v "${required_command}" >/dev/null 2>&1; then
    missing_commands+=("${required_command}")
  fi
done

if (( ${#missing_commands[@]} > 0 )); then
  printf 'Missing required command(s): %s\n' "${missing_commands[*]}" >&2
  exit 1
fi

if [[ ! -f "${repo_root}/UnrealCppDemo.uproject" ]]; then
  echo "Unreal project file is missing: ${repo_root}/UnrealCppDemo.uproject" >&2
  exit 1
fi

if [[ -n "${UNREAL_ENGINE_ROOT:-}" && ! -d "${UNREAL_ENGINE_ROOT}/Engine" ]]; then
  echo "UNREAL_ENGINE_ROOT does not contain an Engine directory: ${UNREAL_ENGINE_ROOT}" >&2
  exit 1
fi

echo "[1/2] Building UnrealCppDemoEditor..."
if ! BUILD_DIR="${build_dir}" "${repo_root}/build.sh" >"${build_log}" 2>&1; then
  fail_with_log "Unreal Editor module initialization failed:" "${build_log}"
fi
echo "[1/2] UnrealCppDemoEditor is ready."

echo "[2/2] Generating and validating compile_commands.json..."
if ! BUILD_DIR="${build_dir}" "${repo_root}/compile_commands.sh" >"${commands_log}" 2>&1; then
  fail_with_log "Compile database initialization failed:" "${commands_log}"
fi
echo "[2/2] compile_commands.json is ready."

echo "Project initialized successfully: ${repo_root}"
