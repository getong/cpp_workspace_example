#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-"${repo_root}/build/unreal"}"
build_log="$(mktemp "${TMPDIR:-/tmp}/unreal-cpp-demo-build.XXXXXX")"

cleanup()
{
  rm -f "${build_log}"
}
trap cleanup EXIT

if ! BUILD_DIR="${build_dir}" BUILD_TARGET=unreal-tarray-test \
  "${repo_root}/build.sh" >"${build_log}" 2>&1
then
  echo "Unreal build or Automation Test failed:" >&2
  cat "${build_log}" >&2
  exit 1
fi

case "$(uname -s)" in
  Darwin)
    log_file="${UNREAL_LOG_FILE:-${HOME}/Library/Logs/Unreal Engine/UnrealCppDemoEditor/UnrealCppDemo.log}"
    ;;
  *)
    log_file="${UNREAL_LOG_FILE:-${repo_root}/Saved/Logs/UnrealCppDemo.log}"
    ;;
esac

if [[ ! -f "${log_file}" ]]; then
  echo "Unreal log was not found: ${log_file}" >&2
  exit 1
fi

tarray_output="$(awk '
  /LogTArrayDemo: Display: TARRAY_DEMO\|/ {
    line=$0
    sub(/^.*LogTArrayDemo: Display: TARRAY_DEMO\|/, "", line)
    print line
  }
' "${log_file}")"
test_log_line="$(awk '/Test Completed.*Path=\{UnrealCppDemo\.Containers\.TArray\}/{line=$0} END{print line}' "${log_file}")"

if [[ -z "${tarray_output}" ]]; then
  echo "The Unreal process did not emit a TArray demo result." >&2
  exit 1
fi

if [[ "${test_log_line}" != *"Result={Success}"* ]]; then
  echo "The Unreal TArray Automation Test did not succeed." >&2
  [[ -n "${test_log_line}" ]] && echo "${test_log_line}" >&2
  exit 1
fi

printf '%s\n' "${tarray_output}"
echo "Automation test: Success (UnrealCppDemo.Containers.TArray)"
