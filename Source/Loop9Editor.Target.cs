// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Loop9EditorTarget : TargetRules
{
	public Loop9EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("Loop9");
	}
}
