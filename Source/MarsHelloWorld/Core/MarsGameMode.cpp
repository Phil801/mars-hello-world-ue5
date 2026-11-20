// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsGameMode.h"
#include "Core/MarsPlayerController.h"
#include "Core/MarsShowSettings.h"
#include "Core/MarsShowSubsystem.h"
#include "UI/MarsHUD.h"
#include "World/MarsTerrain.h"
#include "World/MarsEnvironment.h"
#include "World/MarsSpawnMarker.h"
#include "Camera/MarsShowCamera.h"
#include "Combat/CombatantCharacter.h"
#include "Combat/CombatantAstronaut.h"
#include "MarsHelloWorld.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "LandscapeProxy.h"
#include "Engine/DirectionalLight.h"
#include "Components/SkyAtmosphereComponent.h"

AMarsGameMode::AMarsGameMode()
{
	DefaultPawnClass = ACombatantAstronaut::StaticClass();
	PlayerControllerClass = AMarsPlayerController::StaticClass();
	HUDClass = AMarsHUD::StaticClass();
}

void AMarsGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	Seed = Settings->RandomSeed;

	// Mars gravity for this world (DefaultEngine.ini also sets the project default to -371).
	if (AWorldSettings* WS = GetWorldSettings())
	{
		WS->bGlobalGravitySet = true;
		WS->GlobalGravityZ = Settings->MarsGravityZ;
	}
}

void AMarsGameMode::StartPlay()
{
	BuildWorld();
	Super::StartPlay();

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			PlaceAstronaut(It->Get());
		}
	}

	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	UMarsShowSubsystem* Show = GetWorld()->GetSubsystem<UMarsShowSubsystem>();
	if (!Show)
	{
		UE_LOG(LogMars, Error, TEXT("MarsShowSubsystem missing; is this a game world?"));
		return;
	}

	int32 Valid = 0;
	for (int32 i = 0; i < Settings->CombatantClasses.Num(); ++i)
	{
		if (!Settings->CombatantClasses[i].IsNull())
		{
			++Valid;
			const float Delay = Settings->EntranceDelays.IsValidIndex(i) ? Settings->EntranceDelays[i] : 1.f + 1.2f * i;
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &AMarsGameMode::LaunchCombatant, i), FMath::Max(0.01f, Delay), false);
		}
	}
	Show->BeginShow(Valid, Seed);
	MarsShowLog(FString::Printf(TEXT("Mars Hello World: %d combatants, seed %d, gravity %.0f"), Valid, Seed, GetWorld()->GetGravityZ()));
}

void AMarsGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// PostLogin can run before StartPlay / BuildWorld. Wait for the world in that case.
	if (bWorldBuilt)
	{
		PlaceAstronaut(NewPlayer);
	}
}

void AMarsGameMode::PlaceAstronaut(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		return;
	}

	FVector Ground = ArenaCenter + FVector(0.f, 0.f, 120.f);
	FRotator Facing(0.f, 90.f, 0.f); // look toward the first entrance
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			Ground = It->GetActorLocation();
			Facing = FRotator(0.f, It->GetActorRotation().Yaw, 0.f);
			break;
		}
	}
	if (ACombatantAstronaut* Existing = Cast<ACombatantAstronaut>(NewPlayer->GetPawn()))
	{
		Existing->SetActorLocationAndRotation(Ground, Facing, false, nullptr, ETeleportType::TeleportPhysics);
		Existing->PrepareAsPlayer(ArenaCenter, Seed);
	}
	else
	{
		RestartPlayerAtTransform(NewPlayer, FTransform(Facing, Ground));
		if (ACombatantAstronaut* Astro = Cast<ACombatantAstronaut>(NewPlayer->GetPawn()))
		{
			Astro->PrepareAsPlayer(ArenaCenter, Seed);
		}
	}

	if (AMarsPlayerController* PC = Cast<AMarsPlayerController>(NewPlayer))
	{
		PC->SetShowCamera(ShowCamera);
		PC->UseFirstPersonView();
	}
}

float AMarsGameMode::GroundZ(float X, float Y, float Fallback) const
{
	const UMarsShowSubsystem* Show = GetWorld() ? GetWorld()->GetSubsystem<UMarsShowSubsystem>() : nullptr;
	return Show ? Show->GetGroundZ(X, Y, Fallback) : Fallback;
}

void AMarsGameMode::BuildWorld()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UMarsShowSubsystem* Show = World->GetSubsystem<UMarsShowSubsystem>();

	// Ground: a level Landscape (MarsArena), else a placed AMarsTerrain, else a runtime AMarsTerrain.
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		Landscape = *It;
		break;
	}
	for (TActorIterator<AMarsTerrain> It(World); It; ++It)
	{
		Terrain = *It;
		break;
	}
	if (!Landscape && !Terrain)
	{
		Terrain = World->SpawnActor<AMarsTerrain>(AMarsTerrain::StaticClass(), FTransform::Identity, Params);
	}
	if (Terrain)
	{
		Terrain->BuildTerrain(Seed);
	}
	if (Show)
	{
		Show->SetTerrain(Terrain);
		Show->SetLandscape(Landscape);
	}
	ArenaCenter = FVector(0.f, 0.f, GroundZ(0.f, 0.f, 0.f));

	// Sky/lighting: MarsArena places standalone SkyAtmosphere / fog / sun / sky light actors. Only spawn the
	// all-in-one AMarsEnvironment when the map has neither those nor a placed AMarsEnvironment.
	for (TActorIterator<AMarsEnvironment> It(World); It; ++It)
	{
		Environment = *It;
		break;
	}
	const bool bLevelLighting = TActorIterator<ADirectionalLight>(World) || TActorIterator<ASkyAtmosphere>(World);
	const bool bLevelEnvironment = Environment != nullptr || bLevelLighting;
	if (!bLevelEnvironment)
	{
		Environment = World->SpawnActor<AMarsEnvironment>(AMarsEnvironment::StaticClass(), FTransform::Identity, Params);
	}

	for (TActorIterator<AMarsShowCamera> It(World); It; ++It)
	{
		ShowCamera = *It;
		break;
	}
	const bool bLevelCamera = ShowCamera != nullptr;
	if (!ShowCamera)
	{
		const FVector CamStart = ArenaCenter + FVector(-2600.f, -1400.f, 900.f);
		ShowCamera = World->SpawnActor<AMarsShowCamera>(AMarsShowCamera::StaticClass(), FTransform((ArenaCenter - CamStart).Rotation(), CamStart), Params);
	}
	if (ShowCamera)
	{
		ShowCamera->SetArenaCenter(ArenaCenter);
	}

	SpawnMarkers.Reset();
	for (TActorIterator<AMarsSpawnMarker> It(World); It; ++It)
	{
		if (!SpawnMarkers.Contains(It->CombatantIndex))
		{
			SpawnMarkers.Add(It->CombatantIndex, *It);
		}
	}

	bWorldBuilt = true;
	MarsShowLog(FString::Printf(TEXT("Mars world: %s ground, %s environment, %s camera, %d spawn markers"),
		Landscape ? TEXT("level landscape") : TEXT("procedural"),
		bLevelEnvironment ? TEXT("level") : TEXT("runtime"),
		bLevelCamera ? TEXT("level") : TEXT("runtime"), SpawnMarkers.Num()));
}

FVector AMarsGameMode::ComputeEntryStart(EEntryStyle Style, const FVector& ArrivalPoint, const FVector2D& ArriveDir, float ArriveAngle) const
{
	// Where each kind enters from: runners from the far plain, fliers out of the sky.
	const FVector2D Arrive2D(ArrivalPoint.X, ArrivalPoint.Y);
	const float ArriveZ = ArrivalPoint.Z;
	switch (Style)
	{
	case EEntryStyle::Run:
	{
		const FVector2D S = Arrive2D + ArriveDir * 6500.f;
		return FVector(S.X, S.Y, GroundZ(S.X, S.Y, ArriveZ) + 130.f);
	}
	case EEntryStyle::Fly:
	{
		const FVector2D Side(FMath::Cos(ArriveAngle + 0.7f), FMath::Sin(ArriveAngle + 0.7f));
		const FVector2D S = Arrive2D + Side * 5200.f;
		return FVector(S.X, S.Y, ArriveZ + 3800.f);
	}
	case EEntryStyle::Hover:
	default:
	{
		const FVector2D S = Arrive2D + ArriveDir * 5600.f;
		return FVector(S.X, S.Y, FMath::Max(GroundZ(S.X, S.Y, ArriveZ), ArriveZ) + 1100.f);
	}
	}
}

void AMarsGameMode::LaunchCombatant(int32 Index)
{
	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	if (!Settings->CombatantClasses.IsValidIndex(Index))
	{
		return;
	}
	UClass* Class = Settings->CombatantClasses[Index].LoadSynchronous();
	if (!Class)
	{
		UE_LOG(LogMars, Error, TEXT("Combatant class %d failed to load"), Index);
		return;
	}
	const ACombatantCharacter* CDO = Class->GetDefaultObject<ACombatantCharacter>();
	if (!CDO)
	{
		return;
	}

	FVector ArrivalPoint;
	FVector Start;
	const AMarsSpawnMarker* Marker = SpawnMarkers.FindRef(Index);
	if (Marker)
	{
		// Level marker: arrival = marker location (snapped to the ground), start = its EntryStart widget.
		const FVector M = Marker->GetArrivalLocation();
		ArrivalPoint = FVector(M.X, M.Y, GroundZ(M.X, M.Y, M.Z));
		if (Marker->bUseEntryStart)
		{
			Start = Marker->GetEntryStartLocation();
		}
		else
		{
			const FVector2D Dir = FVector2D(M.X - ArenaCenter.X, M.Y - ArenaCenter.Y).GetSafeNormal();
			Start = ComputeEntryStart(CDO->GetProfile().EntryStyle, ArrivalPoint, Dir, FMath::Atan2(Dir.Y, Dir.X));
		}
	}
	else
	{
		// No marker: arrival points on a ring, 120 degrees apart, facing the centre.
		const int32 Count = FMath::Max(1, Settings->CombatantClasses.Num());
		const float ArriveAngle = FMath::DegreesToRadians(90.f + 360.f * Index / Count);
		const FVector2D ArriveDir(FMath::Cos(ArriveAngle), FMath::Sin(ArriveAngle));
		const FVector2D Arrive2D = FVector2D(ArenaCenter.X, ArenaCenter.Y) + ArriveDir * Settings->ArrivalRingRadius;
		ArrivalPoint = FVector(Arrive2D.X, Arrive2D.Y, GroundZ(Arrive2D.X, Arrive2D.Y, ArenaCenter.Z));
		Start = ComputeEntryStart(CDO->GetProfile().EntryStyle, ArrivalPoint, ArriveDir, ArriveAngle);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FRotator Facing = (ArrivalPoint - Start).GetSafeNormal2D().Rotation();
	ACombatantCharacter* C = GetWorld()->SpawnActor<ACombatantCharacter>(Class, Start, Facing, Params);
	if (C)
	{
		C->LaunchEntrance(ArrivalPoint, ArenaCenter, Seed * 31 + Index * 7919);
		MarsShowLog(FString::Printf(TEXT("%s enters from %s (%s)"), *C->GetProfile().DisplayName.ToString(), *Start.ToCompactString(),
			Marker ? *FString::Printf(TEXT("marker %s"), *Marker->GetActorNameOrLabel()) : TEXT("ring fallback")));
	}
}