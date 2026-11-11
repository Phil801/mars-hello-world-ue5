// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/MarsAssets.h"
#include "MarsHitFlash.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/**
 * Niagara-free hit VFX: a primitive that pops out and shrinks away plus a flash of light.
 * Cheap, deterministic, needs no assets. Replace with a Niagara system later via SpawnSystemAtLocation.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsHitFlash : public AActor
{
	GENERATED_BODY()

public:
	AMarsHitFlash();

	static AMarsHitFlash* Spawn(UWorld* World, const FVector& Location, const FLinearColor& Color, float MaxScale, float Lifetime, EMarsShape Shape);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Visual;

private:
	void Setup(const FLinearColor& InColor, float InMaxScale, float InLifetime, EMarsShape InShape);

	float MaxScale = 1.f;
	float Lifetime = 0.3f;
	float Elapsed = 0.f;
	float PeakLight = 20000.f;
	bool bFlat = false;
};
