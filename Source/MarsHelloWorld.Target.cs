// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

using UnrealBuildTool;
using System.Collections.Generic;

public class MarsHelloWorldTarget : TargetRules
{
	public MarsHelloWorldTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("MarsHelloWorld");
	}
}
