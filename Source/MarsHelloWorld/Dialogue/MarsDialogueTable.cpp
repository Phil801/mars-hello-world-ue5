// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Dialogue/MarsDialogueTable.h"
#include "Core/MarsGameplayTags.h"

#define LOCTEXT_NAMESPACE "MarsDialogue"

const FName MarsDialogueIds::ArrivalHello(TEXT("ArrivalHello"));
const FName MarsDialogueIds::ArrivalSeenKevin(TEXT("ArrivalSeenKevin"));
const FName MarsDialogueIds::ArrivalVariant(TEXT("ArrivalVariant"));
const FName MarsDialogueIds::AskAreYouKevin(TEXT("AskAreYouKevin"));
const FName MarsDialogueIds::AskAreYouKevinVariant(TEXT("AskAreYouKevinVariant"));
const FName MarsDialogueIds::DenyPhil(TEXT("DenyPhil"));
const FName MarsDialogueIds::DenyTyler(TEXT("DenyTyler"));
const FName MarsDialogueIds::DenyOther(TEXT("DenyOther"));
const FName MarsDialogueIds::KeepLooking(TEXT("KeepLooking"));
const FName MarsDialogueIds::Accuse(TEXT("Accuse"));
const FName MarsDialogueIds::AccuseVariant(TEXT("AccuseVariant"));
const FName MarsDialogueIds::AllAreKevin(TEXT("AllAreKevin"));
const FName MarsDialogueIds::KillKevin(TEXT("KillKevin"));
const FName MarsDialogueIds::KillKevinVariant(TEXT("KillKevinVariant"));


namespace
{
	FSoftObjectPath VoiceAssetPath(const TCHAR* FileStem)
	{
		return FSoftObjectPath(FString::Printf(TEXT("/Game/Audio/%s.%s"), FileStem, FileStem));
	}

	FMarsDialogueLine MakeLine(FName Id, const FText& Text, const TCHAR* FileStem)
	{
		FMarsDialogueLine Line;
		Line.LineId = Id;
		Line.Text = Text;
		Line.RawVoiceFile = FString(FileStem) + TEXT(".wav");
		Line.VoiceAsset = VoiceAssetPath(FileStem);
		return Line;
	}

	struct FSpeakerLines
	{
		EMarsSpeakerId Speaker = EMarsSpeakerId::Martian;
		TArray<FMarsDialogueLine> Lines;
	};

	const TArray<FSpeakerLines>& GetTable()
	{
		static const TArray<FSpeakerLines> Table = {
			{ EMarsSpeakerId::Martian, {
				MakeLine(MarsDialogueIds::ArrivalHello,           LOCTEXT("Martian_ArrivalHello", "Hello world, I'm looking for Kevin."), TEXT("VO_Martian_ArrivalHello")),
				MakeLine(MarsDialogueIds::ArrivalSeenKevin,       LOCTEXT("Martian_ArrivalSeenKevin", "Have you seen Kevin?"), TEXT("VO_Martian_ArrivalSeenKevin")),
				MakeLine(MarsDialogueIds::ArrivalVariant,         LOCTEXT("Martian_ArrivalVariant", "Hello, world! Kevin was supposed to meet me here."), TEXT("VO_Martian_ArrivalVariant")),
				MakeLine(MarsDialogueIds::AskAreYouKevin,         LOCTEXT("Martian_AskAreYouKevin", "Are you Kevin?"), TEXT("VO_Martian_AskAreYouKevin")),
				MakeLine(MarsDialogueIds::AskAreYouKevinVariant,  LOCTEXT("Martian_AskAreYouKevinVariant", "Wait. Are you Kevin?"), TEXT("VO_Martian_AskAreYouKevinVariant")),
				MakeLine(MarsDialogueIds::DenyPhil,               LOCTEXT("Martian_DenyPhil", "I'm Phil."), TEXT("VO_Martian_DenyPhil")),
				MakeLine(MarsDialogueIds::DenyTyler,              LOCTEXT("Martian_DenyTyler", "I'm Tyler."), TEXT("VO_Martian_DenyTyler")),
				MakeLine(MarsDialogueIds::DenyOther,              LOCTEXT("Martian_DenyOther", "Kevin? No. Not me."), TEXT("VO_Martian_DenyOther")),
				MakeLine(MarsDialogueIds::KeepLooking,            LOCTEXT("Martian_KeepLooking", "Still looking for Kevin."), TEXT("VO_Martian_KeepLooking")),
				MakeLine(MarsDialogueIds::Accuse,                 LOCTEXT("Martian_Accuse", "Wait... you're Kevin!"), TEXT("VO_Martian_Accuse")),
				MakeLine(MarsDialogueIds::AccuseVariant,          LOCTEXT("Martian_AccuseVariant", "That's Kevin! Get him!"), TEXT("VO_Martian_AccuseVariant")),
				MakeLine(MarsDialogueIds::AllAreKevin,            LOCTEXT("Martian_AllAreKevin", "You're all Kevin!"), TEXT("VO_Martian_AllAreKevin")),
				MakeLine(MarsDialogueIds::KillKevin,              LOCTEXT("Martian_KillKevin", "I killed Kevin!"), TEXT("VO_Martian_KillKevin")),
				MakeLine(MarsDialogueIds::KillKevinVariant,       LOCTEXT("Martian_KillKevinVariant", "Got him! I killed Kevin!"), TEXT("VO_Martian_KillKevinVariant")),
			}},
			{ EMarsSpeakerId::XBot, {
				MakeLine(MarsDialogueIds::ArrivalHello,           LOCTEXT("XBot_ArrivalHello", "Hello, world. I'm looking for Kevin."), TEXT("VO_XBot_ArrivalHello")),
				MakeLine(MarsDialogueIds::ArrivalSeenKevin,       LOCTEXT("XBot_ArrivalSeenKevin", "Have you seen Kevin?"), TEXT("VO_XBot_ArrivalSeenKevin")),
				MakeLine(MarsDialogueIds::ArrivalVariant,         LOCTEXT("XBot_ArrivalVariant", "Hello, world. Scanning for Kevin."), TEXT("VO_XBot_ArrivalVariant")),
				MakeLine(MarsDialogueIds::AskAreYouKevin,         LOCTEXT("XBot_AskAreYouKevin", "Are you Kevin?"), TEXT("VO_XBot_AskAreYouKevin")),
				MakeLine(MarsDialogueIds::AskAreYouKevinVariant,  LOCTEXT("XBot_AskAreYouKevinVariant", "Identify. Are you Kevin?"), TEXT("VO_XBot_AskAreYouKevinVariant")),
				MakeLine(MarsDialogueIds::DenyPhil,               LOCTEXT("XBot_DenyPhil", "I'm Phil."), TEXT("VO_XBot_DenyPhil")),
				MakeLine(MarsDialogueIds::DenyTyler,              LOCTEXT("XBot_DenyTyler", "I'm Tyler."), TEXT("VO_XBot_DenyTyler")),
				MakeLine(MarsDialogueIds::DenyOther,              LOCTEXT("XBot_DenyOther", "Negative. I am not Kevin."), TEXT("VO_XBot_DenyOther")),
				MakeLine(MarsDialogueIds::KeepLooking,            LOCTEXT("XBot_KeepLooking", "Kevin is not in this scan."), TEXT("VO_XBot_KeepLooking")),
				MakeLine(MarsDialogueIds::Accuse,                 LOCTEXT("XBot_Accuse", "Target identified. You are Kevin."), TEXT("VO_XBot_Accuse")),
				MakeLine(MarsDialogueIds::AccuseVariant,          LOCTEXT("XBot_AccuseVariant", "Kevin confirmed. Engaging."), TEXT("VO_XBot_AccuseVariant")),
				MakeLine(MarsDialogueIds::AllAreKevin,            LOCTEXT("XBot_AllAreKevin", "Recalculating. All units are Kevin."), TEXT("VO_XBot_AllAreKevin")),
				MakeLine(MarsDialogueIds::KillKevin,              LOCTEXT("XBot_KillKevin", "I killed Kevin!"), TEXT("VO_XBot_KillKevin")),
				MakeLine(MarsDialogueIds::KillKevinVariant,       LOCTEXT("XBot_KillKevinVariant", "Kevin eliminated."), TEXT("VO_XBot_KillKevinVariant")),
			}},
			{ EMarsSpeakerId::Venusian, {
				MakeLine(MarsDialogueIds::ArrivalHello,           LOCTEXT("Venusian_ArrivalHello", "Hello world, I'm looking for Kevin."), TEXT("VO_Venusian_ArrivalHello")),
				MakeLine(MarsDialogueIds::ArrivalSeenKevin,       LOCTEXT("Venusian_ArrivalSeenKevin", "Have you seen Kevin?"), TEXT("VO_Venusian_ArrivalSeenKevin")),
				MakeLine(MarsDialogueIds::ArrivalVariant,         LOCTEXT("Venusian_ArrivalVariant", "Hello, world! I'm looking for someone called Kevin."), TEXT("VO_Venusian_ArrivalVariant")),
				MakeLine(MarsDialogueIds::AskAreYouKevin,         LOCTEXT("Venusian_AskAreYouKevin", "Are you Kevin?"), TEXT("VO_Venusian_AskAreYouKevin")),
				MakeLine(MarsDialogueIds::AskAreYouKevinVariant,  LOCTEXT("Venusian_AskAreYouKevinVariant", "You there... are you Kevin?"), TEXT("VO_Venusian_AskAreYouKevinVariant")),
				MakeLine(MarsDialogueIds::DenyPhil,               LOCTEXT("Venusian_DenyPhil", "I'm Phil."), TEXT("VO_Venusian_DenyPhil")),
				MakeLine(MarsDialogueIds::DenyTyler,              LOCTEXT("Venusian_DenyTyler", "I'm Tyler."), TEXT("VO_Venusian_DenyTyler")),
				MakeLine(MarsDialogueIds::DenyOther,              LOCTEXT("Venusian_DenyOther", "Oh, no, I'm not Kevin."), TEXT("VO_Venusian_DenyOther")),
				MakeLine(MarsDialogueIds::KeepLooking,            LOCTEXT("Venusian_KeepLooking", "Has anyone seen Kevin?"), TEXT("VO_Venusian_KeepLooking")),
				MakeLine(MarsDialogueIds::Accuse,                 LOCTEXT("Venusian_Accuse", "I knew it! You're Kevin!"), TEXT("VO_Venusian_Accuse")),
				MakeLine(MarsDialogueIds::AccuseVariant,          LOCTEXT("Venusian_AccuseVariant", "Kevin! I found Kevin!"), TEXT("VO_Venusian_AccuseVariant")),
				MakeLine(MarsDialogueIds::AllAreKevin,            LOCTEXT("Venusian_AllAreKevin", "Wait... you're ALL Kevin!"), TEXT("VO_Venusian_AllAreKevin")),
				MakeLine(MarsDialogueIds::KillKevin,              LOCTEXT("Venusian_KillKevin", "I killed Kevin!"), TEXT("VO_Venusian_KillKevin")),
				MakeLine(MarsDialogueIds::KillKevinVariant,       LOCTEXT("Venusian_KillKevinVariant", "I killed Kevin! Finally!"), TEXT("VO_Venusian_KillKevinVariant")),
			}},
		};
		return Table;
	}
}

EMarsSpeakerId MarsDialogue::SpeakerFromSpecies(const FGameplayTag& Species)
{
	if (Species == MarsTags::Species_XBot)
	{
		return EMarsSpeakerId::XBot;
	}
	if (Species == MarsTags::Species_Venusian)
	{
		return EMarsSpeakerId::Venusian;
	}
	return EMarsSpeakerId::Martian;
}

const TCHAR* MarsDialogue::SpeakerLabel(EMarsSpeakerId Speaker)
{
	switch (Speaker)
	{
	case EMarsSpeakerId::XBot: return TEXT("X-Bot");
	case EMarsSpeakerId::Venusian: return TEXT("Venusian");
	default: return TEXT("Martian");
	}
}

const TCHAR* MarsDialogue::ClaimedNameLabel(EMarsClaimedName Name)
{
	switch (Name)
	{
	case EMarsClaimedName::Phil: return TEXT("Phil");
	case EMarsClaimedName::Tyler: return TEXT("Tyler");
	default: return TEXT("not Kevin");
	}
}

FName MarsDialogue::IdentityLineId(EMarsClaimedName Name)
{
	switch (Name)
	{
	case EMarsClaimedName::Phil: return MarsDialogueIds::DenyPhil;
	case EMarsClaimedName::Tyler: return MarsDialogueIds::DenyTyler;
	default: return MarsDialogueIds::DenyOther;
	}
}

TArray<FName, TInlineAllocator<3>> MarsDialogue::ArrivalLineIds()
{
	return { MarsDialogueIds::ArrivalHello, MarsDialogueIds::ArrivalSeenKevin, MarsDialogueIds::ArrivalVariant };
}

const FMarsDialogueLine* MarsDialogue::FindLine(EMarsSpeakerId Speaker, FName LineId)
{
	for (const FSpeakerLines& Pack : GetTable())
	{
		if (Pack.Speaker != Speaker)
		{
			continue;
		}
		for (const FMarsDialogueLine& Line : Pack.Lines)
		{
			if (Line.LineId == LineId)
			{
				return &Line;
			}
		}
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
