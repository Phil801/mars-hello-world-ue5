// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MarsArenaBuilder.generated.h"

/**
 * Editor-only helper that turns the runtime-generated Mars into real level actors:
 * a Landscape sampled from AMarsTerrain's height function, the rock field, the Mars sky as standalone
 * actors (SkyAtmosphere, ExponentialHeightFog, sun DirectionalLight, SkyLight, Phobos and Deimos),
 * AMarsShowCamera, AMarsMusicActor, a PlayerStart and one AMarsSpawnMarker per combatant.
 * Run it from Content/Python/mars_build_arena.py (or any editor Python: unreal.MarsArenaBuilder.build_mars_arena(world, True)).
 * Everything it creates is tagged "MarsArenaGenerated" so a rebuild replaces only its own actors.
 */
UCLASS()
class MARSHELLOWORLD_API UMarsArenaBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Builds the arena into World (an editor world). Returns a summary, or a message starting with "ERROR". */
	UFUNCTION(BlueprintCallable, Category = "Mars|Editor")
	static FString BuildMarsArena(UWorld* World, bool bReplaceExisting = true);

	/**
	 * Replaces only the sky/lighting: removes any AMarsEnvironment and previously generated Mars/Environment
	 * actors, then places standalone SkyAtmosphere, ExponentialHeightFog, DirectionalLight (sun), SkyLight and
	 * the two moon StaticMeshActors with exactly AMarsEnvironment's settings (including what it sets at BeginPlay).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mars|Editor")
	static FString BuildMarsEnvironment(UWorld* World);

	static const FName GeneratedTag;
};