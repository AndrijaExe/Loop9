// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Loop9Target : TargetRules
{
	public Loop9Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// UE 5.8.2 requires V7 for Shared (installed-engine) targets.
		// Keep the 5.6 include order so existing game code does not need a
		// full include-order pass.
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("Loop9");
	}
}
