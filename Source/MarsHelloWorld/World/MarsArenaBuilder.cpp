// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsArenaBuilder.h"
#include "World/MarsTerrain.h"
#include "World/MarsEnvironment.h"
#include "World/MarsRockField.h"
#include "World/MarsSpawnMarker.h"
#include "World/MarsAssets.h"
#include "Camera/MarsShowCamera.h"
#include "Audio/MarsMusicActor.h"
#include "Combat/CombatantCharacter.h"
#include "Core/MarsShowSettings.h"
#include "MarsHelloWorld.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"

#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"

const FName UMarsArenaBuilder::GeneratedTag(TEXT("MarsArenaGenerated"));

#if WITH_EDITOR
namespace
{
	// 8 x 8 components of 63 quads = 505 x 505 vertices over the 600 m terrain (~1.19 m per quad).
	constexpr int32 QuadsPerComponent = 63;
	constexpr int32 ComponentsPerSide = 8;
	constexpr int32 VertsPerSide = QuadsPerComponent * ComponentsPerSide + 1;
	constexpr float LandscapeScaleZ = 100.f; // +-256 m of height range at ~0.8 cm precision

	template <typename T>
	T* SpawnTagged(UWorld* World, const FTransform& Xform, const TCHAR* Label, const TCHAR* Folder)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		T* Actor = World->SpawnActor<T>(T::StaticClass(), Xform, Params);
		if (Actor)
		{
			Actor->Tags.AddUnique(UMarsArenaBuilder::GeneratedTag);
			Actor->SetActorLabel(Label);
			Actor->SetFolderPath(FName(Folder));
		}
		return Actor;
	}

	/** Saved (not dynamic) colour instance of M_MarsSurface for a level-placed moon. Created or updated in place. */
	UMaterialInterface* MakeMoonMaterial(const TCHAR* AssetName, const FLinearColor& Color)
	{
		UMaterialInterface* Parent = MarsAssets::GetSurfaceMaterial();
		if (!Parent)
		{
			return nullptr;
		}
		const FString PackageName = FString::Printf(TEXT("/Game/Materials/%s"), AssetName);
		UMaterialInstanceConstant* MIC = LoadObject<UMaterialInstanceConstant>(nullptr, *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_Quiet | LOAD_NoWarn);
		bool bCreated = false;
		if (!MIC)
		{
			UPackage* Package = CreatePackage(*PackageName);
			MIC = NewObject<UMaterialInstanceConstant>(Package, AssetName, RF_Public | RF_Standalone | RF_Transactional);
			bCreated = true;
		}
		MIC->SetParentEditorOnly(Parent);
		MIC->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Color")), Color);
		MIC->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Glow")), 0.f);
		MIC->PostEditChange();
		MIC->MarkPackageDirty();
		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(MIC);
		}
		return MIC;
	}

	AStaticMeshActor* SpawnMoon(UWorld* World, const TCHAR* Label, const TCHAR* MatName, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color)
	{
		AStaticMeshActor* Moon = SpawnTagged<AStaticMeshActor>(World, FTransform(Rotation, Location, Scale), Label, TEXT("Mars/Environment"));
		if (!Moon)
		{
			return nullptr;
		}
		// Same as MarsAssets::AddShape: movable, no collision/overlaps/navigation, coloured M_MarsSurface.
		UStaticMeshComponent* Comp = Moon->GetStaticMeshComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(MarsAssets::GetShape(EMarsShape::Sphere));
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCanEverAffectNavigation(false);
		if (UMaterialInterface* Mat = MakeMoonMaterial(MatName, Color))
		{
			Comp->SetMaterial(0, Mat);
		}
		return Moon;
	}

	/** Standalone copies of every AMarsEnvironment component, with its constructor AND BeginPlay settings. */
	int32 SpawnEnvironmentActors(UWorld* World)
	{
		const AMarsEnvironment* Env = GetDefault<AMarsEnvironment>();
		const float SunElevation = Env->SunElevation; // 24
		const float SunAzimuth = Env->SunAzimuth;     // 135
		int32 Count = 0;
		ASkyAtmosphere* SkyActor = nullptr;
		AExponentialHeightFog* FogActor = nullptr;
		ADirectionalLight* SunActor = nullptr;
		ASkyLight* SkyLightActor = nullptr;

		// --- Sky atmosphere (units: km and 1/km) ---
		if ((SkyActor = SpawnTagged<ASkyAtmosphere>(World, FTransform::Identity, TEXT("Mars_SkyAtmosphere"), TEXT("Mars/Environment"))) != nullptr)
		{
			USkyAtmosphereComponent* Sky = SkyActor->GetComponent();
			Sky->BottomRadius = 3389.5f;
			Sky->AtmosphereHeight = 80.f;
			Sky->GroundAlbedo = FColor(178, 98, 58);
			Sky->RayleighScatteringScale = 0.0035f;
			Sky->RayleighScattering = FLinearColor(0.175287f, 0.409607f, 1.f);
			Sky->RayleighExponentialDistribution = 11.1f;
			Sky->MieScatteringScale = 0.014f;
			Sky->MieScattering = FLinearColor(1.f, 0.70f, 0.48f);
			Sky->MieAbsorptionScale = 0.010f;
			Sky->MieAbsorption = FLinearColor(0.10f, 0.34f, 0.80f);
			Sky->MieAnisotropy = 0.76f;
			Sky->MieExponentialDistribution = 10.f;
			Sky->OtherAbsorptionScale = 0.f;
			Sky->MultiScatteringFactor = 1.f;
			Sky->MarkRenderStateDirty();
			++Count;
		}

		// --- Dust haze (AMarsEnvironment sets these at BeginPlay) ---
		if ((FogActor = SpawnTagged<AExponentialHeightFog>(World, FTransform::Identity, TEXT("Mars_HeightFog"), TEXT("Mars/Environment"))) != nullptr)
		{
			UExponentialHeightFogComponent* Fog = FogActor->GetComponent();
			Fog->SetFogDensity(0.03f);
			Fog->SetFogHeightFalloff(0.12f);
			Fog->SetFogInscatteringColor(FLinearColor(0.52f, 0.25f, 0.11f));
			Fog->SetFogMaxOpacity(0.92f);
			Fog->SetStartDistance(1500.f);
			Fog->SetVolumetricFog(true);
			Fog->SetVolumetricFogAlbedo(FColor(230, 160, 120));
			++Count;
		}

		// --- Sun (constructor settings + BeginPlay rotation and atmosphere sun flag) ---
		if ((SunActor = SpawnTagged<ADirectionalLight>(World, FTransform(FRotator(-SunElevation, SunAzimuth, 0.f), FVector::ZeroVector), TEXT("Mars_Sun"), TEXT("Mars/Environment"))) != nullptr)
		{
			// SpawnActor composes with ADirectionalLight's default -46 pitch on the root, so set the final rotation explicitly.
			SunActor->SetActorRotation(FRotator(-SunElevation, SunAzimuth, 0.f));
			UDirectionalLightComponent* Sun = CastChecked<UDirectionalLightComponent>(SunActor->GetLightComponent());
			Sun->SetMobility(EComponentMobility::Movable); // ADirectionalLight defaults to Stationary
			Sun->Intensity = 4.6f;
			Sun->LightColor = FColor(255, 243, 226);
			Sun->CastShadows = true;
			Sun->SetAtmosphereSunLight(true);
			Sun->MarkRenderStateDirty();
			++Count;
		}

		// --- Sky light (real-time captured sky, so no manual recapture is needed) ---
		if ((SkyLightActor = SpawnTagged<ASkyLight>(World, FTransform::Identity, TEXT("Mars_SkyLight"), TEXT("Mars/Environment"))) != nullptr)
		{
			USkyLightComponent* SL = SkyLightActor->GetLightComponent();
			SL->SetMobility(EComponentMobility::Movable);
			SL->bRealTimeCapture = true;
			SL->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			SL->bLowerHemisphereIsBlack = false;
			SL->LowerHemisphereColor = FLinearColor(0.22f, 0.09f, 0.04f);
			SL->Intensity = 1.f;
			SL->MarkRenderStateDirty();
			++Count;
		}

		// --- Phobos and Deimos (AMarsEnvironment::AddMoons at BeginPlay) ---
		const FVector PhobosDir = FRotator(32.f, SunAzimuth + 150.f, 0.f).Vector();
		const FVector DeimosDir = FRotator(48.f, SunAzimuth + 205.f, 0.f).Vector();
		AStaticMeshActor* Phobos = SpawnMoon(World, TEXT("Mars_Phobos"), TEXT("MI_Phobos"), PhobosDir * 180000.f, FRotator(10.f, 30.f, 0.f), FVector(28.f, 22.f, 19.f), FLinearColor(0.16f, 0.13f, 0.11f));
		AStaticMeshActor* Deimos = SpawnMoon(World, TEXT("Mars_Deimos"), TEXT("MI_Deimos"), DeimosDir * 220000.f, FRotator::ZeroRotator, FVector(9.f, 8.f, 7.f), FLinearColor(0.20f, 0.17f, 0.14f));
		Count += (Phobos ? 1 : 0) + (Deimos ? 1 : 0);

		// Outliner hierarchy, mirroring Kevin's hand-made setup in MarsArena (world transforms kept):
		//   Mars_SkyAtmosphere > Mars_HeightFog, Mars_Sun, Mars_SkyLight;  Mars_Deimos > Mars_Phobos
		const FAttachmentTransformRules KeepWorld = FAttachmentTransformRules::KeepWorldTransform;
		if (SkyActor)
		{
			for (AActor* Child : TArray<AActor*>{ FogActor, SunActor, SkyLightActor })
			{
				if (Child)
				{
					Child->AttachToActor(SkyActor, KeepWorld);
				}
			}
		}
		if (Phobos && Deimos)
		{
			Phobos->AttachToActor(Deimos, KeepWorld);
		}
		return Count;
	}
}
#endif

FString UMarsArenaBuilder::BuildMarsEnvironment(UWorld* World)
{
#if WITH_EDITOR
	if (!World || World->IsGameWorld())
	{
		return TEXT("ERROR: pass the editor world (not a PIE/game world)");
	}
	TArray<AActor*> Old;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const bool bGeneratedEnv = It->ActorHasTag(GeneratedTag) && It->GetFolderPath() == FName(TEXT("Mars/Environment"));
		if (It->IsA<AMarsEnvironment>() || bGeneratedEnv)
		{
			Old.Add(*It);
		}
	}
	for (AActor* A : Old)
	{
		World->EditorDestroyActor(A, true);
	}
	const int32 Count = SpawnEnvironmentActors(World);
	World->MarkPackageDirty();
	const FString Summary = FString::Printf(TEXT("Mars environment rebuilt: removed %d old actor(s), placed %d standalone actors"), Old.Num(), Count);
	UE_LOG(LogMars, Display, TEXT("%s"), *Summary);
	return Summary;
#else
	return TEXT("ERROR: editor only");
#endif
}

FString UMarsArenaBuilder::BuildMarsArena(UWorld* World, bool bReplaceExisting)
{
#if WITH_EDITOR
	if (!World || World->IsGameWorld())
	{
		return TEXT("ERROR: pass the editor world (not a PIE/game world)");
	}

	if (bReplaceExisting)
	{
		TArray<AActor*> Old;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(GeneratedTag))
			{
				Old.Add(*It);
			}
		}
		for (AActor* A : Old)
		{
			World->EditorDestroyActor(A, true);
		}
	}

	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	const int32 Seed = Settings->RandomSeed;

	// Temporary procedural terrain: only its height function and rock layout are used.
	FActorSpawnParameters TmpParams;
	TmpParams.ObjectFlags |= RF_Transient;
	TmpParams.bHideFromSceneOutliner = true;
	AMarsTerrain* Height = World->SpawnActor<AMarsTerrain>(AMarsTerrain::StaticClass(), FTransform::Identity, TmpParams);
	if (!Height)
	{
		return TEXT("ERROR: could not spawn the temporary AMarsTerrain");
	}
	FRandomStream Stream(Seed);
	Height->InitHeightField(Stream);
	TArray<FTransform> BoulderXforms, SlabXforms;
	Height->ComputeRockTransforms(Stream, BoulderXforms, SlabXforms);

	const float HalfExtent = Height->GetHalfExtent();
	const float QuadSize = (2.f * HalfExtent) / float(VertsPerSide - 1);

	// --- Landscape ----------------------------------------------------------------------------
	TArray<uint16> HeightData;
	HeightData.SetNumUninitialized(VertsPerSide * VertsPerSide);
	float MinH = TNumericLimits<float>::Max(), MaxH = TNumericLimits<float>::Lowest();
	for (int32 Y = 0; Y < VertsPerSide; ++Y)
	{
		for (int32 X = 0; X < VertsPerSide; ++X)
		{
			const float WX = -HalfExtent + X * QuadSize;
			const float WY = -HalfExtent + Y * QuadSize;
			const float H = Height->GetHeightAt(WX, WY);
			MinH = FMath::Min(MinH, H);
			MaxH = FMath::Max(MaxH, H);
			const float Encoded = 32768.f + H * 128.f / LandscapeScaleZ;
			HeightData[Y * VertsPerSide + X] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Encoded), 0, 65535));
		}
	}

	FActorSpawnParameters LParams;
	LParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALandscape* Landscape = World->SpawnActor<ALandscape>(ALandscape::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(-HalfExtent, -HalfExtent, 0.f)), LParams);
	if (!Landscape)
	{
		World->EditorDestroyActor(Height, false);
		return TEXT("ERROR: could not spawn ALandscape");
	}
	Landscape->SetActorRelativeScale3D(FVector(QuadSize, QuadSize, LandscapeScaleZ));
	if (UMaterialInterface* GroundMat = Height->GetGroundMaterialPath().LoadSynchronous())
	{
		Landscape->LandscapeMaterial = GroundMat;
	}
	else
	{
		Landscape->LandscapeMaterial = MarsAssets::GetGroundMaterial();
	}

	TMap<FGuid, TArray<uint16>> HeightPerLayer;
	HeightPerLayer.Add(FGuid(), MoveTemp(HeightData));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialPerLayer;
	MaterialPerLayer.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());
	Landscape->Import(FGuid::NewGuid(), 0, 0, VertsPerSide - 1, VertsPerSide - 1, 1, QuadsPerComponent,
		HeightPerLayer, TEXT(""), MaterialPerLayer, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
	if (ULandscapeInfo* Info = Landscape->GetLandscapeInfo())
	{
		Info->UpdateLayerInfoMap(Landscape);
	}
	Landscape->Tags.AddUnique(GeneratedTag);
	Landscape->SetActorLabel(TEXT("MarsLandscape"));
	Landscape->SetFolderPath(TEXT("Mars/Terrain"));

	// --- Rocks --------------------------------------------------------------------------------
	int32 RockCount = 0;
	if (AMarsRockField* Rocks = SpawnTagged<AMarsRockField>(World, FTransform::Identity, TEXT("MarsRockField"), TEXT("Mars/Terrain")))
	{
		UMaterialInterface* RockMat = Height->GetRockMaterialPath().LoadSynchronous();
		Rocks->GetBoulders()->SetStaticMesh(MarsAssets::GetShape(EMarsShape::Sphere));
		Rocks->GetSlabs()->SetStaticMesh(MarsAssets::GetShape(EMarsShape::Cube));
		if (RockMat)
		{
			Rocks->GetBoulders()->SetMaterial(0, RockMat);
			Rocks->GetSlabs()->SetMaterial(0, RockMat);
		}
		Rocks->GetBoulders()->AddInstances(BoulderXforms, false, true);
		Rocks->GetSlabs()->AddInstances(SlabXforms, false, true);
		RockCount = BoulderXforms.Num() + SlabXforms.Num();
	}

	// --- Lighting, camera, music, player start ------------------------------------------------
	const FVector ArenaCenter(0.f, 0.f, Height->GetHeightAt(0.f, 0.f));
	TArray<AMarsEnvironment*> OldEnvironments;
	for (TActorIterator<AMarsEnvironment> It(World); It; ++It)
	{
		OldEnvironments.Add(*It);
	}
	for (AMarsEnvironment* OldEnv : OldEnvironments)
	{
		World->EditorDestroyActor(OldEnv, true); // the sky is placed as standalone actors now
	}
	SpawnEnvironmentActors(World);

	const FVector CamStart = ArenaCenter + FVector(-2600.f, -1400.f, 900.f);
	SpawnTagged<AMarsShowCamera>(World, FTransform((ArenaCenter - CamStart).Rotation(), CamStart), TEXT("MarsShowCamera"), TEXT("Mars/Show"));
	SpawnTagged<AMarsMusicActor>(World, FTransform::Identity, TEXT("MarsMusic"), TEXT("Mars/Show"));
	SpawnTagged<APlayerStart>(World, FTransform(FRotator(0.f, 90.f, 0.f), ArenaCenter + FVector(0.f, 0.f, 120.f)), TEXT("MarsPlayerStart"), TEXT("Mars/Show"));

	// --- Spawn markers (same ring math AMarsGameMode used to compute at runtime) --------------
	const int32 Count = FMath::Max(1, Settings->CombatantClasses.Num());
	int32 Markers = 0;
	for (int32 Index = 0; Index < Settings->CombatantClasses.Num(); ++Index)
	{
		UClass* Class = Settings->CombatantClasses[Index].LoadSynchronous();
		const ACombatantCharacter* CDO = Class ? Class->GetDefaultObject<ACombatantCharacter>() : nullptr;
		const EEntryStyle Style = CDO ? CDO->GetProfile().EntryStyle : EEntryStyle::Run;

		const float ArriveAngle = FMath::DegreesToRadians(90.f + 360.f * Index / Count);
		const FVector2D ArriveDir(FMath::Cos(ArriveAngle), FMath::Sin(ArriveAngle));
		const FVector2D Arrive2D = FVector2D(ArenaCenter.X, ArenaCenter.Y) + ArriveDir * Settings->ArrivalRingRadius;
		const float ArriveZ = Height->GetHeightAt(Arrive2D.X, Arrive2D.Y);
		const FVector Arrival(Arrive2D.X, Arrive2D.Y, ArriveZ);

		FVector Start;
		switch (Style)
		{
		case EEntryStyle::Run:
		{
			const FVector2D S = Arrive2D + ArriveDir * 6500.f;
			Start = FVector(S.X, S.Y, Height->GetHeightAt(S.X, S.Y) + 130.f);
			break;
		}
		case EEntryStyle::Fly:
		{
			const FVector2D Side(FMath::Cos(ArriveAngle + 0.7f), FMath::Sin(ArriveAngle + 0.7f));
			const FVector2D S = Arrive2D + Side * 5200.f;
			Start = FVector(S.X, S.Y, ArriveZ + 3800.f);
			break;
		}
		case EEntryStyle::Hover:
		default:
		{
			const FVector2D S = Arrive2D + ArriveDir * 5600.f;
			Start = FVector(S.X, S.Y, FMath::Max(Height->GetHeightAt(S.X, S.Y), ArriveZ) + 1100.f);
			break;
		}
		}

		// Arrow points from the entry start toward the arrival point.
		const FRotator Facing = (Arrival - Start).GetSafeNormal2D().Rotation();
		const FString Name = CDO ? CDO->GetProfile().DisplayName.ToString() : FString::Printf(TEXT("%d"), Index);
		const FString Label = FString::Printf(TEXT("SpawnMarker_%d_%s"), Index, *Name.Replace(TEXT(" "), TEXT("")));
		if (AMarsSpawnMarker* Marker = SpawnTagged<AMarsSpawnMarker>(World, FTransform(Facing, Arrival), *Label, TEXT("Mars/SpawnMarkers")))
		{
			Marker->CombatantIndex = Index;
			Marker->bUseEntryStart = true;
			Marker->EntryStart = Marker->GetActorTransform().InverseTransformPosition(Start);
			++Markers;
		}
	}

	World->EditorDestroyActor(Height, false);
	World->MarkPackageDirty();

	const FString Summary = FString::Printf(TEXT("Mars arena built: landscape %dx%d verts (%.1f cm/quad, height %.0f..%.0f cm), %d rocks, %d spawn markers, seed %d"),
		VertsPerSide, VertsPerSide, QuadSize, MinH, MaxH, RockCount, Markers, Seed);
	UE_LOG(LogMars, Display, TEXT("%s"), *Summary);
	return Summary;
#else
	return TEXT("ERROR: editor only");
#endif
}