// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsEnvironment.h"
#include "World/MarsAssets.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"

AMarsEnvironment::AMarsEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// --- Sky atmosphere (units: km and 1/km) -------------------------------------------------
	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(Root);
	SkyAtmosphere->BottomRadius = 3389.5f;            // Mars mean radius
	SkyAtmosphere->AtmosphereHeight = 80.f;
	SkyAtmosphere->GroundAlbedo = FColor(178, 98, 58);
	// Thin CO2 atmosphere: keep Rayleigh's physical blue tint but ~1/10th of Earth's strength.
	SkyAtmosphere->RayleighScatteringScale = 0.0035f;
	SkyAtmosphere->RayleighScattering = FLinearColor(0.175287f, 0.409607f, 1.f);
	SkyAtmosphere->RayleighExponentialDistribution = 11.1f; // Mars scale height ~11 km
	// Suspended iron-oxide dust dominates: strong forward-scattering Mie that absorbs blue/green.
	SkyAtmosphere->MieScatteringScale = 0.014f;
	SkyAtmosphere->MieScattering = FLinearColor(1.f, 0.70f, 0.48f);
	SkyAtmosphere->MieAbsorptionScale = 0.010f;
	SkyAtmosphere->MieAbsorption = FLinearColor(0.10f, 0.34f, 0.80f);
	SkyAtmosphere->MieAnisotropy = 0.76f;
	SkyAtmosphere->MieExponentialDistribution = 10.f;
	SkyAtmosphere->OtherAbsorptionScale = 0.f;           // no ozone layer on Mars
	SkyAtmosphere->MultiScatteringFactor = 1.f;

	// --- Dust haze -------------------------------------------------------------------------------
	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(Root);

	// --- Sun -------------------------------------------------------------------------------------
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->Intensity = 4.6f;                              // lux; Earth default sun is ~10 in UE units
	Sun->LightColor = FColor(255, 243, 226);
	Sun->CastShadows = true;

	// --- Sky light (ambient from the captured sky) -------------------------------------------
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLight->bLowerHemisphereIsBlack = false;
	SkyLight->LowerHemisphereColor = FLinearColor(0.22f, 0.09f, 0.04f);
	SkyLight->Intensity = 1.f;
}

void AMarsEnvironment::BeginPlay()
{
	Super::BeginPlay();

	Sun->SetWorldRotation(FRotator(-SunElevation, SunAzimuth, 0.f));
	Sun->SetAtmosphereSunLight(true);

	// Fog colour/density through the BlueprintCallable setters (stable across 5.x renames).
	HeightFog->SetFogDensity(0.03f);
	HeightFog->SetFogHeightFalloff(0.12f);
	HeightFog->SetFogInscatteringColor(FLinearColor(0.52f, 0.25f, 0.11f));
	HeightFog->SetFogMaxOpacity(0.92f);
	HeightFog->SetStartDistance(1500.f);
	HeightFog->SetVolumetricFog(true);
	HeightFog->SetVolumetricFogAlbedo(FColor(230, 160, 120));

	SkyLight->RecaptureSky();

	AddMoons();
}

void AMarsEnvironment::AddMoons()
{
	// Phobos (big, close) and Deimos (tiny dot) hanging in the sky opposite the sun.
	const FVector PhobosDir = FRotator(32.f, SunAzimuth + 150.f, 0.f).Vector();
	const FVector DeimosDir = FRotator(48.f, SunAzimuth + 205.f, 0.f).Vector();

	MarsAssets::AddShape(this, Root, TEXT("Phobos"), EMarsShape::Sphere, PhobosDir * 180000.f,
		FRotator(10.f, 30.f, 0.f), FVector(28.f, 22.f, 19.f), FLinearColor(0.16f, 0.13f, 0.11f));
	MarsAssets::AddShape(this, Root, TEXT("Deimos"), EMarsShape::Sphere, DeimosDir * 220000.f,
		FRotator::ZeroRotator, FVector(9.f, 8.f, 7.f), FLinearColor(0.20f, 0.17f, 0.14f));
}
