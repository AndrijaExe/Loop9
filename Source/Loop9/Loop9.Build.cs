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
			"StateTreeModule",
			"GameplayStateTreeModule",
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
			"Loop9/Variant_Horror",
			"Loop9/Variant_Horror/UI",
			"Loop9/Variant_Shooter",
			"Loop9/Variant_Shooter/AI",
			"Loop9/Variant_Shooter/UI",
			"Loop9/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
