# Building the Unreal TArray demo

## Requirements

- Unreal Engine 5.8 with the Editor binaries installed;
- CMake 3.14 or newer;
- Ninja or another CMake generator supported by the host.

On macOS the scripts automatically discover `/Users/Shared/Epic Games/UE_*`.
For another installation, set `UNREAL_ENGINE_ROOT` to the directory that
contains `Engine`.

## Project layout

The repository root is the Unreal project root:

```text
UnrealCppDemo.uproject
Config/
Source/
  UnrealCppDemo.Target.cs
  UnrealCppDemoEditor.Target.cs
  UnrealCppDemo/
```

`Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, and `build` are
generated directories and are excluded from source control.

## Build

After cloning the repository or removing generated files with `git clean`,
initialize the complete local build and clangd environment:

```sh
./init.sh
```

This builds the Editor module first, then generates and validates the root
`compile_commands.json`. Successful UnrealBuildTool output is kept quiet;
diagnostics are printed when a step fails.

For another Unreal Engine installation, pass its root explicitly:

```sh
UNREAL_ENGINE_ROOT=/path/to/UE_5.8 ./init.sh
```

To build only the Unreal Editor module:

```sh
./build.sh
```

The script uses `build/unreal` only for the small CMake wrapper.  C++
compilation and linking are performed by UnrealBuildTool.

## Run the TArray example

```sh
./run.sh
```

This starts `UnrealEditor-Cmd` without rendering, runs
`UnrealCppDemo.Containers.TArray`, prints the `TArray` result, and exits with a
failure status if the Automation Test does not succeed.

## Generate compile commands

```sh
./compile_commands.sh
```

This invokes UnrealBuildTool's `GenerateClangDatabase` mode and writes an
Unreal-aware `compile_commands.json` at the repository root.
