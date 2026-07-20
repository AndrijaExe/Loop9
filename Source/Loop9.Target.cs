// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Loop9Target : TargetRules
{
	public Loop9Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// UE 5.8: game targets share UnrealGame build products, so older
		// DefaultBuildSettings/IncludeOrderVersion need an explicit override.
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("Loop9");
	}
}
