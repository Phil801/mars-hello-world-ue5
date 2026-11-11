// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/CombatantXBot.h"
#include "Core/MarsGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#define LOCTEXT_NAMESPACE "MarsCombatants"

// X-Bot: a chrome combat robot that rockets in on boot thrusters, lands, and fights at range
// with eye lasers, closing in for a rocket punch. (Original design built from engine shapes;
// not Adobe Mixamo's "X Bot" mannequin.)
ACombatantXBot::ACombatantXBot()
{
	Profile.DisplayName = LOCTEXT("XBotName", "X-Bot");
	Profile.GreetingLine = LOCTEXT("XBotHello", "Hello, world.");
	Profile.Species = MarsTags::Species_XBot;
	Profile.UIColor = FLinearColor(0.3f, 0.85f, 1.f);
	Profile.MaxHealth = 600.f;
	Profile.EntryStyle = EEntryStyle::Fly;
	Profile.MoveSpeed = 560.f;
	Profile.FlySpeed = 1900.f;
	Profile.PreferredRange = 900.f;
	Profile.Aggression = 0.45f;
	Profile.VoiceAsset = FSoftObjectPath(TEXT("/Game/Audio/VO_XBot_HelloWorld.VO_XBot_HelloWorld"));
	Profile.RawVoiceFile = TEXT("VO_XBot_HelloWorld.wav");

	FCombatantAttack Laser;
	Laser.Name = TEXT("EyeLaser");
	Laser.Kind = EAttackKind::Projectile;
	Laser.DamageType = MarsTags::Damage_Laser;
	Laser.Damage = 12.f;
	Laser.MinRange = 200.f;
	Laser.MaxRange = 2600.f;
	Laser.Cooldown = 0.75f;
	Laser.Weight = 2.f;
	Laser.ProjectileSpeed = 4200.f;
	Laser.ProjectileRadius = 10.f;
	Laser.Knockback = 120.f;
	Laser.Color = FLinearColor(1.f, 0.12f, 0.05f);
	Profile.Attacks.Add(Laser);

	FCombatantAttack Burst = Laser;
	Burst.Name = TEXT("LaserBurst");
	Burst.Damage = 8.f;
	Burst.ProjectileCount = 3;
	Burst.Cooldown = 3.0f;
	Burst.Weight = 0.8f;
	Profile.Attacks.Add(Burst);

	FCombatantAttack Punch;
	Punch.Name = TEXT("RocketPunch");
	Punch.Kind = EAttackKind::Melee;
	Punch.DamageType = MarsTags::Damage_Melee;
	Punch.Damage = 38.f;
	Punch.MaxRange = 260.f;
	Punch.Cooldown = 2.2f;
	Punch.WindUp = 0.18f;
	Punch.Knockback = 1100.f;
	Profile.Attacks.Add(Punch);
}

void ACombatantXBot::BuildBody()
{
	const FLinearColor Chrome(0.62f, 0.64f, 0.68f);
	const FLinearColor Gunmetal(0.12f, 0.13f, 0.15f);
	const FLinearColor Cyan(0.2f, 0.9f, 1.f);
	const FLinearColor Flame(1.f, 0.45f, 0.08f);

	AddPart(TEXT("FootL"), EMarsShape::Cube, FVector(6.f, -18.f, 6.f), FRotator::ZeroRotator, FVector(0.3f, 0.16f, 0.12f), Gunmetal);
	AddPart(TEXT("FootR"), EMarsShape::Cube, FVector(6.f, 18.f, 6.f), FRotator::ZeroRotator, FVector(0.3f, 0.16f, 0.12f), Gunmetal);
	AddPart(TEXT("LegL"), EMarsShape::Cylinder, FVector(0.f, -18.f, 40.f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.6f), Chrome);
	AddPart(TEXT("LegR"), EMarsShape::Cylinder, FVector(0.f, 18.f, 40.f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.6f), Chrome);
	AddPart(TEXT("Torso"), EMarsShape::Cube, FVector(0.f, 0.f, 102.f), FRotator::ZeroRotator, FVector(0.42f, 0.58f, 0.56f), Chrome);
	AddPart(TEXT("Core"), EMarsShape::Cube, FVector(21.f, 0.f, 108.f), FRotator::ZeroRotator, FVector(0.03f, 0.2f, 0.2f), Cyan, 6.f);
	AddPart(TEXT("Head"), EMarsShape::Cube, FVector(0.f, 0.f, 158.f), FRotator::ZeroRotator, FVector(0.34f, 0.36f, 0.3f), Gunmetal);
	AddPart(TEXT("Visor"), EMarsShape::Cube, FVector(17.f, 0.f, 161.f), FRotator::ZeroRotator, FVector(0.02f, 0.3f, 0.07f), Cyan, 8.f);
	AddPart(TEXT("Antenna"), EMarsShape::Cylinder, FVector(0.f, 10.f, 186.f), FRotator::ZeroRotator, FVector(0.02f, 0.02f, 0.28f), Chrome);
	AddPart(TEXT("ArmL"), EMarsShape::Cylinder, FVector(0.f, -38.f, 104.f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.55f), Chrome);
	AddPart(TEXT("ArmR"), EMarsShape::Cylinder, FVector(0.f, 38.f, 104.f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.55f), Chrome);
	AddPart(TEXT("FistL"), EMarsShape::Cube, FVector(0.f, -38.f, 72.f), FRotator::ZeroRotator, FVector(0.16f), Gunmetal);
	AddPart(TEXT("FistR"), EMarsShape::Cube, FVector(0.f, 38.f, 72.f), FRotator::ZeroRotator, FVector(0.16f), Gunmetal);
	AddPart(TEXT("Jetpack"), EMarsShape::Cube, FVector(-28.f, 0.f, 108.f), FRotator::ZeroRotator, FVector(0.16f, 0.4f, 0.4f), Gunmetal);
	AddPart(TEXT("FlameL"), EMarsShape::Cone, FVector(-28.f, -12.f, 72.f), FRotator(180.f, 0.f, 0.f), FVector(0.12f, 0.12f, 0.35f), Flame, 15.f);
	AddPart(TEXT("FlameR"), EMarsShape::Cone, FVector(-28.f, 12.f, 72.f), FRotator(180.f, 0.f, 0.f), FVector(0.12f, 0.12f, 0.35f), Flame, 15.f);
}

void ACombatantXBot::AnimateBody(float DeltaSeconds)
{
	const float T = GetAge();
	const float Speed = GetMoveSpeedFraction();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bThrust = Move->IsFlying() || Move->IsFalling();

	// Thruster flames flicker when airborne, vanish on the ground.
	const float Flicker = bThrust ? 0.3f + 0.12f * FMath::Sin(T * 61.f) + 0.08f * FMath::Sin(T * 23.f) : 0.0f;
	for (const TCHAR* Name : { TEXT("FlameL"), TEXT("FlameR") })
	{
		if (UStaticMeshComponent* F = GetPart(Name))
		{
			F->SetVisibility(bThrust);
			F->SetRelativeScale3D(FVector(0.12f, 0.12f, FMath::Max(0.01f, Flicker * (Move->IsFlying() ? 1.6f : 1.f))));
		}
	}

	// Stiff robotic walk.
	const float Step = 22.f * Speed * FMath::Sign(FMath::Sin(T * 9.f));
	if (UStaticMeshComponent* L = GetPart(TEXT("LegL"))) { L->SetRelativeRotation(FRotator(bThrust ? 0.f : Step, 0.f, 0.f)); }
	if (UStaticMeshComponent* R = GetPart(TEXT("LegR"))) { R->SetRelativeRotation(FRotator(bThrust ? 0.f : -Step, 0.f, 0.f)); }

	const bool bPunch = (T - AttackPoseTime) < 0.3f;
	if (UStaticMeshComponent* A = GetPart(TEXT("ArmR"))) { A->SetRelativeRotation(FRotator(bPunch ? -90.f : 0.f, 0.f, 0.f)); }
	if (UStaticMeshComponent* Fst = GetPart(TEXT("FistR"))) { Fst->SetRelativeLocation(bPunch ? FVector(45.f, 38.f, 104.f) : FVector(0.f, 38.f, 72.f)); }

	// Greeting: a full robotic head spin.
	const float Since = T - FlourishTime;
	const float HeadYaw = Since < 1.2f ? 360.f * FMath::Clamp(Since / 1.2f, 0.f, 1.f) : 0.f;
	if (UStaticMeshComponent* H = GetPart(TEXT("Head"))) { H->SetRelativeRotation(FRotator(0.f, HeadYaw, 0.f)); }
	if (UStaticMeshComponent* V = GetPart(TEXT("Visor")))
	{
		const FVector Off = FRotator(0.f, HeadYaw, 0.f).RotateVector(FVector(17.f, 0.f, 0.f));
		V->SetRelativeLocation(FVector(Off.X, Off.Y, 161.f));
		V->SetRelativeRotation(FRotator(0.f, HeadYaw, 0.f));
	}
}

void ACombatantXBot::OnGreetingFlourish()
{
	FlourishTime = GetAge();
}

void ACombatantXBot::OnAttackFired(const FCombatantAttack& Attack)
{
	if (Attack.Kind == EAttackKind::Melee)
	{
		AttackPoseTime = GetAge();
	}
}

FVector ACombatantXBot::GetMuzzleLocation() const
{
	// Lasers come out of the visor.
	return GetActorLocation() + GetActorForwardVector() * 40.f + FVector(0.f, 0.f, 66.f);
}

#undef LOCTEXT_NAMESPACE
