// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class USceneComponent;
class AActor;

/** Engine primitives every UE install ships in /Engine/BasicShapes. */
enum class EMarsShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
	Plane
};

/**
 * Asset lookups with graceful fallbacks, so the game runs from a fresh clone with zero
 * project assets and looks better once Content/Python/mars_setup.py has created materials.
 *
 * All materials are expected to expose a vector parameter named "Color"
 * (/Engine/BasicShapes/BasicShapeMaterial does; the generated M_Mars* materials do too)
 * and optionally a scalar "Glow".
 */
namespace MarsAssets
{
	MARSHELLOWORLD_API UStaticMesh* GetShape(EMarsShape Shape);

	/** Lit surface for bodies and rocks. /Game/Materials/M_MarsSurface, else BasicShapeMaterial. */
	MARSHELLOWORLD_API UMaterialInterface* GetSurfaceMaterial();

	/** Glowing surface for eyes, thrusters, projectiles. /Game/Materials/M_MarsEmissive, else surface. */
	MARSHELLOWORLD_API UMaterialInterface* GetEmissiveMaterial();

	/** Vertex-coloured, two-sided ground. /Game/Materials/M_MarsGround, else surface. */
	MARSHELLOWORLD_API UMaterialInterface* GetGroundMaterial();

	/** True when the editor setup script has created the M_Mars* materials. */
	MARSHELLOWORLD_API bool HasProjectMaterials();

	MARSHELLOWORLD_API UMaterialInstanceDynamic* MakeColoredMaterial(UObject* Outer, UMaterialInterface* Parent, const FLinearColor& Color, float Glow = 0.f);

	/**
	 * Creates, attaches and registers a primitive mesh component at runtime.
	 * Glow > 0 uses the emissive material. Collision is off (visual only).
	 */
	MARSHELLOWORLD_API UStaticMeshComponent* AddShape(AActor* Owner, USceneComponent* Parent, FName Name, EMarsShape Shape,
		const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& Scale,
		const FLinearColor& Color, float Glow = 0.f);
}
