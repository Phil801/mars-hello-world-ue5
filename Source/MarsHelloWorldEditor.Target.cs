// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

using UnrealBuildTool;
using System.Collections.Generic;

public class MarsHelloWorldEditorTarget : TargetRules
{
	public MarsHelloWorldEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("MarsHelloWorld");
	}
}
