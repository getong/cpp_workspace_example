# unreal-cpp-demo

This is a standard Unreal Engine C++ project for exploring `TArray`.  The
`.uproject`, `Config`, and `Source` directories live at the repository root,
and all C++ code is compiled by UnrealBuildTool against the real Engine Core
module.

The top-level CMake project is only a convenient wrapper around
UnrealBuildTool.  Run `./build.sh` to build the Editor module, `./run.sh` to
execute the `TArray` Automation Test in `UnrealEditor-Cmd`, and
`./compile_commands.sh` to generate an Unreal-aware clang database.

See [the Unreal and TArray notes](docs/TARRAY.md) for the relevant Engine
source locations and example operations.

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
