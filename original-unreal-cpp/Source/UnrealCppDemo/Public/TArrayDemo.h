#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"

struct FTArrayDemoStep
{
    FString Function;
    FString Result;
};

struct FTArrayDemoResult
{
    TArray<FTArrayDemoStep> Steps;
    TArray<int32> Values;
    int32 Sum = 0;
    int32 Capacity = 0;
    int32 IndexOf30 = INDEX_NONE;
    SIZE_T AllocatedBytes = 0;
    bool bContains20 = false;
    bool bIsContiguous = false;
};

UNREALCPPDEMO_API FTArrayDemoResult RunTArrayDemo();
