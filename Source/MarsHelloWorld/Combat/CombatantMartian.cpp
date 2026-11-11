// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/CombatantMartian.h"
#include "Core/MarsGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#define LOCTEXT_NAMESPACE "MarsCombatants"

// The locals: a lanky little green Martian with a big brain, three eyes and antennae.
// Runs in, bounding in the low gravity. Close-range brawler with an acid spit.
ACombatantMartian::ACombatantMartian()
{
	Profile.DisplayName = LOCTEXT("MartianName", "Martian");
	Profile.GreetingLine = LOCTEXT("MartianHello", "Hello world, I'm looking for Kevin.");
	Profile.Species = MarsTags::Species_Martian;
	Profile.UIColor = FLinearColor(0.35f, 1.f, 0.3f);
	Profile.MaxHealth = 640.f;
	Profile.EntryStyle = EEntryStyle::Run;
	Profile.MoveSpeed = 700.f;
	Profile.PreferredRange = 520.f;
	Profile.Aggression = 0.8f;
	Profile.VoiceAsset = FSoftObjectPath(TEXT("/Game/Audio/VO_Martian_ArrivalHello.VO_Martian_ArrivalHello"));
	Profile.RawVoiceFile = TEXT("VO_Martian_ArrivalHello.wav");

	FCombatantAttack Claw;
	Claw.Name = TEXT("ClawSlam");
	Claw.Kind = EAttackKind::Melee;
	Claw.DamageType = MarsTags::Damage_Melee;
	Claw.Damage = 42.f;
	Claw.MaxRange = 240.f;
	Claw.Cooldown = 1.5f;
	Claw.Weight = 2.f;
	Claw.WindUp = 0.22f;
	Claw.Knockback = 750.f;
	Profile.Attacks.Add(Claw);

	FCombatantAttack Spit;
	Spit.Name = TEXT("AcidSpit");
	Spit.Kind = EAttackKind::Projectile;
	Spit.DamageType = MarsTags::Damage_Acid;
	Spit.Damage = 22.f;
	Spit.MinRange = 280.f;
	Spit.MaxRange = 2200.f;
	Spit.Cooldown = 2.0f;
	Spit.ProjectileSpeed = 1600.f;
	Spit.ProjectileGravityScale = 0.8f;
	Spit.ProjectileRadius = 22.f;
	Spit.Knockback = 250.f;
	Spit.Color = FLinearColor(0.55f, 1.f, 0.1f);
	Profile.Attacks.Add(Spit);

	FCombatantAttack Volley = Spit;
	Volley.Name = TEXT("AcidVolley");
	Volley.Damage = 9.f;
	Volley.ProjectileCount = 3;
	Volley.Cooldown = 4.5f;
	Volley.Weight = 0.6f;
	Volley.ProjectileRadius = 14.f;
	Profile.Attacks.Add(Volley);
}

void ACombatantMartian::BuildBody()
{
	const FLinearColor Skin(0.18f, 0.62f, 0.16f);
	const FLinearColor DarkSkin(0.08f, 0.32f, 0.07f);
	const FLinearColor EyeGlow(1.f, 0.9f, 0.2f);

	AddPart(TEXT("LegL"), EMarsShape::Cylinder, FVector(0.f, -17.f, 34.f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 0.68f), DarkSkin);
	AddPart(TEXT("LegR"), EMarsShape::Cylinder, FVector(0.f, 17.f, 34.f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 0.68f), DarkSkin);
	AddPart(TEXT("Torso"), EMarsShape::Sphere, FVector(0.f, 0.f, 92.f), FRotator::ZeroRotator, FVector(0.5f, 0.46f, 0.66f), Skin);
	AddPart(TEXT("Head"), EMarsShape::Sphere, FVector(4.f, 0.f, 160.f), FRotator::ZeroRotator, FVector(0.72f, 0.68f, 0.58f), Skin);
	AddPart(TEXT("EyeL"), EMarsShape::Sphere, FVector(34.f, -14.f, 164.f), FRotator::ZeroRotator, FVector(0.12f), EyeGlow, 4.f);
	AddPart(TEXT("EyeR"), EMarsShape::Sphere, FVector(34.f, 14.f, 164.f), FRotator::ZeroRotator, FVector(0.12f), EyeGlow, 4.f);
	AddPart(TEXT("EyeM"), EMarsShape::Sphere, FVector(36.f, 0.f, 178.f), FRotator::ZeroRotator, FVector(0.1f), EyeGlow, 4.f);
	AddPart(TEXT("AntennaL"), EMarsShape::Cylinder, FVector(0.f, -14.f, 202.f), FRotator(0.f, 0.f, -18.f), FVector(0.025f, 0.025f, 0.38f), DarkSkin);
	AddPart(TEXT("AntennaR"), EMarsShape::Cylinder, FVector(0.f, 14.f, 202.f), FRotator(0.f, 0.f, 18.f), FVector(0.025f, 0.025f, 0.38f), DarkSkin);
	AddPart(TEXT("AntennaTipL"), EMarsShape::Sphere, FVector(0.f, -21.f, 222.f), FRotator::ZeroRotator, FVector(0.07f), FLinearColor(1.f, 0.3f, 0.6f), 6.f);
	AddPart(TEXT("AntennaTipR"), EMarsShape::Sphere, FVector(0.f, 21.f, 222.f), FRotator::ZeroRotator, FVector(0.07f), FLinearColor(1.f, 0.3f, 0.6f), 6.f);
	AddPart(TEXT("ArmL"), EMarsShape::Cylinder, FVector(0.f, -36.f, 100.f), FRotator(0.f, 0.f, 15.f), FVector(0.1f, 0.1f, 0.6f), Skin);
	AddPart(TEXT("ArmR"), EMarsShape::Cylinder, FVector(0.f, 36.f, 100.f), FRotator(0.f, 0.f, -15.f), FVector(0.1f, 0.1f, 0.6f), Skin);
	AddPart(TEXT("ClawL"), EMarsShape::Cone, FVector(6.f, -44.f, 66.f), FRotator(180.f, 0.f, 0.f), FVector(0.14f, 0.14f, 0.2f), DarkSkin);
	AddPart(TEXT("ClawR"), EMarsShape::Cone, FVector(6.f, 44.f, 66.f), FRotator(180.f, 0.f, 0.f), FVector(0.14f, 0.14f, 0.2f), DarkSkin);
}

void ACombatantMartian::AnimateBody(float DeltaSeconds)
{
	const float T = GetAge();
	const float Speed = GetMoveSpeedFraction();
	const bool bAir = GetCharacterMovement()->IsFalling();

	const float Swing = bAir ? 25.f : 38.f * Speed * FMath::Sin(T * 13.f);
	if (UStaticMeshComponent* L = GetPart(TEXT("LegL"))) { L->SetRelativeRotation(FRotator(Swing, 0.f, 0.f)); }
	if (UStaticMeshComponent* R = GetPart(TEXT("LegR"))) { R->SetRelativeRotation(FRotator(bAir ? -25.f : -Swing, 0.f, 0.f)); }

	// Arms: counter-swing when running, raised when attacking or greeting.
	const bool bPose = (T - AttackPoseTime) < 0.35f;
	const bool bWave = (T - FlourishTime) < 1.8f;
	const float ArmL = bPose ? -80.f : (bWave ? 0.f : -Swing * 0.8f);
	const float ArmR = bPose ? -80.f : (bWave ? 0.f : Swing * 0.8f);
	const float WaveRoll = bWave ? 140.f + 25.f * FMath::Sin(T * 12.f) : -15.f;
	if (UStaticMeshComponent* A = GetPart(TEXT("ArmL"))) { A->SetRelativeRotation(FRotator(ArmL, 0.f, 15.f)); }
	if (UStaticMeshComponent* A = GetPart(TEXT("ArmR"))) { A->SetRelativeRotation(FRotator(ArmR, 0.f, WaveRoll)); }

	// Antennae wobble; body bob.
	const float Wobble = 10.f * FMath::Sin(T * 7.f);
	if (UStaticMeshComponent* A = GetPart(TEXT("AntennaL"))) { A->SetRelativeRotation(FRotator(Wobble, 0.f, -18.f)); }
	if (UStaticMeshComponent* A = GetPart(TEXT("AntennaR"))) { A->SetRelativeRotation(FRotator(-Wobble, 0.f, 18.f)); }
	BodyRoot->SetRelativeLocation(FVector(0.f, 0.f, -95.f + 4.f * Speed * FMath::Abs(FMath::Sin(T * 13.f))));
}

void ACombatantMartian::OnGreetingFlourish()
{
	FlourishTime = GetAge();
	Jump(); // a happy low-gravity hop while waving
}

void ACombatantMartian::OnAttackFired(const FCombatantAttack& Attack)
{
	AttackPoseTime = GetAge();
}

FVector ACombatantMartian::GetMuzzleLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * 45.f + FVector(0.f, 0.f, 55.f);
}

#undef LOCTEXT_NAMESPACE
