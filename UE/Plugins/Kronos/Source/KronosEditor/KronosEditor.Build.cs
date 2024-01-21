// Copyright 2022-2023 Horizon Games. All Rights Reserved.

using UnrealBuildTool;

public class KronosEditor : ModuleRules
{
    public KronosEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });
		
		PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine", "UnrealEd", "Projects", "SlateCore", "PlacementMode", "Kronos" });
    }
}