# Hacking

This repository is an Unreal Engine project.  Do not add standalone C++
targets that include Engine headers outside UnrealBuildTool.

## Source layout

- `Source/UnrealCppDemo/Public` contains the module's public API;
- `Source/UnrealCppDemo/Private` contains its implementation;
- `Source/UnrealCppDemo/Private/Tests` contains Unreal Automation Tests;
- `cmake/unreal-engine.cmake` only exposes convenient UnrealBuildTool targets.

Do not edit or commit `Binaries`, `Intermediate`, `Saved`,
`DerivedDataCache`, or `build`.  They are generated from the `.uproject` and
`Source` files.

## Workflow

```sh
./build.sh
./run.sh
./compile_commands.sh
```

`run.sh` is the test entry point.  It must finish with both the `TArray`
result and `Automation test: Success`.

When adding another Unreal module dependency, declare it in
`Source/UnrealCppDemo/UnrealCppDemo.Build.cs`.  Do not add Engine include
directories manually to CMake.
