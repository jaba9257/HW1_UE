using UnrealBuildTool;

public class HW2Tools : ModuleRules
{
    public HW2Tools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "HW1", "UnrealEd", "AIModule", "InputCore", "NavigationSystem", "PCG"
        });
    }
}
