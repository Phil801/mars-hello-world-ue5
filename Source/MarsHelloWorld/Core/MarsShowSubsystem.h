// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MarsShowSubsystem.generated.h"

class ACombatantCharacter;
class AMarsTerrain;
class UMarsConversationDirector;

UENUM(BlueprintType)
enum class EMarsShowPhase : uint8
{
	Arrivals,
	Conversation,
	Accusation,
	Fight,
	Victory
};

/** Rising damage numbers and short call-outs drawn by AMarsHUD. */
USTRUCT(BlueprintType)
struct FMarsFloatingText
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	float SpawnTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	float Lifetime = 1.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct FMarsCaption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	FText Line;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Mars")
	float ExpireTime = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarsShowPhaseChanged, EMarsShowPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarsCombatantDied, ACombatantCharacter*, Victim, ACombatantCharacter*, Killer);

/**
 * The show's referee and stage manager (a world subsystem, not a singleton):
 * registers combatants, owns the conversation director (Kevin hunt, accusation, kill lines),
 * scales damage for sudden death, declares the winner, and holds the HUD's captions/banners.
 */
UCLASS()
class MARSHELLOWORLD_API UMarsShowSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	void BeginShow(int32 InExpectedCombatants, int32 InSeed);
	void SetTerrain(AMarsTerrain* InTerrain) { Terrain = InTerrain; }

	// --- Combatant / director callbacks ----------------------------------------------------------
	void RegisterCombatant(ACombatantCharacter* Combatant);
	void NotifyArrived(ACombatantCharacter* Combatant);
	void NotifyDeath(ACombatantCharacter* Victim, ACombatantCharacter* Killer);
	void ReportDamage(ACombatantCharacter* Victim, float Amount, const FVector& Where, const FLinearColor& Color);
	void SetCurrentSpeaker(ACombatantCharacter* Combatant);
	void SetPhase(EMarsShowPhase NewPhase);
	void MarkFightClockStarted();
	void StartFight();

	// --- Presentation ---------------------------------------------------------------------------
	void ShowCaption(const FText& Speaker, const FText& Line, const FLinearColor& Color, float Duration);
	void ShowBanner(const FText& Text, const FLinearColor& Color, float Duration, float Scale = 4.f);
	void AddFloatingText(const FVector& Where, const FString& Text, const FLinearColor& Color, float Scale = 1.f, float Lifetime = 1.2f);
	void AddCameraShake(float Amount) { CameraTrauma = FMath::Min(1.f, CameraTrauma + Amount); }

	// --- Queries ----------------------------------------------------------------------------------
	TArray<ACombatantCharacter*> GetCombatants(bool bAliveOnly) const;
	TArray<ACombatantCharacter*> GetNpcCombatants(bool bAliveOnly) const;
	ACombatantCharacter* GetPlayerAstronaut() const;
	ACombatantCharacter* GetCurrentSpeaker() const { return CurrentSpeaker.Get(); }
	ACombatantCharacter* GetWinner() const { return Winner.Get(); }
	EMarsShowPhase GetPhase() const { return Phase; }
	float GetPhaseStartTime() const { return PhaseStartTime; }
	float GetFightElapsed() const;
	float GetDamageMultiplier() const;
	bool IsSuddenDeath() const;
	float GetGroundZ(float X, float Y, float Fallback = 0.f) const;
	int32 GetSeed() const { return Seed; }
	float ConsumeCameraTrauma(float DeltaSeconds);
	float GetCameraTrauma() const { return CameraTrauma; }

	const FMarsCaption& GetCaption() const { return Caption; }
	const FMarsCaption& GetBanner() const { return Banner; }
	float GetBannerScale() const { return BannerScale; }
	const TArray<FMarsFloatingText>& GetFloatingTexts() const { return FloatingTexts; }
	const TArray<FMarsCaption>& GetKillFeed() const { return KillFeed; }
	void PruneExpired();

	UPROPERTY(BlueprintAssignable, Category = "Mars")
	FOnMarsShowPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Mars")
	FOnMarsCombatantDied OnCombatantDied;

private:
	void CheckForWinner();
	void ForceStragglers();
	float Now() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACombatantCharacter>> Combatants;

	UPROPERTY(Transient)
	TObjectPtr<UMarsConversationDirector> Director;

	TWeakObjectPtr<ACombatantCharacter> CurrentSpeaker;
	TWeakObjectPtr<ACombatantCharacter> Winner;
	TWeakObjectPtr<AMarsTerrain> Terrain;

	EMarsShowPhase Phase = EMarsShowPhase::Arrivals;
	float PhaseStartTime = 0.f;
	float FightStartTime = -1.f;
	int32 ExpectedCombatants = 3;
	int32 Seed = 801;
	float CameraTrauma = 0.f;
	float BannerScale = 4.f;

	FMarsCaption Caption;
	FMarsCaption Banner;
	TArray<FMarsFloatingText> FloatingTexts;
	TArray<FMarsCaption> KillFeed;

	FTimerHandle StragglerTimer;
	FTimerHandle RestartTimer;
};
