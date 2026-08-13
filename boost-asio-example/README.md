# boost-asio-example

This is the boost-asio-example project, a collection of Boost.Asio feature
modules: timers, executors, strands, C++20 coroutines and loopback
networking, each in a small self-contained example.

A Chinese walkthrough of every module and the core Asio concepts lives in
[docs/asio-guide.md](docs/asio-guide.md).

# Modules

Each feature lives in its own directory under `source/modules/<name>/` and is
registered in `source/modules/registry.cpp`. The executable dispatches by
module name:

```sh
boost-asio-example            # list available modules
boost-asio-example all        # run every module
boost-asio-example echo_tcp   # run a single module
```

Current modules:

- `hello` — prints the project greeting from the core library
- `timer` — `steady_timer`: sync wait, `async_wait`, periodic re-arm and
  cancellation
- `post_dispatch` — submitting work to an executor: `post` vs `dispatch` vs
  `defer`
- `strand` — serializing handlers on a multi-threaded `thread_pool` with a
  strand (no locks needed)
- `coroutine` — C++20 coroutines: `co_spawn`, `awaitable`, and the `||` / `&&`
  awaitable operators
- `echo_tcp` — coroutine-based TCP echo server and client over loopback
- `udp_echo` — callback-style UDP echo over loopback datagrams
- `resolver` — asynchronous name resolution with `ip::tcp::resolver`

All networking examples talk to themselves over the loopback interface on
system-assigned ports, so the whole suite runs offline and never conflicts
with other programs.

To add a module, create `source/modules/<name>/<name>.{hpp,cpp}` exposing
`modules::<name>::run()`, add the `.cpp` to `CMakeLists.txt`, and append one
entry to the list in `registry.cpp`.

# Scripts

- `./build.sh` — configure and build (Release by default) into `build/dev`
  using the vcpkg toolchain. Override with `BUILD_DIR` / `BUILD_TYPE` /
  `CMAKE_GENERATOR` environment variables.
- `./compile_commands.sh` — developer-mode build (tests enabled) into
  `build/compile_commands`, then copies `compile_commands.json` to the repo
  root for clangd (`.clangd` points at the repo root).

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

<!--
Please go to https://choosealicense.com/licenses/ and choose a license that
fits your needs. The recommended license for a project of this type is the
GNU AGPLv3.
-->
