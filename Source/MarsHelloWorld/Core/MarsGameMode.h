// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MarsGameMode.generated.h"

class AMarsTerrain;
class AMarsEnvironment;
class AMarsShowCamera;

/**
 * Builds Mars at runtime (no .umap content needed), then hands the show to
 * UMarsShowSubsystem / UMarsConversationDirector:
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

	UPROPERTY(Transient)
	TObjectPtr<AMarsTerrain> Terrain;

	UPROPERTY(Transient)
	TObjectPtr<AMarsEnvironment> Environment;

	UPROPERTY(Transient)
	TObjectPtr<AMarsShowCamera> ShowCamera;

	FVector ArenaCenter = FVector::ZeroVector;
	int32 Seed = 801;
};
