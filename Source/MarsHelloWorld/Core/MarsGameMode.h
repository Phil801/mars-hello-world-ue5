// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MarsGameMode.generated.h"

class AMarsTerrain;
class AMarsEnvironment;
class AMarsShowCamera;
class AMarsSpawnMarker;
class ALandscapeProxy;
enum class EEntryStyle : uint8;

/**
 * Uses the Mars actors placed in the level (MarsArena: Landscape, SkyAtmosphere/fog/sun/sky light, AMarsShowCamera,
 * PlayerStart, AMarsSpawnMarker) and spawns runtime stand-ins only for what is missing, so any map
 * (even an empty one) still plays. Then spawns the playable Earth
 * astronaut, then hands the show to UMarsShowSubsystem / UMarsConversationDirector:
 * entrances -> Kevin-hunt conversation -> accusation -> free-for-all -> victory.
 * Set as GlobalDefaultGameMode in Config/DefaultEngine.ini.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMarsGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

	/** Spectator-only show: no player pawn is spawned. */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	AMarsShowCamera* GetShowCamera() const { return ShowCamera; }
	AMarsTerrain* GetTerrain() const { return Terrain; }
	FVector GetArenaCenter() const { return ArenaCenter; }

private:
	void BuildWorld();
	void LaunchCombatant(int32 Index);
	void PlaceAstronaut(APlayerController* NewPlayer);
	float GroundZ(float X, float Y, float Fallback) const;
	FVector ComputeEntryStart(EEntryStyle Style, const FVector& ArrivalPoint, const FVector2D& ArriveDir, float ArriveAngle) const;

	UPROPERTY(Transient)
	TObjectPtr<AMarsTerrain> Terrain;

	/** Placed or fallback-spawned AMarsEnvironment; null when the map has its own sky/lighting actors. */
	UPROPERTY(Transient)
	TObjectPtr<AMarsEnvironment> Environment;

	UPROPERTY(Transient)
	TObjectPtr<AMarsShowCamera> ShowCamera;

	UPROPERTY(Transient)
	TObjectPtr<ALandscapeProxy> Landscape;

	/** Level-placed NPC spawn markers by CombatantIndex. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AMarsSpawnMarker>> SpawnMarkers;

	bool bWorldBuilt = false;

	FVector ArenaCenter = FVector::ZeroVector;
	int32 Seed = 801;
};
