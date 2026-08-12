# cpp-20-exmaple

This is the cpp-20-exmaple project, a collection of C++20 feature modules.

# Modules

Each feature lives in its own directory under `source/modules/<name>/` and is
registered in `source/modules/registry.cpp`. The executable dispatches by
module name:

```sh
cpp-20-exmaple              # list available modules
cpp-20-exmaple all          # run every module
cpp-20-exmaple span_usage   # run a single module
```

Current modules:

- `hello` — prints the project greeting from the core library
- `span_usage` — `std::span` as a unified view over arrays and vectors
- `concepts` — constrain templates with concepts and `requires`
- `ranges` — lazy view pipelines and range-based algorithms
- `coroutine` — a `co_yield` based lazy generator
- `three_way_compare` — defaulted `operator<=>` gives all six comparisons
- `designated_init` — designated initializers for aggregate types
- `format` — type-safe text formatting with `std::format` (falls back to fmt)
- `lambda` — lambda expressions: captures, `mutable`, generic/template lambdas, `constexpr`, IIFE, recursion, algorithms

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
