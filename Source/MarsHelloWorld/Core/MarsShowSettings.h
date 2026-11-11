// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MarsShowSettings.generated.h"

class ACombatantCharacter;

/**
 * Project Settings > Game > Mars Show. Stored in Config/DefaultGame.ini under
 * [/Script/MarsHelloWorld.MarsShowSettings]. Data, not code: tune the show without recompiling.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mars Show"))
class MARSHELLOWORLD_API UMarsShowSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMarsShowSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Seed for every random choice in the show (rock layout, AI choices). Same seed = same-ish fight. */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	int32 RandomSeed = 801;

	/** Mars surface gravity in cm/s^2 (3.71 m/s^2). Earth is -980. */
	UPROPERTY(Config, EditAnywhere, Category = "World")
	float MarsGravityZ = -371.f;

	/** Combatants in entrance order. Defaults to Martian, X-Bot, Venusian (set in the constructor). */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	TArray<TSoftClassPtr<ACombatantCharacter>> CombatantClasses;

	/** Seconds after start when each combatant is launched toward the arena (index matches CombatantClasses). */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	TArray<float> EntranceDelays;

	/** Radius (cm) of the ring where combatants stop to greet. */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	float ArrivalRingRadius = 950.f;

	/** Pause between "FIGHT!" countdown steps. */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	float CountdownStepSeconds = 0.8f;

	/** After this many seconds of fighting, damage starts ramping up so the show always ends (~60-90 s total). */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	float SuddenDeathAfterSeconds = 50.f;

	/** Extra damage multiplier gained per second of sudden death. */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	float SuddenDeathRampPerSecond = 0.08f;

	/** Restart automatically this many seconds after the victory banner (0 = never). */
	UPROPERTY(Config, EditAnywhere, Category = "Show")
	float AutoRestartSeconds = 0.f;
};
