// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MarsDialogueTable.generated.h"

/** Who is saying a line. Matches the three combatant subclasses / Piper voices. */
UENUM(BlueprintType)
enum class EMarsSpeakerId : uint8
{
	Martian,
	XBot,
	Venusian,
	Astronaut
};

/** The name a combatant gives when asked "Are you Kevin?" Assigned at random (by show seed), no duplicates. */
UENUM(BlueprintType)
enum class EMarsClaimedName : uint8
{
	Phil,
	Tyler,
	Danyon
};

/**
 * One spoken line: subtitle text plus the RawAudio / imported SoundWave that plays it.
 * C++ data only (no .uasset Data Table). Keep the texts in sync with Tools/voice/generate_voices.py.
 */
USTRUCT(BlueprintType)
struct FMarsDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName LineId;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Text;

	/** WAV file under <Project>/RawAudio (e.g. VO_Martian_ArrivalHello.wav). */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FString RawVoiceFile;

	/** Imported SoundWave (Content/Python/mars_setup.py). Preferred when present. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FSoftObjectPath VoiceAsset;
};

/** Stable line ids. Every speaker has a recording for each of these. Defined in the .cpp (module load, after FName is up). */
namespace MarsDialogueIds
{
	extern const FName ArrivalHello;
	extern const FName ArrivalSeenKevin;
	extern const FName ArrivalVariant;
	extern const FName AskAreYouKevin;
	extern const FName AskAreYouKevinVariant;
	extern const FName DenyPhil;
	extern const FName DenyTyler;
	extern const FName DenyOther;   // retired (archived WAVs); kept so old ids still resolve
	extern const FName DenyDanyon;
	extern const FName KeepLooking;
	extern const FName Accuse;
	extern const FName AccuseVariant;
	extern const FName AllAreKevin;
	extern const FName KillKevin;
	extern const FName KillKevinVariant;
	extern const FName DenyKevin;
	extern const FName DenyKevinVariant;
	extern const FName VictoryHateKevin;
}

namespace MarsDialogue
{
	MARSHELLOWORLD_API EMarsSpeakerId SpeakerFromSpecies(const FGameplayTag& Species);
	MARSHELLOWORLD_API const TCHAR* SpeakerLabel(EMarsSpeakerId Speaker);
	MARSHELLOWORLD_API const TCHAR* ClaimedNameLabel(EMarsClaimedName Name);

	/** Identity line for a claimed name: DenyPhil, DenyTyler, or DenyDanyon. */
	MARSHELLOWORLD_API FName IdentityLineId(EMarsClaimedName Name);

	/** The three arrival-line variants, in table order. */
	MARSHELLOWORLD_API TArray<FName, TInlineAllocator<3>> ArrivalLineIds();

	/** Null if that speaker has no such line (should not happen for the ids above). */
	MARSHELLOWORLD_API const FMarsDialogueLine* FindLine(EMarsSpeakerId Speaker, FName LineId);
}
