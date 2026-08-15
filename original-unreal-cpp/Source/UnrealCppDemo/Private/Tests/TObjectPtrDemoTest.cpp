#include "TObjectPtrDemo.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTObjectPtrDemoTest,
    "UnrealCppDemo.UObject.TObjectPtr",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTObjectPtrDemoTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FTObjectPtrDemoResult Result = RunTObjectPtrDemo();

    TestTrue(TEXT("Default-constructed TObjectPtr is null"), Result.bDefaultIsNull);
    TestTrue(
        TEXT("TObjectPtr has the same size as a raw pointer"),
        Result.ObjectPtrSize == Result.RawPtrSize
    );
    TestEqual(TEXT("operator-> accesses members like a raw pointer"), Result.IdThroughArrow, 42);
    TestEqual(TEXT("Implicit conversion feeds raw-pointer APIs"), Result.IdThroughRawCall, 42);
    TestTrue(TEXT("TObjectPtr compares equal to the raw pointer"), Result.bEqualsRawPointer);
    TestEqual(TEXT("TArray<TObjectPtr> finds elements by raw pointer"), Result.IndexInArray, 0);
    TestTrue(TEXT("Cast recovers the derived type through TObjectPtr"), Result.bCastSucceeded);
    TestTrue(TEXT("GC reference collector sees the UPROPERTY TObjectPtr"), Result.bGCSeesReference);
    TestTrue(TEXT("IsValid is true before MarkAsGarbage"), Result.bValidBeforeMark);
    TestFalse(TEXT("IsValid detects the object marked as garbage"), Result.bValidAfterMark);
    TestEqual(TEXT("Every demonstrated operation is recorded"), Result.Steps.Num(), 10);

    return true;
}

#endif
