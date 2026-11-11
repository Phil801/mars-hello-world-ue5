// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CombatantTypes.generated.h"

/** Life-cycle of a combatant. A plain C++ state machine: Entering -> Greeting -> Ready -> Fighting -> Dead | Victorious. */
UENUM(BlueprintType)
enum class ECombatantState : uint8
{
	Entering,
	Greeting,
	Ready,
	Fighting,
	Dead,
	Victorious
};

/** How a combatant arrives in the arena. */
UENUM(BlueprintType)
enum class EEntryStyle : uint8
{
	Run,   // sprints and bounds across the ground (Martian)
	Fly,   // thrusters in from the sky, then lands (X-Bot)
	Hover  // glides in and keeps floating (Venusian)
};

UENUM(BlueprintType)
enum class EAttackKind : uint8
{
	Melee,
	Projectile,
	Pulse
};

/** One attack. Data only, so new moves need no new code. */
USTRUCT(BlueprintType)
struct FCombatantAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	EAttackKind Kind = EAttackKind::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FGameplayTag DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Damage = 20.f;

	/** Attack is only chosen when the target is between MinRange and MaxRange (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float MaxRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Cooldown = 1.5f;

	/** Relative chance when several attacks are ready. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Weight = 1.f;

	/** Melee: delay between lunge and impact. Pulse: charge-up time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float WindUp = 0.2f;

	/** Melee/Pulse knockback speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Knockback = 700.f;

	/** Pulse radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float ProjectileSpeed = 2500.f;

	/** 0 = straight line. >0 = lobbed with a computed ballistic arc (uses Mars gravity). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float ProjectileGravityScale = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float HomingAcceleration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float ProjectileRadius = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	int32 ProjectileCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	FLinearColor Color = FLinearColor::White;
};

/** Everything that makes a combatant who it is. Filled in C++ by each subclass, overridable by a data asset. */
USTRUCT(BlueprintType)
struct FCombatantProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText GreetingLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FGameplayTag Species;

	/** Colour used for subtitles, health bar and kill feed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor UIColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	EEntryStyle EntryStyle = EEntryStyle::Run;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float FlySpeed = 1400.f;

	/** Hover style only: height above the ground to float at. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float HoverHeight = 0.f;

	/** Distance the AI likes to keep from its target when not going for a melee hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float PreferredRange = 700.f;

	/** 0..1: how likely it is to charge in for melee when melee is ready. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float Aggression = 0.5f;

	/** Imported SoundWave (created by Content/Python/mars_setup.py). Preferred when present. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	FSoftObjectPath VoiceAsset;

	/** Fallback: WAV file under <Project>/RawAudio played through a procedural sound wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	FString RawVoiceFile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<FCombatantAttack> Attacks;
};

/**
 * Optional data asset: create one in the editor (Miscellaneous > Data Asset > CombatantDefinition),
 * assign it to a combatant Blueprint's DefinitionOverride and tune without touching C++.
 */
UCLASS(BlueprintType)
class MARSHELLOWORLD_API UCombatantDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combatant")
	FCombatantProfile Profile;
};
