#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"

#include "TObjectPtrDemo.generated.h"

// TObjectPtr<T> is UE5's replacement for raw UObject* members.  Its purpose:
//
// 1. Safe default: a TObjectPtr member is always zero-initialized, while a
//    forgotten raw pointer member starts as garbage.
// 2. Drop-in replacement: it converts implicitly to and from T*, so existing
//    call sites, comparisons, and container lookups keep working unchanged.
// 3. Zero overhead where it matters: it has the same size as a raw pointer,
//    and in Shipping builds every access compiles down to a plain T* read.
// 4. Editor-only superpowers: in editor builds each access resolves through
//    an object handle, which lets the engine track accesses for incremental
//    garbage collection and load assets lazily on first use.
// 5. GC integration: combined with UPROPERTY() the reflection system sees the
//    reference, keeps the target alive, and nulls the pointer when the target
//    is destroyed instead of leaving it dangling.
UCLASS()
class UNREALCPPDEMO_API UTObjectPtrDemoObject : public UObject
{
    GENERATED_BODY()

public:
    // The recommended UE5 form of a UObject member: UPROPERTY() + TObjectPtr
    // instead of UPROPERTY() + UTObjectPtrDemoObject*.
    UPROPERTY()
    TObjectPtr<UTObjectPtrDemoObject> Next;

    int32 Id = 0;
};

struct FTObjectPtrDemoStep
{
    FString Operation;
    FString Result;
};

struct FTObjectPtrDemoResult
{
    TArray<FTObjectPtrDemoStep> Steps;
    SIZE_T ObjectPtrSize = 0;
    SIZE_T RawPtrSize = 0;
    int32 IdThroughArrow = 0;
    int32 IdThroughRawCall = 0;
    int32 IndexInArray = INDEX_NONE;
    bool bDefaultIsNull = false;
    bool bEqualsRawPointer = false;
    bool bCastSucceeded = false;
    bool bGCSeesReference = false;
    bool bValidBeforeMark = false;
    bool bValidAfterMark = false;
};

UNREALCPPDEMO_API FTObjectPtrDemoResult RunTObjectPtrDemo();
