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
			"MovieScene",
			"LevelSequence",
			"HTTP",
			"Json",
			"JsonUtilities",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		// Direct Steamworks SDK access for features the Online Subsystem does not
		// expose: Steam Deck on-screen keyboard and achievement progress toasts.
		if (Target.Platform == UnrealTargetPlatform.Win64 ||
			Target.Platform == UnrealTargetPlatform.Linux ||
			Target.Platform == UnrealTargetPlatform.Mac)
		{
			PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
			PrivateDefinitions.Add("LOOP9_WITH_STEAM=1");
		}
		else
		{
			PrivateDefinitions.Add("LOOP9_WITH_STEAM=0");
		}

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
			"Loop9/UI",
			"Loop9/Camera"
		});
	}
}
