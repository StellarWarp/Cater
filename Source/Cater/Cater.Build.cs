// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Cater : ModuleRules
{
	public Cater(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] {
			"Cater",
			"Cater/Gameplay",
			"Cater/Player",
			"Cater/UI",
			"Cater/System",
			"Cater/Online",
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine", 
			"InputCore",
			"HeadMountedDisplay", 
			"NavigationSystem",
			"AIModule",
			"UMG",
			"SlateCore",
			"Slate",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
		});
		
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
	}
}