#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_type="${BUILD_TYPE:-Release}"
build_type_lower="$(printf '%s' "${build_type}" | tr '[:upper:]' '[:lower:]')"
build_dir="${BUILD_DIR:-"${repo_root}/build/${build_type_lower}"}"

# "${repo_root}/build.sh"

# Single-config generators (Ninja, Makefiles) put the executable at the build
# dir root; multi-config ones (Xcode, VS) add a per-config subdirectory.
if [[ -x "${build_dir}/rust-ffi" ]]; then
  exe="${build_dir}/rust-ffi"
elif [[ -x "${build_dir}/${build_type}/rust-ffi" ]]; then
  exe="${build_dir}/${build_type}/rust-ffi"
else
  echo "rust-ffi executable not found under ${build_dir}" >&2
  exit 1
fi

exec "${exe}" "$@"
