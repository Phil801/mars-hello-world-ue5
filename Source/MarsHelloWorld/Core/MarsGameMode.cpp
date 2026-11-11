// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsGameMode.h"
#include "Core/MarsPlayerController.h"
#include "Core/MarsShowSettings.h"
#include "Core/MarsShowSubsystem.h"
#include "UI/MarsHUD.h"
#include "World/MarsTerrain.h"
#include "World/MarsEnvironment.h"
#include "Camera/MarsShowCamera.h"
#include "Combat/CombatantCharacter.h"
#include "Combat/CombatantAstronaut.h"
#include "MarsHelloWorld.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

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
	Show->SetTerrain(Terrain);

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
	// PostLogin can run before StartPlay / BuildWorld. Wait for the terrain in that case.
	if (Terrain)
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

	const FVector Ground = ArenaCenter + FVector(0.f, 0.f, 120.f);
	const FRotator Facing(0.f, 90.f, 0.f); // look toward the first entrance
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

void AMarsGameMode::BuildWorld()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Terrain = World->SpawnActor<AMarsTerrain>(AMarsTerrain::StaticClass(), FTransform::Identity, Params);
	if (Terrain)
	{
		Terrain->BuildTerrain(Seed);
		ArenaCenter = FVector(0.f, 0.f, Terrain->GetHeightAt(0.f, 0.f));
	}

	Environment = World->SpawnActor<AMarsEnvironment>(AMarsEnvironment::StaticClass(), FTransform::Identity, Params);

	const FVector CamStart = ArenaCenter + FVector(-2600.f, -1400.f, 900.f);
	ShowCamera = World->SpawnActor<AMarsShowCamera>(AMarsShowCamera::StaticClass(), FTransform((ArenaCenter - CamStart).Rotation(), CamStart), Params);
	if (ShowCamera)
	{
		ShowCamera->SetArenaCenter(ArenaCenter);
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

	// Arrival points on a ring, 120 degrees apart, facing the centre.
	const int32 Count = FMath::Max(1, Settings->CombatantClasses.Num());
	const float ArriveAngle = FMath::DegreesToRadians(90.f + 360.f * Index / Count);
	const FVector2D ArriveDir(FMath::Cos(ArriveAngle), FMath::Sin(ArriveAngle));
	const float Ring = Settings->ArrivalRingRadius;
	const FVector2D Arrive2D = FVector2D(ArenaCenter.X, ArenaCenter.Y) + ArriveDir * Ring;
	const float ArriveZ = Terrain ? Terrain->GetHeightAt(Arrive2D.X, Arrive2D.Y) : ArenaCenter.Z;
	const FVector ArrivalPoint(Arrive2D.X, Arrive2D.Y, ArriveZ);

	// Where each kind enters from: runners from the far plain, fliers out of the sky.
	FVector Start;
	switch (CDO->GetProfile().EntryStyle)
	{
	case EEntryStyle::Run:
	{
		const FVector2D S = Arrive2D + ArriveDir * 6500.f;
		const float Z = Terrain ? Terrain->GetHeightAt(S.X, S.Y) : ArriveZ;
		Start = FVector(S.X, S.Y, Z + 130.f);
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
		const float Z = Terrain ? Terrain->GetHeightAt(S.X, S.Y) : ArriveZ;
		Start = FVector(S.X, S.Y, FMath::Max(Z, ArriveZ) + 1100.f);
		break;
	}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FRotator Facing = (ArrivalPoint - Start).GetSafeNormal2D().Rotation();
	ACombatantCharacter* C = GetWorld()->SpawnActor<ACombatantCharacter>(Class, Start, Facing, Params);
	if (C)
	{
		C->LaunchEntrance(ArrivalPoint, ArenaCenter, Seed * 31 + Index * 7919);
		MarsShowLog(FString::Printf(TEXT("%s enters from %s"), *C->GetProfile().DisplayName.ToString(), *Start.ToCompactString()));
	}
}
