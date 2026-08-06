#include "TArrayDemo.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTArrayDemoTest,
    "UnrealCppDemo.Containers.TArray",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTArrayDemoTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FTArrayDemoResult Result = RunTArrayDemo();
    constexpr int32 ExpectedValues[] = {10, 20, 20, 30, 50};
    constexpr int32 ExpectedNum = static_cast<int32>(UE_ARRAY_COUNT(ExpectedValues));

    TestEqual(TEXT("Num tracks constructed elements"), Result.Values.Num(), ExpectedNum);
    TestTrue(TEXT("Max is the allocated capacity"), Result.Capacity >= Result.Values.Num());
    TestTrue(
        TEXT("Allocated storage covers all elements"),
        Result.AllocatedBytes >= static_cast<SIZE_T>(Result.Values.Num()) * sizeof(int32)
    );
    TestTrue(TEXT("GetData points at contiguous storage"), Result.bIsContiguous);
    TestTrue(TEXT("Contains locates an existing value"), Result.bContains20);
    TestEqual(TEXT("Find returns the sorted index"), Result.IndexOf30, 3);
    TestEqual(TEXT("Range iteration can aggregate values"), Result.Sum, 130);
    TestEqual(TEXT("Every demonstrated operation is recorded"), Result.Steps.Num(), 14);

    for (int32 Index = 0; Index < Result.Values.Num() && Index < ExpectedNum; ++Index)
    {
        TestEqual(TEXT("Sort orders values and preserves duplicates"), Result.Values[Index], ExpectedValues[Index]);
    }

    return true;
}

#endif
