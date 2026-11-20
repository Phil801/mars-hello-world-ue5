// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsSpawnMarker.generated.h"

class UArrowComponent;
class UBillboardComponent;

/**
 * Level-placed start marker for one NPC combatant (index into UMarsShowSettings::CombatantClasses).
 * The actor location is the arrival point on the arena ring; EntryStart (a 3D widget in the editor)
 * is where the combatant appears before running / flying / hovering in. AMarsGameMode reads these
 * and falls back to its ring math when a marker is missing. The spawning logic itself stays in C++.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Mars Spawn Marker"))
class MARSHELLOWORLD_API AMarsSpawnMarker : public AActor
{
	GENERATED_BODY()

public:
	AMarsSpawnMarker();

	/** Which entry in UMarsShowSettings::CombatantClasses this marker is for. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mars")
	int32 CombatantIndex = 0;

	/** If false, the game mode computes the entry start from the combatant's entry style instead of EntryStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mars")
	bool bUseEntryStart = true;

	/** Entry start point, relative to this marker. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mars", meta = (MakeEditWidget = "true", EditCondition = "bUseEntryStart"))
	FVector EntryStart = FVector(0.f, 0.f, 0.f);

	UFUNCTION(BlueprintPure, Category = "Mars")
	FVector GetArrivalLocation() const { return GetActorLocation(); }

	UFUNCTION(BlueprintPure, Category = "Mars")
	FVector GetEntryStartLocation() const { return GetActorTransform().TransformPosition(EntryStart); }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UArrowComponent> Arrow;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UBillboardComponent> Billboard;
#endif
};
