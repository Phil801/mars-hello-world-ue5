// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsTerrain.generated.h"

class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;

/** A crater stamped into the heightfield. */
USTRUCT()
struct FMarsCrater
{
	GENERATED_BODY()

	FVector2D Center = FVector2D::ZeroVector;
	float Radius = 1000.f;
	float Depth = 200.f;
};

/**
 * Procedural Mars: a heightfield mesh (fractal dunes, far ridges, craters, a raised horizon bowl
 * and a flattened arena in the middle) plus instanced boulders made from engine basic shapes.
 * No .umap or landscape asset required; GetHeightAt() is a pure function so gameplay can query
 * ground height without traces.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsTerrain : public AActor
{
	GENERATED_BODY()

public:
	AMarsTerrain();

	/** Builds the mesh and scatters rocks. Deterministic for a given seed. */
	UFUNCTION(BlueprintCallable, Category = "Mars")
	void BuildTerrain(int32 Seed);

	/** World-space ground height (cm) at X/Y. Valid after BuildTerrain (craters depend on the seed). */
	UFUNCTION(BlueprintPure, Category = "Mars")
	float GetHeightAt(float X, float Y) const;

	UFUNCTION(BlueprintPure, Category = "Mars")
	float GetArenaRadius() const { return ArenaRadius; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UProceduralMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UInstancedStaticMeshComponent> Boulders;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UInstancedStaticMeshComponent> Slabs;

	/** Vertices per side. 201 x 201 = ~80k triangles, one draw call. */
	UPROPERTY(EditAnywhere, Category = "Mars", meta = (ClampMin = "16", ClampMax = "512"))
	int32 GridResolution = 201;

	/** Half-width of the terrain square (cm). 30000 = 600 m across. */
	UPROPERTY(EditAnywhere, Category = "Mars")
	float HalfExtent = 30000.f;

	/** Flat-ish fighting area radius (cm) around the origin. */
	UPROPERTY(EditAnywhere, Category = "Mars")
	float ArenaRadius = 1700.f;

	UPROPERTY(EditAnywhere, Category = "Mars")
	int32 RockCount = 260;

	/** Flip triangle winding if the ground renders inside-out on your engine build (see CHECKS.md). */
	UPROPERTY(EditAnywhere, Category = "Mars")
	bool bFlipWinding = false;

private:
	float SampleFractal(float X, float Y, int32 Octaves) const;
	FLinearColor GroundColorAt(float X, float Y, float Height, const FVector& Normal) const;
	void ScatterRocks(FRandomStream& Stream);

	FVector2D NoiseOffset = FVector2D::ZeroVector;
	TArray<FMarsCrater> Craters;
};
