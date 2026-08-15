#include "TObjectPtrDemo.h"

#include "Templates/Casts.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/UObjectGlobals.h"

namespace
{
void AddStep(FTObjectPtrDemoResult& Demo, const TCHAR* Operation, FString Result)
{
    Demo.Steps.Add({Operation, MoveTemp(Result)});
}

const TCHAR* BoolText(const bool bValue)
{
    return bValue ? TEXT("true") : TEXT("false");
}

// A legacy-style API that only accepts a raw pointer.  TObjectPtr converts
// implicitly, so callers migrating a member to TObjectPtr do not change.
int32 ReadIdThroughRawPointer(const UTObjectPtrDemoObject* Object)
{
    return Object != nullptr ? Object->Id : INDEX_NONE;
}
} // namespace

FTObjectPtrDemoResult RunTObjectPtrDemo()
{
    FTObjectPtrDemoResult Result;

    // A raw pointer member left uninitialized is garbage; TObjectPtr is
    // always zero-initialized to null.
    TObjectPtr<UTObjectPtrDemoObject> Pointer;
    Result.bDefaultIsNull = (Pointer == nullptr);
    AddStep(
        Result,
        TEXT("TObjectPtr<UTObjectPtrDemoObject> Pointer"),
        FString::Printf(TEXT("default-initialized to null=%s"), BoolText(Result.bDefaultIsNull))
    );

    // NewObject returns a raw T*; assigning it works because TObjectPtr is a
    // drop-in replacement with implicit conversions in both directions.
    UTObjectPtrDemoObject* Raw = NewObject<UTObjectPtrDemoObject>();
    Pointer = Raw;
    AddStep(
        Result,
        TEXT("Pointer = NewObject<...>()"),
        FString::Printf(TEXT("assigned raw pointer, Pointer=%s"), *Pointer->GetName())
    );

    // operator-> and operator* behave exactly like a raw pointer.  In editor
    // builds each access resolves through an object handle, which is what
    // enables access tracking, incremental GC, and lazy loading.
    Pointer->Id = 42;
    Result.IdThroughArrow = Pointer->Id;
    AddStep(
        Result,
        TEXT("Pointer->Id = 42"),
        FString::Printf(TEXT("read back through operator->: %d"), Result.IdThroughArrow)
    );

    // Same size as a raw pointer; in Shipping builds it compiles down to a
    // plain T*, so adopting it costs nothing at runtime.
    Result.ObjectPtrSize = sizeof(TObjectPtr<UTObjectPtrDemoObject>);
    Result.RawPtrSize = sizeof(UTObjectPtrDemoObject*);
    AddStep(
        Result,
        TEXT("sizeof(TObjectPtr) vs sizeof(T*)"),
        FString::Printf(
            TEXT("%llu vs %llu bytes"),
            static_cast<uint64>(Result.ObjectPtrSize),
            static_cast<uint64>(Result.RawPtrSize)
        )
    );

    // Implicit conversion to T* feeds APIs that still take raw pointers.
    Result.IdThroughRawCall = ReadIdThroughRawPointer(Pointer);
    AddStep(
        Result,
        TEXT("ReadIdThroughRawPointer(Pointer)"),
        FString::Printf(TEXT("implicit conversion to T*, returned %d"), Result.IdThroughRawCall)
    );

    // Comparisons against raw pointers keep working unchanged.
    Result.bEqualsRawPointer = (Pointer == Raw);
    AddStep(
        Result,
        TEXT("Pointer == Raw"),
        FString::Printf(TEXT("returned %s"), BoolText(Result.bEqualsRawPointer))
    );

    // Containers of TObjectPtr interoperate with raw pointers too.
    TArray<TObjectPtr<UTObjectPtrDemoObject>> Objects;
    Objects.Add(Pointer);
    Result.IndexInArray = Objects.Find(Raw);
    AddStep(
        Result,
        TEXT("TArray<TObjectPtr<...>>::Find(Raw)"),
        FString::Printf(TEXT("returned index %d"), Result.IndexInArray)
    );

    // Cast<> resolves the dynamic type straight through a TObjectPtr.
    const TObjectPtr<UObject> Base = Pointer;
    Result.bCastSucceeded = (Cast<UTObjectPtrDemoObject>(Base) == Raw);
    AddStep(
        Result,
        TEXT("Cast<UTObjectPtrDemoObject>(Base)"),
        FString::Printf(TEXT("recovered derived pointer=%s"), BoolText(Result.bCastSucceeded))
    );

    // The reflection system sees a UPROPERTY() TObjectPtr, so the GC both
    // keeps the target alive and nulls the property when the target dies —
    // a raw pointer outside UPROPERTY would silently dangle instead.
    UTObjectPtrDemoObject* Owner = NewObject<UTObjectPtrDemoObject>();
    Owner->Next = Pointer;
    TArray<UObject*> Referenced;
    FReferenceFinder Collector(Referenced, nullptr, false, false, false, false);
    Collector.FindReferences(Owner);
    Result.bGCSeesReference = Referenced.Contains(Raw);
    AddStep(
        Result,
        TEXT("FReferenceFinder::FindReferences(Owner)"),
        FString::Printf(TEXT("GC sees UPROPERTY TObjectPtr reference=%s"), BoolText(Result.bGCSeesReference))
    );

    // IsValid() works through the implicit conversion and detects objects
    // marked as garbage before the GC has even run; after the next GC pass
    // the UPROPERTY() TObjectPtr itself would be reset to null.
    Result.bValidBeforeMark = IsValid(Pointer);
    Pointer->MarkAsGarbage();
    Result.bValidAfterMark = IsValid(Pointer);
    Owner->MarkAsGarbage();
    AddStep(
        Result,
        TEXT("MarkAsGarbage() + IsValid(Pointer)"),
        FString::Printf(
            TEXT("valid before=%s, after=%s"),
            BoolText(Result.bValidBeforeMark),
            BoolText(Result.bValidAfterMark)
        )
    );

    return Result;
}
