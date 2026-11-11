// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsShowSubsystem.h"
#include "Core/MarsShowSettings.h"
#include "Core/MarsPlayerController.h"
#include "Audio/MarsMusicActor.h"
#include "Dialogue/MarsConversationDirector.h"
#include "Combat/CombatantCharacter.h"
#include "Combat/MarsHealthComponent.h"
#include "World/MarsTerrain.h"
#include "MarsHelloWorld.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "MarsShow"

bool UMarsShowSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UMarsShowSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
		if (Director)
		{
			World->GetTimerManager().ClearAllTimersForObject(Director);
			Director->Stop();
		}
	}
	Director = nullptr;
	Music = nullptr;
	Super::Deinitialize();
}

float UMarsShowSubsystem::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

void UMarsShowSubsystem::BeginShow(int32 InExpectedCombatants, int32 InSeed)
{
	ExpectedCombatants = FMath::Max(1, InExpectedCombatants);
	Seed = InSeed;
	SetPhase(EMarsShowPhase::Arrivals);
	ShowBanner(LOCTEXT("Welcome", "MARS  -  JEZERO CRATER"), FLinearColor(1.f, 0.62f, 0.35f), 3.0f, 3.f);

	Director = NewObject<UMarsConversationDirector>(this);
	Director->BeginShow(ExpectedCombatants, Seed);
	if (ACombatantCharacter* Player = GetPlayerAstronaut())
	{
		Director->SetPlayer(Player);
	}

	// If anyone is still not in place after 25 s (stuck on a rock, fell through the world...), pull them in.
	GetWorld()->GetTimerManager().SetTimer(StragglerTimer, this, &UMarsShowSubsystem::ForceStragglers, 25.f, false);

	if (UWorld* World = GetWorld())
	{
		if (!IsValid(Music))
		{
			FActorSpawnParameters Spawn;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Spawn.Name = TEXT("MarsMusic");
			Music = World->SpawnActor<AMarsMusicActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
		}
		if (Music)
		{
			Music->StartAmbient();
		}
	}
}

void UMarsShowSubsystem::RegisterCombatant(ACombatantCharacter* Combatant)
{
	if (Combatant && !Combatants.Contains(Combatant))
	{
		Combatants.Add(Combatant);
	}
}

void UMarsShowSubsystem::NotifyArrived(ACombatantCharacter* Combatant)
{
	if (Director)
	{
		Director->NotifyArrived(Combatant);
	}
}

void UMarsShowSubsystem::SetCurrentSpeaker(ACombatantCharacter* Combatant)
{
	CurrentSpeaker = Combatant;
}

void UMarsShowSubsystem::MarkFightClockStarted()
{
	if (FightStartTime < 0.f)
	{
		FightStartTime = Now();
	}
}

void UMarsShowSubsystem::StartFight()
{
	MarkFightClockStarted();
	SetPhase(EMarsShowPhase::Fight);
	AddCameraShake(0.6f);
	for (ACombatantCharacter* C : GetCombatants(true))
	{
		if (C->GetCombatState() != ECombatantState::Fighting)
		{
			C->StartFighting();
		}
	}
}

void UMarsShowSubsystem::ForceStragglers()
{
	for (ACombatantCharacter* C : GetCombatants(true))
	{
		if (C->GetCombatState() == ECombatantState::Entering)
		{
			UE_LOG(LogMars, Warning, TEXT("%s never arrived; pulling it into the arena"), *C->GetName());
			C->ForceArrive();
		}
	}
}

void UMarsShowSubsystem::NotifyDeath(ACombatantCharacter* Victim, ACombatantCharacter* Killer)
{
	if (!Victim)
	{
		return;
	}
	FMarsCaption Line;
	Line.Color = Killer ? Killer->GetProfile().UIColor : FLinearColor::White;
	Line.Speaker = Killer ? Killer->GetProfile().DisplayName : LOCTEXT("Mars", "Mars");
	Line.Line = FText::Format(LOCTEXT("KillFeed", "{0} defeated {1}"), Line.Speaker, Victim->GetProfile().DisplayName);
	Line.ExpireTime = Now() + 8.f;
	KillFeed.Add(Line);

	ShowBanner(FText::Format(LOCTEXT("Fallen", "{0} HAS FALLEN"), Victim->GetProfile().DisplayName), Victim->GetProfile().UIColor, 1.8f, 3.f);
	AddCameraShake(0.8f);
	if (Director)
	{
		Director->NotifyDeath(Victim, Killer);
	}
	OnCombatantDied.Broadcast(Victim, Killer);
	CheckForWinner();
}

void UMarsShowSubsystem::NotifyFirstShot()
{
	if (Music)
	{
		Music->NotifyFirstShot();
	}
}

void UMarsShowSubsystem::NotifyVoiceStarted(float DurationSeconds)
{
	if (Music)
	{
		Music->NotifyVoice(DurationSeconds);
	}
}

void UMarsShowSubsystem::PlayVictoryMusic()
{
	if (Music)
	{
		Music->PlayVictory();
	}
}

void UMarsShowSubsystem::CheckForWinner()
{
	if (Phase == EMarsShowPhase::Victory)
	{
		return;
	}
	const TArray<ACombatantCharacter*> LivingNpcs = GetNpcCombatants(true);
	if (LivingNpcs.Num() > 0)
	{
		return;
	}

	SetPhase(EMarsShowPhase::Victory);
	PlayVictoryMusic();
	if (ACombatantCharacter* Player = GetPlayerAstronaut())
	{
		Winner = Player;
		ShowBanner(LOCTEXT("YouWin", "YOU WIN"), Player->GetProfile().UIColor, 9999.f, 5.f);
		Player->Celebrate();
		if (AMarsPlayerController* PC = Cast<AMarsPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			PC->EnterVictoryUi();
		}
	}
	else if (const TArray<ACombatantCharacter*> Alive = GetCombatants(true); Alive.Num() == 1)
	{
		Winner = Alive[0];
		ShowBanner(FText::Format(LOCTEXT("Wins", "{0} WINS!"), Alive[0]->GetProfile().DisplayName), Alive[0]->GetProfile().UIColor, 9999.f, 6.f);
		Alive[0]->Celebrate();
	}
	else
	{
		ShowBanner(LOCTEXT("Draw", "NOBODY SURVIVED.  MARS WINS."), FLinearColor(1.f, 0.4f, 0.2f), 9999.f, 4.f);
	}

	const float AutoRestart = GetDefault<UMarsShowSettings>()->AutoRestartSeconds;
	if (AutoRestart > 0.f)
	{
		FTimerDelegate Restart = FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				PC->ConsoleCommand(TEXT("RestartLevel"));
			}
		});
		GetWorld()->GetTimerManager().SetTimer(RestartTimer, Restart, AutoRestart, false);
	}
}

void UMarsShowSubsystem::ReportDamage(ACombatantCharacter* Victim, float Amount, const FVector& Where, const FLinearColor& Color)
{
	AddFloatingText(Where, FString::Printf(TEXT("%d"), FMath::RoundToInt(Amount)), Color, FMath::Clamp(0.8f + Amount / 40.f, 0.8f, 2.2f));
	AddCameraShake(FMath::Clamp(Amount / 120.f, 0.05f, 0.35f));
}

void UMarsShowSubsystem::ShowCaption(const FText& Speaker, const FText& Line, const FLinearColor& Color, float Duration)
{
	Caption.Speaker = Speaker;
	Caption.Line = Line;
	Caption.Color = Color;
	Caption.ExpireTime = Now() + Duration;
}

void UMarsShowSubsystem::ShowBanner(const FText& Text, const FLinearColor& Color, float Duration, float Scale)
{
	Banner.Line = Text;
	Banner.Color = Color;
	Banner.ExpireTime = Now() + Duration;
	BannerScale = Scale;
}

void UMarsShowSubsystem::AddFloatingText(const FVector& Where, const FString& Text, const FLinearColor& Color, float Scale, float Lifetime)
{
	FMarsFloatingText F;
	F.WorldLocation = Where;
	F.Text = Text;
	F.Color = Color;
	F.SpawnTime = Now();
	F.Lifetime = Lifetime;
	F.Scale = Scale;
	FloatingTexts.Add(F);
}

void UMarsShowSubsystem::PruneExpired()
{
	const float T = Now();
	FloatingTexts.RemoveAll([T](const FMarsFloatingText& F) { return T - F.SpawnTime > F.Lifetime; });
	KillFeed.RemoveAll([T](const FMarsCaption& C) { return T > C.ExpireTime; });
}

TArray<ACombatantCharacter*> UMarsShowSubsystem::GetCombatants(bool bAliveOnly) const
{
	TArray<ACombatantCharacter*> Out;
	for (ACombatantCharacter* C : Combatants)
	{
		if (IsValid(C) && (!bAliveOnly || C->IsAlive()))
		{
			Out.Add(C);
		}
	}
	return Out;
}

TArray<ACombatantCharacter*> UMarsShowSubsystem::GetNpcCombatants(bool bAliveOnly) const
{
	TArray<ACombatantCharacter*> Out;
	for (ACombatantCharacter* C : GetCombatants(bAliveOnly))
	{
		if (!C->IsPlayerAstronaut())
		{
			Out.Add(C);
		}
	}
	return Out;
}

ACombatantCharacter* UMarsShowSubsystem::GetPlayerAstronaut() const
{
	for (ACombatantCharacter* C : Combatants)
	{
		if (IsValid(C) && C->IsPlayerAstronaut())
		{
			return C;
		}
	}
	return nullptr;
}

float UMarsShowSubsystem::GetFightElapsed() const
{
	return FightStartTime < 0.f ? 0.f : Now() - FightStartTime;
}

bool UMarsShowSubsystem::IsSuddenDeath() const
{
	return Phase == EMarsShowPhase::Fight && GetFightElapsed() > GetDefault<UMarsShowSettings>()->SuddenDeathAfterSeconds;
}

float UMarsShowSubsystem::GetDamageMultiplier() const
{
	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	const float Over = GetFightElapsed() - Settings->SuddenDeathAfterSeconds;
	return Over > 0.f ? 1.f + Over * Settings->SuddenDeathRampPerSecond : 1.f;
}

float UMarsShowSubsystem::GetGroundZ(float X, float Y, float Fallback) const
{
	if (const AMarsTerrain* T = Terrain.Get())
	{
		return T->GetHeightAt(X, Y);
	}
	return Fallback;
}

float UMarsShowSubsystem::ConsumeCameraTrauma(float DeltaSeconds)
{
	const float Current = CameraTrauma;
	CameraTrauma = FMath::Max(0.f, CameraTrauma - DeltaSeconds * 1.5f);
	return Current;
}

void UMarsShowSubsystem::SetPhase(EMarsShowPhase NewPhase)
{
	Phase = NewPhase;
	PhaseStartTime = Now();
	UE_LOG(LogMars, Log, TEXT("Show phase -> %s"), *UEnum::GetValueAsString(NewPhase));
	OnPhaseChanged.Broadcast(NewPhase);
}

#undef LOCTEXT_NAMESPACE

void UMarsShowSubsystem::SetTerrain(AMarsTerrain* InTerrain)
{
	Terrain = InTerrain;
}
