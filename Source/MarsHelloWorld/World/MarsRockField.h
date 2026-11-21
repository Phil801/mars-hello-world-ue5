// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsRockField.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class AMarsTerrain;

/**
 * Level-placed rocks, filled by UMarsArenaBuilder from the terrain seed.
 * The builder lays rocks out for 100 cm engine shapes (round boulders and flat slabs); PlaceRocks swaps each
 * one for a random rock mesh with the same location, rotation and volume, one instanced component per mesh.
 */
UCLASS(meta = (DisplayName = "Mars Rock Field"))
class MARSHELLOWORLD_API AMarsRockField : public AActor
{
	GENERATED_BODY()

public:
	AMarsRockField();

	UInstancedStaticMeshComponent* GetBoulders() const { return Boulders; }
	UInstancedStaticMeshComponent* GetSlabs() const { return Slabs; }
	float GetPebbleDensity() const { return PebbleDensity; }
	/** Total pebbles placed: PebbleBaseCount * PebbleDensity. Independent of the rock layout. */
	int32 GetPebbleCount() const { return FMath::CeilToInt(PebbleBaseCount * PebbleDensity); }
	static constexpr int32 PebbleBaseCount = 50;
	/** Total boulders placed: BoulderBaseCount * BoulderDensity. Independent of the rock layout. */
	int32 GetBoulderCount() const { return FMath::RoundToInt(BoulderBaseCount * BoulderDensity); }
	static constexpr int32 BoulderBaseCount = 85;
	/** Total slabs placed: SlabBaseCount * SlabDensity. Independent of the rock layout. */
	int32 GetSlabCount() const { return FMath::RoundToInt(SlabBaseCount * SlabDensity); }
	static constexpr int32 SlabBaseCount = 50;
	float GetRockDensity() const { return RockDensity; }

	/** Replaces any existing rocks. Transforms are actor-local and sized for 100 cm engine shapes. */
	void PlaceRocks(const TArray<FTransform>& BoulderShapes, const TArray<FTransform>& SlabShapes, int32 Seed,
		const TArray<FTransform>& PebbleCandidates = TArray<FTransform>(), const TArray<FTransform>& BoulderCandidates = TArray<FTransform>(),
		const TArray<FTransform>& SlabCandidates = TArray<FTransform>());

	/** Extra scatter spots for pebbles, drawn from their own stream so the main rock layout is unchanged. */
	static void ComputePebbleCandidates(const AMarsTerrain* Height, int32 Seed, int32 Count, TArray<FTransform>& Out);

	/** Even scatter spots on their own stream (boulders use Seed, slabs use Seed ^ 0x51AB). */
	static void ComputeBoulderCandidates(const AMarsTerrain* Height, int32 Seed, int32 Count, TArray<FTransform>& Out);

#if WITH_EDITOR
	/** Swaps the sphere and cube instances for random rock meshes, keeping each one's placement. */
	UFUNCTION(CallInEditor, Category = "Mars|Rocks")
	void ReplaceShapesWithRocks();

	/** Re-scatters the rocks from the terrain seed (Project Settings > Mars Show) and AMarsTerrain's RockCount. */
	UFUNCTION(CallInEditor, Category = "Mars|Rocks")
	void RegenerateRocks();
#endif

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USceneComponent> Root;

	/** Legacy sphere instances (empty once rocks are placed). */
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UInstancedStaticMeshComponent> Boulders;

	/** Legacy cube instances (empty once rocks are placed). */
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UInstancedStaticMeshComponent> Slabs;

	/** Meshes used in place of the round boulder shapes. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks")
	TArray<TSoftObjectPtr<UStaticMesh>> BoulderMeshes;

	/** Meshes used in place of the flat slab shapes. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks")
	TArray<TSoftObjectPtr<UStaticMesh>> SlabMeshes;

	/** How much of each rock's height is sunk into the ground. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks", meta = (ClampMin = "0", ClampMax = "0.9"))
	float SinkFraction = 0.15f;

	/** Multiplier on AMarsTerrain::RockCount for the rock layout. 1 = the terrain's count; 2 = twice as many. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks", meta = (ClampMin = "0", ClampMax = "20"))
	float RockDensity = 1.f;

	/** Multiplier on the number of pebbles (meshes named Pebble): PebbleBaseCount (50) * this. Separate from RockDensity. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks", meta = (ClampMin = "1", ClampMax = "500"))
	float PebbleDensity = 100.f;

	/** Multiplier on the number of boulders (meshes named Boulder): BoulderBaseCount (85) * this. Separate from RockDensity. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks", meta = (ClampMin = "0", ClampMax = "20"))
	float BoulderDensity = 1.f;

	/** Multiplier on the number of slabs (meshes named Slab): SlabBaseCount (50) * this. Separate from RockDensity. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks", meta = (ClampMin = "0", ClampMax = "20"))
	float SlabDensity = 1.f;

	/** Boulder and Pillar meshes ignore the layout shape's size: random scale between X and Y times the mesh's own size. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks")
	FVector2D BoulderScale = FVector2D(1.f, 10.f);

	/** Pebbles ignore the layout shape's size: random scale between X and Y times the mesh's own size. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks")
	FVector2D PebbleScale = FVector2D(0.5f, 1.5f);

	/** Rock meshes (SM_Rock_*) ignore the layout shape's size: random scale between X and Y times the mesh's own size. */
	UPROPERTY(EditAnywhere, Category = "Mars|Rocks")
	FVector2D RockScale = FVector2D(0.5f, 1.5f);

	UPROPERTY(VisibleAnywhere, Category = "Mars|Rocks")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> RockComponents;

private:
	void ClearRockComponents();
	UInstancedStaticMeshComponent* CreateRockComponent(UStaticMesh* Mesh);
};