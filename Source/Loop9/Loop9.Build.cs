// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Loop9 : ModuleRules
{
	public Loop9(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"UMG",
			"Slate",
			"SlateCore",
			"MoviePlayer",
			"HTTP",
			"Json",
			"JsonUtilities",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Loop9",
			"Loop9/Anomaly",
			"Loop9/Anomaly/Pursuer",
			"Loop9/Characters",
			"Loop9/Interaction",
			"Loop9/Loop",
			"Loop9/AI",
			"Loop9/AI/Services",
			"Loop9/Subsystems",
			"Loop9/Controllers",
			"Loop9/UI"
		});
	}
}
