// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/MarsHitFlash.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AMarsHitFlash::AMarsHitFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->IntensityUnits = ELightUnits::Unitless;
	Light->Intensity = 0.f;
	Light->AttenuationRadius = 600.f;
	Light->CastShadows = false;
}

AMarsHitFlash* AMarsHitFlash::Spawn(UWorld* World, const FVector& Location, const FLinearColor& Color, float InMaxScale, float InLifetime, EMarsShape Shape)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMarsHitFlash* Flash = World->SpawnActor<AMarsHitFlash>(AMarsHitFlash::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Flash)
	{
		Flash->Setup(Color, InMaxScale, InLifetime, Shape);
	}
	return Flash;
}

void AMarsHitFlash::Setup(const FLinearColor& InColor, float InMaxScale, float InLifetime, EMarsShape InShape)
{
	MaxScale = FMath::Max(0.05f, InMaxScale);
	Lifetime = FMath::Max(0.05f, InLifetime);
	bFlat = (InShape == EMarsShape::Cylinder);

	Visual = MarsAssets::AddShape(this, Root, TEXT("Visual"), InShape, FVector::ZeroVector, FRotator::ZeroRotator,
		FVector(0.01f), InColor, /*Glow*/ 12.f);
	if (Visual)
	{
		Visual->SetCastShadow(false);
	}
	Light->SetLightColor(InColor);
	Light->AttenuationRadius = 300.f + 150.f * MaxScale;
	PeakLight = 8000.f + 6000.f * MaxScale;
	SetLifeSpan(Lifetime + 0.05f);
}

void AMarsHitFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	const float T = FMath::Clamp(Elapsed / Lifetime, 0.f, 1.f);

	// Fast pop to full size in the first 25%, then shrink away.
	const float Grow = T < 0.25f ? FMath::InterpEaseOut(0.f, 1.f, T / 0.25f, 2.f) : 1.f - FMath::InterpEaseIn(0.f, 1.f, (T - 0.25f) / 0.75f, 2.f);
	const float S = FMath::Max(0.01f, Grow * MaxScale);
	if (Visual)
	{
		Visual->SetRelativeScale3D(bFlat ? FVector(S, S, 0.04f) : FVector(S));
	}
	Light->SetIntensity(PeakLight * (1.f - T) * (1.f - T));
}
