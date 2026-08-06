#include "TArrayDemo.h"

#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTArrayDemo, Log, All);

class FUnrealCppDemoModule final : public FDefaultGameModuleImpl
{
public:
    void StartupModule() override
    {
        const FTArrayDemoResult Result = RunTArrayDemo();

        for (const FTArrayDemoStep& Step : Result.Steps)
        {
            UE_LOG(LogTArrayDemo, Display, TEXT("TARRAY_DEMO|%s -> %s"), *Step.Function, *Step.Result);
        }
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCppDemoModule, UnrealCppDemo, "UnrealCppDemo");
