# Unreal Engine and TObjectPtr

## What TObjectPtr is for

`TObjectPtr<T>` is UE5's recommended type for `UObject*` members.  Epic's
guideline is: inside `UPROPERTY()` members of UCLASS/USTRUCT types, write

```cpp
UPROPERTY()
TObjectPtr<UWeapon> Weapon;
```

instead of `UPROPERTY() UWeapon* Weapon;`.  Function parameters, locals, and
return values should stay raw `T*`.

## Advantages demonstrated by the example

`RunTObjectPtrDemo()` demonstrates each of these:

1. **Safe default.**  A `TObjectPtr` member is always zero-initialized to
   null, while an uninitialized raw pointer member is garbage.
2. **Drop-in replacement.**  It converts implicitly to and from `T*`, so
   assignments from `NewObject`, calls into raw-pointer APIs, comparisons,
   `Cast<>`, and `TArray` lookups all keep working without call-site changes.
3. **Zero overhead.**  `sizeof(TObjectPtr<T>) == sizeof(T*)`, and in Shipping
   builds every access compiles down to a plain pointer read.
4. **Editor-build tracking.**  In editor builds each access resolves through
   an object handle (`FObjectHandle`).  That central access point is what
   lets the engine track object accesses for incremental garbage collection
   and lazily load assets on first access.
5. **GC integration.**  Combined with `UPROPERTY()`, the reflection system
   sees the reference (shown in the demo with `FReferenceFinder`), keeps the
   target alive, and resets the pointer to null when the target is destroyed
   instead of leaving it dangling.  The demo also shows `IsValid()` reporting
   `false` as soon as the target is marked as garbage.

## Build and run the example

```sh
./build.sh   # build the Editor module
./run.sh     # run both Automation Tests headless and print the demo steps
```

When the `UnrealCppDemo` module starts, each demonstrated operation is logged
with the `TOBJECTPTR_DEMO|` prefix, and the
`UnrealCppDemo.UObject.TObjectPtr` Automation Test asserts the recorded
results.

With the local UE 5.8 installation, the actual template is in:

```text
/Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h
```

`TObjectPtr` wraps an `FObjectPtr`, which stores an `FObjectHandle`.  With
lazy loading disabled (the default for runtime builds) the handle is just the
`UObject*` itself, which is why the type is pointer-sized and free at
runtime.
