// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Camera/MarsShowCamera.h"
#include "Camera/CameraComponent.h"
#include "Core/MarsShowSubsystem.h"
#include "Combat/CombatantCharacter.h"
#include "Engine/World.h"

AMarsShowCamera::AMarsShowCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // after the combatants moved this frame

	UCameraComponent* Cam = GetCameraComponent();
	Cam->SetFieldOfView(70.f);
	Cam->bConstrainAspectRatio = false;

	// Mars grade: warm, slightly hazy, a touch of film grain and vignette.
	FPostProcessSettings& PP = Cam->PostProcessSettings;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(1.08f, 1.0f, 0.92f, 1.0f);
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(1.05f, 0.98f, 0.92f, 1.0f);
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.45f;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.9f;
	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = 0.12f;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.3f;
}

void AMarsShowCamera::BeginPlay()
{
	Super::BeginPlay();
	SmoothedLookAt = ArenaCenter;
}

void AMarsShowCamera::CycleMode()
{
	Mode = static_cast<EMarsCameraMode>((static_cast<uint8>(Mode) + 1) % 3);
}

FVector AMarsShowCamera::Centroid(const TArray<ACombatantCharacter*>& Actors, const FVector& Fallback, float& OutSpread)
{
	OutSpread = 0.f;
	if (Actors.Num() == 0)
	{
		return Fallback;
	}
	FVector Sum = FVector::ZeroVector;
	for (const ACombatantCharacter* A : Actors)
	{
		Sum += A->GetActorLocation();
	}
	const FVector C = Sum / Actors.Num();
	for (const ACombatantCharacter* A : Actors)
	{
		OutSpread = FMath::Max(OutSpread, static_cast<float>(FVector::Dist(C, A->GetActorLocation())));
	}
	return C;
}

void AMarsShowCamera::ComputeDesired(UMarsShowSubsystem* Show, float Time, FVector& OutLocation, FVector& OutLookAt, float& OutFOV) const
{
	OutFOV = 70.f;
	const float Yaw = FMath::DegreesToRadians(OrbitYaw);
	const FVector Around(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f);

	if (!Show)
	{
		OutLookAt = ArenaCenter;
		OutLocation = ArenaCenter + Around * 2600.f + FVector(0.f, 0.f, 900.f);
		return;
	}

	float Spread = 0.f;
	const TArray<ACombatantCharacter*> Alive = Show->GetCombatants(true);
	const FVector Brawl = Centroid(Alive, ArenaCenter, Spread);
	const EMarsShowPhase Phase = Show->GetPhase();

	if (Mode == EMarsCameraMode::WideOrbit)
	{
		OutLookAt = (Brawl + ArenaCenter) * 0.5f + FVector(0.f, 0.f, 80.f);
		OutLocation = ArenaCenter + Around * 3200.f + FVector(0.f, 0.f, 1500.f);
		OutFOV = 65.f;
		return;
	}
	if (Mode == EMarsCameraMode::LowAction)
	{
		OutLookAt = Brawl + FVector(0.f, 0.f, 120.f);
		OutLocation = Brawl + Around * (700.f + Spread * 1.1f) + FVector(0.f, 0.f, 90.f);
		OutFOV = 82.f;
		return;
	}

	// Director mode.
	switch (Phase)
	{
	case EMarsShowPhase::Arrivals:
	{
		// Wide establishing shot; widen to include incoming fliers/runners.
		const TArray<ACombatantCharacter*> All = Show->GetCombatants(false);
		const FVector Mid = Centroid(All, ArenaCenter, Spread);
		OutLookAt = FMath::Lerp(ArenaCenter, Mid, 0.55f) + FVector(0.f, 0.f, 150.f);
		const float Dist = FMath::Clamp(2200.f + Spread * 0.7f, 2200.f, 7500.f);
		OutLocation = OutLookAt + Around * Dist + FVector(0.f, 0.f, 500.f + Spread * 0.15f);
		OutFOV = 72.f;
		break;
	}
	case EMarsShowPhase::Conversation:
	{
		if (ACombatantCharacter* Speaker = Show->GetCurrentSpeaker())
		{
			// Front-on medium close-up of whoever is saying hello.
			const FVector Head = Speaker->GetHeadPoint();
			const FVector Fwd = Speaker->GetActorForwardVector().GetSafeNormal2D();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd);
			OutLookAt = Head - FVector(0.f, 0.f, 25.f);
			OutLocation = Head + Fwd * 520.f + Side * 140.f + FVector(0.f, 0.f, 30.f);
			OutFOV = 55.f;
		}
		else
		{
			OutLookAt = Brawl + FVector(0.f, 0.f, 120.f);
			OutLocation = Brawl + Around * 1500.f + FVector(0.f, 0.f, 380.f);
		}
		break;
	}
	case EMarsShowPhase::Accusation:
	{
		if (ACombatantCharacter* Speaker = Show->GetCurrentSpeaker())
		{
			const FVector Head = Speaker->GetHeadPoint();
			const FVector Fwd = Speaker->GetActorForwardVector().GetSafeNormal2D();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd);
			OutLookAt = Head - FVector(0.f, 0.f, 25.f);
			OutLocation = Head + Fwd * 520.f + Side * 140.f + FVector(0.f, 0.f, 30.f);
			OutFOV = 55.f;
		}
		else
		{
			// Slow push-in on the line-up between spoken cues.
			const float T = FMath::Clamp(Time - Show->GetPhaseStartTime(), 0.f, 4.f) / 4.f;
			OutLookAt = Brawl + FVector(0.f, 0.f, 120.f);
			OutLocation = Brawl + Around * FMath::Lerp(2100.f, 1500.f, T) + FVector(0.f, 0.f, FMath::Lerp(700.f, 260.f, T));
			OutFOV = 68.f;
		}
		break;
	}
	case EMarsShowPhase::Fight:
	{
		OutLookAt = Brawl + FVector(0.f, 0.f, 110.f);
		const float Dist = FMath::Clamp(900.f + Spread * 1.6f, 1000.f, 3400.f);
		OutLocation = Brawl + Around * Dist + FVector(0.f, 0.f, 260.f + Spread * 0.25f);
		OutFOV = 74.f;
		break;
	}
	case EMarsShowPhase::Victory:
	default:
	{
		const ACombatantCharacter* Winner = Show->GetWinner();
		const FVector Focus = Winner ? Winner->GetActorLocation() : ArenaCenter;
		OutLookAt = Focus + FVector(0.f, 0.f, 90.f);
		OutLocation = Focus + Around * 650.f + FVector(0.f, 0.f, 140.f);
		OutFOV = 60.f;
		break;
	}
	}
}

void AMarsShowCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	UMarsShowSubsystem* Show = World ? World->GetSubsystem<UMarsShowSubsystem>() : nullptr;
	const float Time = World ? World->GetTimeSeconds() : 0.f;

	// Orbit speed depends on the phase: lazy during conversation, brisk during the fight.
	float OrbitSpeed = 6.f;
	if (Show)
	{
		switch (Show->GetPhase())
		{
		case EMarsShowPhase::Fight: OrbitSpeed = 9.f; break;
		case EMarsShowPhase::Victory: OrbitSpeed = 18.f; break;
		case EMarsShowPhase::Conversation: case EMarsShowPhase::Accusation: OrbitSpeed = 2.f; break;
		default: break;
		}
	}
	if (Mode == EMarsCameraMode::WideOrbit)
	{
		OrbitSpeed = 7.f;
	}
	OrbitYaw = FMath::Fmod(OrbitYaw + OrbitSpeed * DeltaSeconds, 360.f);

	FVector DesiredLoc, DesiredLook;
	float DesiredFOV;
	ComputeDesired(Show, Time, DesiredLoc, DesiredLook, DesiredFOV);

	// Never go underground (terrain + clearance).
	if (Show)
	{
		const float Ground = Show->GetGroundZ(DesiredLoc.X, DesiredLoc.Y, ArenaCenter.Z);
		DesiredLoc.Z = FMath::Max(DesiredLoc.Z, Ground + 140.f);
	}

	if (!bInitialised)
	{
		SetActorLocation(DesiredLoc);
		SmoothedLookAt = DesiredLook;
		bInitialised = true;
	}

	const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), DesiredLoc, DeltaSeconds, 2.2f);
	SmoothedLookAt = FMath::VInterpTo(SmoothedLookAt, DesiredLook, DeltaSeconds, 4.0f);
	FRotator Rot = (SmoothedLookAt - NewLoc).Rotation();

	// Trauma-based shake (trauma^2 so small hits barely register, big ones really kick).
	if (Show)
	{
		const float Trauma = Show->ConsumeCameraTrauma(DeltaSeconds);
		const float Shake = Trauma * Trauma;
		NoiseTime += DeltaSeconds * 22.f;
		Rot.Pitch += Shake * 4.f * FMath::PerlinNoise1D(NoiseTime + 11.3f);
		Rot.Yaw += Shake * 4.f * FMath::PerlinNoise1D(NoiseTime + 47.9f);
		Rot.Roll += Shake * 2.5f * FMath::PerlinNoise1D(NoiseTime + 83.1f);
	}

	SetActorLocationAndRotation(NewLoc, Rot);
	UCameraComponent* Cam = GetCameraComponent();
	Cam->SetFieldOfView(FMath::FInterpTo(Cam->FieldOfView, DesiredFOV, DeltaSeconds, 2.f));
}
