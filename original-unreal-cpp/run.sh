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

if ! BUILD_DIR="${build_dir}" BUILD_TARGET=unreal-automation-test \
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

demo_output()
{
  local marker="$1"
  awk -v marker="${marker}" '
    {
      pos = index($0, marker)
      if (pos > 0) {
        print substr($0, pos + length(marker))
      }
    }
  ' "${log_file}"
}

check_test()
{
  local test_path="$1"
  local test_log_line
  test_log_line="$(awk -v needle="Path={${test_path}}" '
    index($0, "Test Completed") > 0 && index($0, needle) > 0 {line=$0}
    END{print line}
  ' "${log_file}")"

  if [[ "${test_log_line}" != *"Result={Success}"* ]]; then
    echo "The Unreal Automation Test ${test_path} did not succeed." >&2
    [[ -n "${test_log_line}" ]] && echo "${test_log_line}" >&2
    exit 1
  fi
}

tarray_output="$(demo_output "LogTArrayDemo: Display: TARRAY_DEMO|")"
tobjectptr_output="$(demo_output "LogTObjectPtrDemo: Display: TOBJECTPTR_DEMO|")"

if [[ -z "${tarray_output}" ]]; then
  echo "The Unreal process did not emit a TArray demo result." >&2
  exit 1
fi

if [[ -z "${tobjectptr_output}" ]]; then
  echo "The Unreal process did not emit a TObjectPtr demo result." >&2
  exit 1
fi

check_test "UnrealCppDemo.Containers.TArray"
check_test "UnrealCppDemo.UObject.TObjectPtr"

printf '%s\n' "${tarray_output}"
echo
printf '%s\n' "${tobjectptr_output}"
echo "Automation test: Success (UnrealCppDemo.Containers.TArray)"
echo "Automation test: Success (UnrealCppDemo.UObject.TObjectPtr)"
