// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Dialogue/MarsConversationDirector.h"
#include "Core/MarsShowSettings.h"
#include "Core/MarsShowSubsystem.h"
#include "Combat/CombatantCharacter.h"
#include "MarsHelloWorld.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWorld* UMarsConversationDirector::GetWorld() const
{
	const UObject* Outer = GetOuter();
	return Outer ? Outer->GetWorld() : nullptr;
}

UMarsShowSubsystem* UMarsConversationDirector::GetShow() const
{
	return Cast<UMarsShowSubsystem>(GetOuter());
}

void UMarsConversationDirector::BeginShow(int32 InExpectedCombatants, int32 InSeed)
{
	ExpectedCombatants = FMath::Max(1, InExpectedCombatants);
	Seed = InSeed;
	Stream.Initialize(Seed);
	AssignRoles();
	bStopped = false;
	bPlaying = false;
	bClosingQueued = false;
}

void UMarsConversationDirector::SetPlayer(ACombatantCharacter* InPlayer)
{
	Player = InPlayer;
}

void UMarsConversationDirector::Stop()
{
	bStopped = true;
	bPlaying = false;
	Cues.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
}

void UMarsConversationDirector::AssignRoles()
{
	TArray<EMarsClaimedName, TInlineAllocator<3>> Names = {
		EMarsClaimedName::Phil, EMarsClaimedName::Tyler, EMarsClaimedName::Danyon
	};
	for (int32 i = Names.Num() - 1; i > 0; --i)
	{
		const int32 j = Stream.RandRange(0, i);
		Names.Swap(i, j);
	}
	for (int32 i = 0; i < 3; ++i)
	{
		ClaimedNames[i] = Names[i];
	}

	const TArray<FName, TInlineAllocator<3>> ArrivalIds = MarsDialogue::ArrivalLineIds();
	for (int32 i = 0; i < 3; ++i)
	{
		ArrivalChoice[i] = ArrivalIds[Stream.RandRange(0, ArrivalIds.Num() - 1)];
	}

	AccuserId = static_cast<EMarsSpeakerId>(Stream.RandRange(0, 2));
	do
	{
		AccusedId = static_cast<EMarsSpeakerId>(Stream.RandRange(0, 2));
	} while (AccusedId == AccuserId);

	AccuseLineId = (Stream.FRand() < 0.5f) ? MarsDialogueIds::Accuse : MarsDialogueIds::AccuseVariant;

	UE_LOG(LogMars, Log, TEXT("Kevin hunt (seed %d): Martian is %s, X-Bot is %s, Venusian is %s; %s will accuse %s first"),
		Seed,
		MarsDialogue::ClaimedNameLabel(ClaimedNames[0]),
		MarsDialogue::ClaimedNameLabel(ClaimedNames[1]),
		MarsDialogue::ClaimedNameLabel(ClaimedNames[2]),
		MarsDialogue::SpeakerLabel(AccuserId),
		MarsDialogue::SpeakerLabel(AccusedId));
}

EMarsClaimedName UMarsConversationDirector::GetClaimedName(EMarsSpeakerId Speaker) const
{
	const int32 Index = static_cast<int32>(Speaker);
	return ClaimedNames[FMath::Clamp(Index, 0, 2)];
}

EMarsSpeakerId UMarsConversationDirector::SpeakerOf(const ACombatantCharacter* Combatant) const
{
	return Combatant ? MarsDialogue::SpeakerFromSpecies(Combatant->GetProfile().Species) : EMarsSpeakerId::Martian;
}

ACombatantCharacter* UMarsConversationDirector::FindBySpeaker(EMarsSpeakerId Id) const
{
	if (const UMarsShowSubsystem* Show = GetShow())
	{
		for (ACombatantCharacter* C : Show->GetCombatants(false))
		{
			if (C && SpeakerOf(C) == Id)
			{
				return C;
			}
		}
	}
	for (ACombatantCharacter* C : Arrived)
	{
		if (C && SpeakerOf(C) == Id)
		{
			return C;
		}
	}
	return nullptr;
}

FName UMarsConversationDirector::PickArrivalLine(EMarsSpeakerId Speaker)
{
	const int32 Index = FMath::Clamp(static_cast<int32>(Speaker), 0, 2);
	return ArrivalChoice[Index];
}

FName UMarsConversationDirector::PickAskLine(ACombatantCharacter* Asker)
{
	const int32 Index = FMath::Clamp(static_cast<int32>(SpeakerOf(Asker)), 0, 2);
	const int32 Count = AskCountBySpeaker[Index]++;
	return (Count == 0) ? MarsDialogueIds::AskAreYouKevin : MarsDialogueIds::AskAreYouKevinVariant;
}

FName UMarsConversationDirector::PickAccuseLine() const
{
	return AccuseLineId;
}

FName UMarsConversationDirector::PickKillLine()
{
	const int32 Count = KillLineCount++;
	return (Count % 2 == 0) ? MarsDialogueIds::KillKevin : MarsDialogueIds::KillKevinVariant;
}

TArray<ACombatantCharacter*> UMarsConversationDirector::Living() const
{
	TArray<ACombatantCharacter*> Out;
	if (const UMarsShowSubsystem* Show = GetShow())
	{
		Out = Show->GetCombatants(true);
	}
	return Out;
}

int32 UMarsConversationDirector::LivingCount() const
{
	return Living().Num();
}

void UMarsConversationDirector::Enqueue(const FMarsDialogueCue& Cue)
{
	Cues.Add(Cue);
}

void UMarsConversationDirector::EnqueueWait(float Seconds)
{
	FMarsDialogueCue Cue;
	Cue.Kind = EMarsDialogueCueKind::Wait;
	Cue.DelaySeconds = FMath::Max(0.05f, Seconds);
	Enqueue(Cue);
}

void UMarsConversationDirector::EnqueueSpeak(ACombatantCharacter* Speaker, FName LineId, ACombatantCharacter* Address, bool bFlourish, bool bHold, float Delay)
{
	if (!Speaker || LineId.IsNone())
	{
		return;
	}
	FMarsDialogueCue Cue;
	Cue.Kind = EMarsDialogueCueKind::Speak;
	Cue.Speaker = Speaker;
	Cue.Address = Address;
	Cue.LineId = LineId;
	Cue.DelaySeconds = Delay;
	Cue.bFlourish = bFlourish;
	Cue.bHoldForConversation = bHold;
	Enqueue(Cue);
}

void UMarsConversationDirector::NotifyArrived(ACombatantCharacter* Combatant)
{
	if (bStopped || !Combatant || Arrived.Contains(Combatant) || Combatant->IsPlayerAstronaut())
	{
		if (Combatant && Combatant->IsPlayerAstronaut())
		{
			Player = Combatant;
		}
		return;
	}

	UMarsShowSubsystem* Show = GetShow();
	const EMarsShowPhase Phase = Show ? Show->GetPhase() : EMarsShowPhase::Arrivals;
	if (Phase == EMarsShowPhase::Accusation || Phase == EMarsShowPhase::Fight || Phase == EMarsShowPhase::Victory)
	{
		Arrived.Add(Combatant);
		if (Phase != EMarsShowPhase::Victory)
		{
			Combatant->StartFighting();
		}
		return;
	}

	if (Show && Phase == EMarsShowPhase::Arrivals)
	{
		Show->SetPhase(EMarsShowPhase::Conversation);
	}

	Arrived.Add(Combatant);
	EnqueueArrivalBeats(Combatant);
	if (Arrived.Num() >= ExpectedCombatants && !bClosingQueued)
	{
		EnqueueClosingConversation();
	}
	if (!bPlaying)
	{
		PlayNext();
	}
}

void UMarsConversationDirector::EnqueueArrivalBeats(ACombatantCharacter* Newcomer)
{
	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	const float Gap = Settings->ConversationGapSeconds;

	EnqueueSpeak(Newcomer, PickArrivalLine(SpeakerOf(Newcomer)), nullptr, true, true, Gap);

	// First NPC to land asks the astronaut, who answers "No!"
	if (ACombatantCharacter* P = Player.Get())
	{
		if (!bAskedPlayer && P->IsAlive() && !Newcomer->IsPlayerAstronaut())
		{
			bAskedPlayer = true;
			EnqueueSpeak(Newcomer, PickAskLine(Newcomer), P, false, true, Gap);
			EnqueueSpeak(P, MarsDialogueIds::DenyKevin, Newcomer, false, true, Gap);
		}
	}

	// Identity asks only happen between NPCs (the player is not in Arrived).
	if (Arrived.Num() < 2)
	{
		return;
	}

	ACombatantCharacter* Asker = nullptr;
	for (int32 i = Arrived.Num() - 2; i >= 0; --i)
	{
		if (Arrived[i] && Arrived[i]->IsAlive())
		{
			Asker = Arrived[i];
			break;
		}
	}
	if (!Asker)
	{
		return;
	}

	EnqueueSpeak(Asker, PickAskLine(Asker), Newcomer, false, true, Gap);
	EnqueueSpeak(Newcomer, MarsDialogue::IdentityLineId(GetClaimedName(SpeakerOf(Newcomer))), Asker, false, true, Gap);
	if (!Answered.Contains(Newcomer))
	{
		Answered.Add(Newcomer);
	}
}

void UMarsConversationDirector::EnqueueClosingConversation()
{
	bClosingQueued = true;
	const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>();
	const float Gap = Settings->ConversationGapSeconds;

	// Safety: if the first NPC arrived before the player was registered, ask now.
	if (!bAskedPlayer)
	{
		if (ACombatantCharacter* P = Player.Get())
		{
			ACombatantCharacter* Asker = nullptr;
			for (ACombatantCharacter* C : Arrived)
			{
				if (C && C->IsAlive() && !C->IsPlayerAstronaut())
				{
					Asker = C;
					break;
				}
			}
			if (Asker && P->IsAlive())
			{
				bAskedPlayer = true;
				EnqueueSpeak(Asker, PickAskLine(Asker), P, false, true, Gap);
				EnqueueSpeak(P, MarsDialogueIds::DenyKevin, Asker, false, true, Gap);
			}
		}
	}

	// The first arriver never got asked "Are you Kevin?" — ask them now so every identity is heard.
	ACombatantCharacter* First = nullptr;
	for (ACombatantCharacter* C : Arrived)
	{
		if (C && C->IsAlive())
		{
			First = C;
			break;
		}
	}
	if (First && !Answered.Contains(First))
	{
		ACombatantCharacter* Asker = nullptr;
		for (ACombatantCharacter* C : Arrived)
		{
			if (C && C != First && C->IsAlive())
			{
				Asker = C;
				break;
			}
		}
		if (Asker)
		{
			EnqueueSpeak(Asker, PickAskLine(Asker), First, false, true, Gap);
			EnqueueSpeak(First, MarsDialogue::IdentityLineId(GetClaimedName(SpeakerOf(First))), Asker, false, true, Gap);
			Answered.Add(First);
		}
	}

	// A beat of still-searching, then the accusation.
	TArray<ACombatantCharacter*> Lookers;
	for (ACombatantCharacter* C : Arrived)
	{
		if (C && C->IsAlive())
		{
			Lookers.Add(C);
		}
	}
	const int32 LookCount = FMath::Min(2, Lookers.Num());
	for (int32 i = 0; i < LookCount; ++i)
	{
		const int32 Index = (Lookers.Num() > 1) ? (i + 1) % Lookers.Num() : 0;
		EnqueueSpeak(Lookers[Index], MarsDialogueIds::KeepLooking, nullptr, false, true, Gap);
	}

	FMarsDialogueCue AccusePhase;
	AccusePhase.Kind = EMarsDialogueCueKind::BeginAccusation;
	AccusePhase.DelaySeconds = 0.2f;
	Enqueue(AccusePhase);

	// Resolve accuser/accused now if they have already arrived; PlayNext will re-resolve too.
	ACombatantCharacter* AccuserPtr = FindBySpeaker(AccuserId);
	ACombatantCharacter* AccusedPtr = FindBySpeaker(AccusedId);
	EnqueueSpeak(AccuserPtr, PickAccuseLine(), AccusedPtr, true, true, 0.25f);

	FMarsDialogueCue Fire;
	Fire.Kind = EMarsDialogueCueKind::AccuserOpenFire;
	Fire.DelaySeconds = Settings->OthersJoinAfterSeconds;
	Enqueue(Fire);

	FMarsDialogueCue Join;
	Join.Kind = EMarsDialogueCueKind::OthersJoinAccuser;
	Join.DelaySeconds = Settings->FreeForAllAfterSeconds;
	Enqueue(Join);

	for (ACombatantCharacter* C : Lookers)
	{
		if (C && SpeakerOf(C) != AccuserId)
		{
			EnqueueSpeak(C, MarsDialogueIds::AllAreKevin, nullptr, false, false, Gap);
		}
	}

	FMarsDialogueCue FFA;
	FFA.Kind = EMarsDialogueCueKind::FreeForAll;
	FFA.DelaySeconds = 0.1f;
	Enqueue(FFA);
}

void UMarsConversationDirector::NotifyDeath(ACombatantCharacter* Victim, ACombatantCharacter* Killer)
{
	if (bStopped || !Killer || !Killer->IsAlive() || Killer->IsPlayerAstronaut())
	{
		return;
	}

	FMarsDialogueCue Cue;
	Cue.Kind = EMarsDialogueCueKind::Speak;
	Cue.Speaker = Killer;
	Cue.Address = Victim;
	Cue.LineId = PickKillLine();
	Cue.DelaySeconds = 0.2f;
	Cue.bFlourish = false;
	Cue.bHoldForConversation = false;

	// Play the kill line as soon as the current cue finishes (or now, if idle).
	Cues.Insert(Cue, 0);
	if (!bPlaying)
	{
		PlayNext();
	}
}

void UMarsConversationDirector::PlayNext()
{
	if (bStopped)
	{
		bPlaying = false;
		return;
	}

	while (Cues.Num() > 0)
	{
		const FMarsDialogueCue& Next = Cues[0];
		if (Next.Kind == EMarsDialogueCueKind::Speak && (!Next.Speaker || !Next.Speaker->IsAlive()))
		{
			Cues.RemoveAt(0);
			continue;
		}
		if ((Next.Kind == EMarsDialogueCueKind::OthersJoinAccuser || Next.Kind == EMarsDialogueCueKind::FreeForAll
			|| Next.Kind == EMarsDialogueCueKind::AccuserOpenFire || Next.Kind == EMarsDialogueCueKind::BeginAccusation)
			&& LivingCount() <= 1)
		{
			Cues.RemoveAt(0);
			continue;
		}
		break;
	}

	if (Cues.Num() == 0)
	{
		bPlaying = false;
		if (UMarsShowSubsystem* Show = GetShow())
		{
			Show->SetCurrentSpeaker(nullptr);
		}
		return;
	}

	bPlaying = true;
	const FMarsDialogueCue Cue = Cues[0];
	Cues.RemoveAt(0);
	ExecuteCue(Cue);
}

void UMarsConversationDirector::FinishCueAndContinue(float DelaySeconds)
{
	if (bStopped)
	{
		bPlaying = false;
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		bPlaying = false;
		return;
	}
	const float Wait = FMath::Max(0.01f, DelaySeconds);
	World->GetTimerManager().SetTimer(WaitHandle, FTimerDelegate::CreateUObject(this, &UMarsConversationDirector::PlayNext), Wait, false);
}

void UMarsConversationDirector::ExecuteCue(const FMarsDialogueCue& Cue)
{
	UMarsShowSubsystem* Show = GetShow();
	if (Show && Cue.Kind != EMarsDialogueCueKind::Speak)
	{
		Show->SetCurrentSpeaker(nullptr);
	}

	switch (Cue.Kind)
	{
	case EMarsDialogueCueKind::Wait:
		FinishCueAndContinue(Cue.DelaySeconds);
		break;

	case EMarsDialogueCueKind::Speak:
	{
		ACombatantCharacter* Speaker = Cue.Speaker;
		if (!Speaker || !Speaker->IsAlive())
		{
			FinishCueAndContinue(0.01f);
			break;
		}
		if (Show)
		{
			Show->SetCurrentSpeaker(Speaker);
		}
		const float HoldTime = Speaker->SpeakLine(Cue.LineId, Cue.Address, Cue.bHoldForConversation, Cue.bFlourish);
		FinishCueAndContinue(HoldTime + Cue.DelaySeconds);
		break;
	}

	case EMarsDialogueCueKind::BeginAccusation:
		if (Show)
		{
			Show->SetPhase(EMarsShowPhase::Accusation);
			Show->ShowBanner(NSLOCTEXT("MarsShow", "FoundKevin", "THAT'S KEVIN!"), FLinearColor(1.f, 0.85f, 0.4f), 1.6f, 3.5f);
		}
		Accuser = FindBySpeaker(AccuserId);
		Accused = FindBySpeaker(AccusedId);
		FinishCueAndContinue(Cue.DelaySeconds);
		break;

	case EMarsDialogueCueKind::AccuserOpenFire:
	{
		ACombatantCharacter* A = Accuser.IsValid() ? Accuser.Get() : FindBySpeaker(AccuserId);
		ACombatantCharacter* B = Accused.IsValid() ? Accused.Get() : FindBySpeaker(AccusedId);
		Accuser = A;
		Accused = B;
		if (Show)
		{
			Show->MarkFightClockStarted();
		}
		if (A && A->IsAlive() && B && B->IsAlive())
		{
			A->StartFightingAt(B, true, true);
			// Accused must already be Fighting so the first shots can land.
			B->StartFightingAt(A, true, false);
		}
		else if (A && A->IsAlive())
		{
			A->StartFighting();
		}
		FinishCueAndContinue(Cue.DelaySeconds);
		break;
	}

	case EMarsDialogueCueKind::OthersJoinAccuser:
	{
		ACombatantCharacter* A = Accuser.Get();
		for (ACombatantCharacter* C : Living())
		{
			if (C && C != A)
			{
				if (A && A->IsAlive())
				{
					C->StartFightingAt(A, true, true);
				}
				else
				{
					C->StartFighting();
				}
			}
		}
		FinishCueAndContinue(Cue.DelaySeconds);
		break;
	}

	case EMarsDialogueCueKind::FreeForAll:
		for (ACombatantCharacter* C : Living())
		{
			C->ClearTargetPin();
			if (C->GetCombatState() != ECombatantState::Fighting)
			{
				C->StartFighting();
			}
		}
		if (Show)
		{
			Show->StartFight();
		}
		FinishCueAndContinue(Cue.DelaySeconds);
		break;
	}

}
