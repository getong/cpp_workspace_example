#include "TArrayDemo.h"
#include "TObjectPtrDemo.h"

#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTArrayDemo, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogTObjectPtrDemo, Log, All);

class FUnrealCppDemoModule final : public FDefaultGameModuleImpl
{
public:
    void StartupModule() override
    {
        const FTArrayDemoResult TArrayResult = RunTArrayDemo();

        for (const FTArrayDemoStep& Step : TArrayResult.Steps)
        {
            UE_LOG(LogTArrayDemo, Display, TEXT("TARRAY_DEMO|%s -> %s"), *Step.Function, *Step.Result);
        }

        const FTObjectPtrDemoResult ObjectPtrResult = RunTObjectPtrDemo();

        for (const FTObjectPtrDemoStep& Step : ObjectPtrResult.Steps)
        {
            UE_LOG(
                LogTObjectPtrDemo,
                Display,
                TEXT("TOBJECTPTR_DEMO|%s -> %s"),
                *Step.Operation,
                *Step.Result
            );
        }
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCppDemoModule, UnrealCppDemo, "UnrealCppDemo");
