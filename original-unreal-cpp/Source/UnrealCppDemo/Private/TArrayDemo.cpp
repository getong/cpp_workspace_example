#include "TArrayDemo.h"

#include "Templates/UnrealTemplate.h"

namespace
{
FString FormatValues(const TArray<int32>& Values)
{
    FString ValuesText;

    for (const int32 Value : Values)
    {
        if (!ValuesText.IsEmpty())
        {
            ValuesText += TEXT(", ");
        }
        ValuesText += FString::FromInt(Value);
    }

    return FString::Printf(TEXT("[%s]"), *ValuesText);
}

FString FormatState(const TArray<int32>& Values)
{
    return FString::Printf(
        TEXT("Num()=%d, Max()=%d, Values=%s"),
        Values.Num(),
        Values.Max(),
        *FormatValues(Values)
    );
}

void AddStep(FTArrayDemoResult& Demo, const TCHAR* Function, FString Result)
{
    Demo.Steps.Add({Function, MoveTemp(Result)});
}
} // namespace

FTArrayDemoResult RunTArrayDemo()
{
    FTArrayDemoResult Result;
    TArray<int32> Values;
    Values.Reserve(8);
    AddStep(Result, TEXT("Reserve(8)"), FormatState(Values));

    const int32 AddedIndex = Values.Add(30);
    AddStep(
        Result,
        TEXT("Add(30)"),
        FString::Printf(TEXT("returned index %d; %s"), AddedIndex, *FormatState(Values))
    );

    const int32 EmplacedIndex = Values.Emplace(10);
    AddStep(
        Result,
        TEXT("Emplace(10)"),
        FString::Printf(TEXT("returned index %d; %s"), EmplacedIndex, *FormatState(Values))
    );

    Values.Append({20, 40, 20});
    AddStep(Result, TEXT("Append({20, 40, 20})"), FormatState(Values));

    const int32 RemovedCount = Values.RemoveSingle(40);
    AddStep(
        Result,
        TEXT("RemoveSingle(40)"),
        FString::Printf(TEXT("removed %d element; %s"), RemovedCount, *FormatState(Values))
    );

    const int32 UniqueIndex = Values.AddUnique(50);
    AddStep(
        Result,
        TEXT("AddUnique(50)"),
        FString::Printf(TEXT("returned index %d; %s"), UniqueIndex, *FormatState(Values))
    );

    Values.Sort();
    AddStep(Result, TEXT("Sort()"), FormatState(Values));

    Result.bContains20 = Values.Contains(20);
    AddStep(
        Result,
        TEXT("Contains(20)"),
        FString::Printf(TEXT("returned %s"), Result.bContains20 ? TEXT("true") : TEXT("false"))
    );

    Result.IndexOf30 = Values.Find(30);
    AddStep(
        Result,
        TEXT("Find(30)"),
        FString::Printf(TEXT("returned index %d"), Result.IndexOf30)
    );

    int32 Sum = 0;
    bool bIsContiguous = true;
    const int32* Data = Values.GetData();

    for (int32 Index = 0; Index < Values.Num(); ++Index)
    {
        Sum += Values[Index];
        bIsContiguous = bIsContiguous && (&Values[Index] == Data + Index);
    }

    AddStep(
        Result,
        TEXT("GetData()"),
        FString::Printf(TEXT("contiguous storage=%s"), bIsContiguous ? TEXT("true") : TEXT("false"))
    );
    AddStep(Result, TEXT("Num()"), FString::Printf(TEXT("returned %d"), Values.Num()));
    AddStep(Result, TEXT("Max()"), FString::Printf(TEXT("returned %d"), Values.Max()));
    AddStep(
        Result,
        TEXT("GetAllocatedSize()"),
        FString::Printf(TEXT("returned %llu bytes"), static_cast<uint64>(Values.GetAllocatedSize()))
    );
    AddStep(Result, TEXT("range-based for"), FString::Printf(TEXT("sum=%d"), Sum));

    Result.Sum = Sum;
    Result.Capacity = Values.Max();
    Result.AllocatedBytes = Values.GetAllocatedSize();
    Result.bIsContiguous = bIsContiguous;
    Result.Values = MoveTemp(Values);
    return Result;
}
