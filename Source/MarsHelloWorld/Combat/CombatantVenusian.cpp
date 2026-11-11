// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/CombatantVenusian.h"
#include "Core/MarsGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#define LOCTEXT_NAMESPACE "MarsCombatants"

// The Venusian: a luminous, jellyfish-like visitor from Venus's cloud decks (where life has been
// speculated to drift). Glides in and never touches the ground; homing plasma and a sonic pulse.
ACombatantVenusian::ACombatantVenusian()
{
	Profile.DisplayName = LOCTEXT("VenusianName", "Venusian");
	Profile.GreetingLine = LOCTEXT("VenusianHello", "Hello world, I'm looking for Kevin.");
	Profile.Species = MarsTags::Species_Venusian;
	Profile.UIColor = FLinearColor(1.f, 0.45f, 0.85f);
	Profile.MaxHealth = 560.f;
	Profile.EntryStyle = EEntryStyle::Hover;
	Profile.HoverHeight = 160.f;
	Profile.MoveSpeed = 500.f;
	Profile.FlySpeed = 1100.f;
	Profile.PreferredRange = 800.f;
	Profile.Aggression = 0.35f;
	Profile.VoiceAsset = FSoftObjectPath(TEXT("/Game/Audio/VO_Venusian_ArrivalHello.VO_Venusian_ArrivalHello"));
	Profile.RawVoiceFile = TEXT("VO_Venusian_ArrivalHello.wav");

	FCombatantAttack Orb;
	Orb.Name = TEXT("PlasmaOrb");
	Orb.Kind = EAttackKind::Projectile;
	Orb.DamageType = MarsTags::Damage_Plasma;
	Orb.Damage = 28.f;
	Orb.MinRange = 300.f;
	Orb.MaxRange = 2400.f;
	Orb.Cooldown = 2.3f;
	Orb.Weight = 2.f;
	Orb.ProjectileSpeed = 1100.f;
	Orb.HomingAcceleration = 2200.f;
	Orb.ProjectileRadius = 30.f;
	Orb.Knockback = 400.f;
	Orb.Color = FLinearColor(1.f, 0.2f, 0.9f);
	Profile.Attacks.Add(Orb);

	FCombatantAttack Pulse;
	Pulse.Name = TEXT("SonicPulse");
	Pulse.Kind = EAttackKind::Pulse;
	Pulse.DamageType = MarsTags::Damage_Sonic;
	Pulse.Damage = 34.f;
	Pulse.MaxRange = 420.f;
	Pulse.Radius = 460.f;
	Pulse.Cooldown = 4.0f;
	Pulse.Weight = 1.5f;
	Pulse.Knockback = 950.f;
	Pulse.Color = FLinearColor(1.f, 0.75f, 0.4f);
	Profile.Attacks.Add(Pulse);

	FCombatantAttack Lash;
	Lash.Name = TEXT("TentacleLash");
	Lash.Kind = EAttackKind::Melee;
	Lash.DamageType = MarsTags::Damage_Melee;
	Lash.Damage = 20.f;
	Lash.MaxRange = 230.f;
	Lash.Cooldown = 1.6f;
	Lash.WindUp = 0.15f;
	Lash.Knockback = 500.f;
	Profile.Attacks.Add(Lash);
}

void ACombatantVenusian::BuildBody()
{
	const FLinearColor Bell(1.f, 0.82f, 0.45f);
	const FLinearColor Core(1.f, 0.25f, 0.85f);
	const FLinearColor Halo(1.f, 0.85f, 0.35f);
	const FLinearColor Tentacle(0.85f, 0.4f, 0.75f);

	AddPart(TEXT("Bell"), EMarsShape::Sphere, FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, FVector(0.85f, 0.85f, 0.6f), Bell, 0.6f);
	AddPart(TEXT("Core"), EMarsShape::Sphere, FVector(0.f, 0.f, 112.f), FRotator::ZeroRotator, FVector(0.5f, 0.5f, 0.42f), Core, 8.f);
	AddPart(TEXT("Halo"), EMarsShape::Cylinder, FVector(0.f, 0.f, 172.f), FRotator::ZeroRotator, FVector(1.15f, 1.15f, 0.025f), Halo, 6.f);
	AddPart(TEXT("EyeL"), EMarsShape::Sphere, FVector(38.f, -14.f, 128.f), FRotator::ZeroRotator, FVector(0.11f), FLinearColor::White, 10.f);
	AddPart(TEXT("EyeR"), EMarsShape::Sphere, FVector(38.f, 14.f, 128.f), FRotator::ZeroRotator, FVector(0.11f), FLinearColor::White, 10.f);
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = 2.f * PI * float(i) / 6.f;
		const FName Name(*FString::Printf(TEXT("Tentacle%d"), i));
		AddPart(Name, EMarsShape::Cone, FVector(FMath::Cos(A) * 26.f, FMath::Sin(A) * 26.f, 58.f), FRotator(180.f, 0.f, 0.f),
			FVector(0.09f, 0.09f, 0.62f), Tentacle, 1.5f);
	}
}

void ACombatantVenusian::AnimateBody(float DeltaSeconds)
{
	const float T = GetAge();
	const float Since = T - FlourishTime;
	const float Spin = Since < 2.f ? 540.f : 90.f; // halo spins up while greeting

	if (UStaticMeshComponent* H = GetPart(TEXT("Halo")))
	{
		H->AddRelativeRotation(FRotator(0.f, Spin * DeltaSeconds, 0.f));
	}

	// Bell "breathes"; the core pulses brighter when it attacks.
	const float Breath = 1.f + 0.06f * FMath::Sin(T * 3.1f);
	if (UStaticMeshComponent* B = GetPart(TEXT("Bell"))) { B->SetRelativeScale3D(FVector(0.85f * Breath, 0.85f * Breath, 0.6f / Breath)); }
	const float CorePulse = (T - AttackPoseTime) < 0.4f ? 1.35f : 1.f;
	if (UStaticMeshComponent* C = GetPart(TEXT("Core"))) { C->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.42f) * CorePulse); }

	for (int32 i = 0; i < 6; ++i)
	{
		if (UStaticMeshComponent* Tn = GetPart(FName(*FString::Printf(TEXT("Tentacle%d"), i))))
		{
			const float Sway = 18.f * FMath::Sin(T * 2.6f + i * 1.1f);
			Tn->SetRelativeRotation(FRotator(180.f + Sway, 0.f, 12.f * FMath::Cos(T * 2.f + i)));
		}
	}

	// Lean into movement.
	const FVector LocalVel = GetActorRotation().UnrotateVector(GetVelocity());
	BodyRoot->SetRelativeRotation(FRotator(FMath::Clamp(static_cast<float>(-LocalVel.X) * 0.02f, -20.f, 20.f), 0.f,
		FMath::Clamp(static_cast<float>(LocalVel.Y) * 0.02f, -20.f, 20.f)));
}

void ACombatantVenusian::OnGreetingFlourish()
{
	FlourishTime = GetAge();
	KickVelocity(FVector(0.f, 0.f, 500.f), false, true); // rises up to sing its hello
}

void ACombatantVenusian::OnAttackFired(const FCombatantAttack& Attack)
{
	AttackPoseTime = GetAge();
}

FVector ACombatantVenusian::GetMuzzleLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * 55.f + FVector(0.f, 0.f, 20.f);
}

#undef LOCTEXT_NAMESPACE
