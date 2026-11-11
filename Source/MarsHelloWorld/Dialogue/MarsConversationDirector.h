// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Dialogue/MarsDialogueTable.h"
#include "MarsConversationDirector.generated.h"

class ACombatantCharacter;
class UMarsShowSubsystem;

UENUM()
enum class EMarsDialogueCueKind : uint8
{
	Speak,              // Speaker delivers LineId, optionally facing Address
	BeginAccusation,    // Switch the show to the Accusation phase
	AccuserOpenFire,    // Accuser starts shooting the accused (pinned target)
	OthersJoinAccuser,  // Everyone else fights the accuser
	FreeForAll,         // Pins clear; each fighter treats all others as Kevin
	Wait                // Beat of silence (DelaySeconds)
};

USTRUCT()
struct FMarsDialogueCue
{
	GENERATED_BODY()

	UPROPERTY()
	EMarsDialogueCueKind Kind = EMarsDialogueCueKind::Wait;

	UPROPERTY()
	TObjectPtr<ACombatantCharacter> Speaker;

	UPROPERTY()
	TObjectPtr<ACombatantCharacter> Address;

	UPROPERTY()
	FName LineId;

	/** Pause after this cue before the next one (seconds). */
	UPROPERTY()
	float DelaySeconds = 0.4f;

	/** Play the character's greeting flourish (wave, hop, spin...) with the line. */
	UPROPERTY()
	bool bFlourish = false;

	/** Enter the Greeting state and hold until the line ends (conversation, not mid-fight). */
	UPROPERTY()
	bool bHoldForConversation = true;
};

/**
 * Arrival / conversation / accusation state machine.
 * Owned by UMarsShowSubsystem. Builds a cue list from the data-driven dialogue table:
 * arrivals looking for Kevin, "Are you Kevin?" / identity denials, a seeded first accusation,
 * then a free-for-all, with "I killed Kevin!" on each KO.
 */
UCLASS()
class MARSHELLOWORLD_API UMarsConversationDirector : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	void BeginShow(int32 InExpectedCombatants, int32 InSeed);
	void Stop();

	void SetPlayer(ACombatantCharacter* InPlayer);
	void NotifyArrived(ACombatantCharacter* Combatant);
	void NotifyDeath(ACombatantCharacter* Victim, ACombatantCharacter* Killer);

	ACombatantCharacter* GetAccuser() const { return Accuser.Get(); }
	ACombatantCharacter* GetAccused() const { return Accused.Get(); }
	EMarsClaimedName GetClaimedName(EMarsSpeakerId Speaker) const;

private:
	void AssignRoles();
	void EnqueueArrivalBeats(ACombatantCharacter* Newcomer);
	void EnqueueClosingConversation();
	void EnqueueSpeak(ACombatantCharacter* Speaker, FName LineId, ACombatantCharacter* Address, bool bFlourish, bool bHold, float Delay);
	void EnqueueWait(float Seconds);
	void Enqueue(const FMarsDialogueCue& Cue);
	void PlayNext();
	void FinishCueAndContinue(float DelaySeconds);
	void ExecuteCue(const FMarsDialogueCue& Cue);

	ACombatantCharacter* FindBySpeaker(EMarsSpeakerId Id) const;
	EMarsSpeakerId SpeakerOf(const ACombatantCharacter* Combatant) const;
	FName PickArrivalLine(EMarsSpeakerId Speaker);
	FName PickAskLine(ACombatantCharacter* Asker);
	FName PickAccuseLine() const;
	FName PickKillLine();
	UMarsShowSubsystem* GetShow() const;
	TArray<ACombatantCharacter*> Living() const;
	int32 LivingCount() const;

	UPROPERTY()
	TArray<FMarsDialogueCue> Cues;

	UPROPERTY()
	TArray<TObjectPtr<ACombatantCharacter>> Arrived;

	UPROPERTY()
	TArray<TObjectPtr<ACombatantCharacter>> Answered;

	TWeakObjectPtr<ACombatantCharacter> Accuser;
	TWeakObjectPtr<ACombatantCharacter> Accused;
	TWeakObjectPtr<ACombatantCharacter> Player;

	EMarsClaimedName ClaimedNames[3] = { EMarsClaimedName::Phil, EMarsClaimedName::Tyler, EMarsClaimedName::Danyon };
	FName ArrivalChoice[3];
	EMarsSpeakerId AccuserId = EMarsSpeakerId::Martian;
	EMarsSpeakerId AccusedId = EMarsSpeakerId::XBot;
	FName AccuseLineId;

	FRandomStream Stream;
	int32 ExpectedCombatants = 3;
	int32 Seed = 801;
	int32 AskCountBySpeaker[3] = { 0, 0, 0 };
	int32 KillLineCount = 0;
	bool bPlaying = false;
	bool bClosingQueued = false;
	bool bStopped = false;
	bool bAskedPlayer = false;

	FTimerHandle WaitHandle;
};
