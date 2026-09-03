// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Loop9EditorTarget : TargetRules
{
	public Loop9EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// UE 5.8.2 aborts editor startup for Shared targets that are not V7
		// ("Target Upgrade Required"). Keep the 5.6 include order so existing
		// game code does not need a full include-order pass.
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("Loop9");
	}
}
