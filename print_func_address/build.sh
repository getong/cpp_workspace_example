#!/bin/sh
set -eu

strip_stale_openssl_flags() {
  var_name=$1
  eval "value=\${$var_name-}"

  value=$(printf '%s' "$value" \
    | sed \
      -e 's#-L/usr/local/opt/openssl@1\.1/lib##g' \
      -e 's#-I/usr/local/opt/openssl@1\.1/include##g' \
      -e 's#[[:space:]][[:space:]]*# #g' \
      -e 's#^ ##' \
      -e 's# $##')

  export "$var_name=$value"
}

strip_stale_openssl_flags LDFLAGS
strip_stale_openssl_flags CPPFLAGS

cmake -S . -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS:-}" \
  -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS:-}" \
  -DCMAKE_MODULE_LINKER_FLAGS="${LDFLAGS:-}"

cmake --build build

if [ ! -f build/compile_commands.json ]; then
  echo "error: build/compile_commands.json was not generated" >&2
  exit 1
fi

cp build/compile_commands.json .
