// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

using UnrealBuildTool;

public class MarsHelloWorld : ModuleRules
{
	public MarsHelloWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets us write #include "Combat/CombatantCharacter.h" from anywhere in the module
		// (same trick as tomlooman/ActionRoguelike).
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"AIModule",
			"DeveloperSettings",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
