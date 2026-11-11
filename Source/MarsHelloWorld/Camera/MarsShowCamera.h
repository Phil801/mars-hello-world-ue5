// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "MarsShowCamera.generated.h"

class ACombatantCharacter;
class UMarsShowSubsystem;

UENUM()
enum class EMarsCameraMode : uint8
{
	Director,   // auto: follows whoever is talking / the brawl / the winner
	WideOrbit,  // slow orbit of the whole arena
	LowAction   // low, close, dramatic tracking of the brawl centroid
};

/** Spectator "director" camera. Frames the focus actors, stays above the terrain, shakes on big hits. */
UCLASS()
class MARSHELLOWORLD_API AMarsShowCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AMarsShowCamera(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	void SetArenaCenter(const FVector& InCenter) { ArenaCenter = InCenter; }
	void CycleMode();
	EMarsCameraMode GetMode() const { return Mode; }

protected:
	virtual void BeginPlay() override;

private:
	void ComputeDesired(UMarsShowSubsystem* Show, float Time, FVector& OutLocation, FVector& OutLookAt, float& OutFOV) const;
	static FVector Centroid(const TArray<ACombatantCharacter*>& Actors, const FVector& Fallback, float& OutSpread);

	FVector ArenaCenter = FVector::ZeroVector;
	EMarsCameraMode Mode = EMarsCameraMode::Director;
	FVector SmoothedLookAt = FVector::ZeroVector;
	float OrbitYaw = 200.f;
	float NoiseTime = 0.f;
	bool bInitialised = false;
};
