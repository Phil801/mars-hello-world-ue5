// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsEnvironment.generated.h"

class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UStaticMeshComponent;

/**
 * Everything that makes the sky look like Mars, in one actor:
 * SkyAtmosphere tuned to Mars (3389.5 km radius, thin CO2 Rayleigh layer, heavy blue-absorbing dust
 * = butterscotch day sky and blue sunsets), dusty ExponentialHeightFog, a weaker/whiter sun
 * (Mars gets ~43% of Earth's sunlight), a real-time captured SkyLight, and Phobos/Deimos.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AMarsEnvironment();

	virtual void BeginPlay() override;

	/** Sun elevation in degrees above the horizon (lower = longer shadows, bluer halo near the sun). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mars")
	float SunElevation = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mars")
	float SunAzimuth = 135.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USkyLightComponent> SkyLight;

private:
	void AddMoons();
};
