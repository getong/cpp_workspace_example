# Unreal Engine and TArray

## Why Unreal Engine is not in vcpkg

Unreal Engine is not a normal header/library package.  Its modules depend on
UnrealBuildTool, platform definitions, generated headers, Unreal's allocator,
and a matching set of Engine binaries.  For that reason, adding the Engine to
a vcpkg manifest or including `Containers/Array.h` in a standalone CMake
executable would not create a valid Unreal program.

This repository places all executable C++ code in a standard Unreal module.
The top-level CMake targets are only convenient wrappers around
UnrealBuildTool.

## Build and run the example

On this machine CMake automatically detects installations named
`/Users/Shared/Epic Games/UE_*`.  On another machine, configure the exact SDK
path explicitly:

```sh
UNREAL_ENGINE_ROOT="/path/to/UE_5.8" ./build.sh
```

Build the Editor module:

```sh
./build.sh
```

Run the headless Automation Test:

```sh
./run.sh
```

The project can also be opened directly with `UnrealCppDemo.uproject`.
When the `UnrealCppDemo` module starts, it logs the resulting array, element
count, capacity, allocated bytes, and whether the elements are contiguous.

## What the demo exercises

`RunTArrayDemo()` uses these operations:

- `Reserve`, `Add`, `Emplace`, and `Append` for allocation and construction;
- `RemoveSingle` and `AddUnique` for mutation;
- `Sort`, indexed access, range iteration, and `GetData` for access;
- `Num`, `Max`, and `GetAllocatedSize` for size and capacity inspection.

The result is `[10, 20, 20, 30, 50]` with a sum of `130`.

With the local UE 5.8 installation, the actual template is in:

```text
/Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Public/Containers/Array.h
```

The important implementation members are the allocator instance, `ArrayNum`
(constructed element count), and `ArrayMax` (capacity).  `TArray` stores
elements contiguously, but adding or removing elements can reallocate or move
them, so pointers and references into the array may be invalidated.
