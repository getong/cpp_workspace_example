using UnrealBuildTool;

public class UnrealCppDemoEditorTarget : TargetRules
{
    public UnrealCppDemoEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("UnrealCppDemo");
    }
}
