// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/CombatantAstronaut.h"
#include "Combat/MarsHealthComponent.h"
#include "Combat/MarsProjectile.h"
#include "Combat/MarsHitFlash.h"
#include "Core/MarsGameplayTags.h"
#include "Core/MarsShowSubsystem.h"
#include "Dialogue/MarsDialogueTable.h"
#include "World/MarsAssets.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "MarsCombatants"

ACombatantAstronaut::ACombatantAstronaut()
{
	Profile.DisplayName = LOCTEXT("AstronautName", "Astronaut");
	Profile.GreetingLine = LOCTEXT("AstronautNo", "No!");
	Profile.Species = MarsTags::Species_Astronaut;
	Profile.UIColor = FLinearColor(0.95f, 0.85f, 0.55f);
	Profile.MaxHealth = 120.f;
	Profile.EntryStyle = EEntryStyle::Run;
	Profile.MoveSpeed = 620.f;
	Profile.PreferredRange = 800.f;
	Profile.Aggression = 0.f;
	Profile.VoiceAsset = FSoftObjectPath(TEXT("/Game/Audio/VO_Astronaut_DenyKevin.VO_Astronaut_DenyKevin"));
	Profile.RawVoiceFile = TEXT("VO_Astronaut_DenyKevin.wav");

	FCombatantAttack Rifle;
	Rifle.Name = TEXT("Rifle");
	Rifle.Kind = EAttackKind::Projectile;
	Rifle.DamageType = MarsTags::Damage_Rifle;
	Rifle.Damage = 28.f;
	Rifle.MinRange = 0.f;
	Rifle.MaxRange = 8000.f;
	Rifle.Cooldown = 0.16f;
	Rifle.Weight = 1.f;
	Rifle.ProjectileSpeed = 9000.f;
	Rifle.ProjectileRadius = 8.f;
	Rifle.Knockback = 180.f;
	Rifle.Color = FLinearColor(1.f, 0.85f, 0.25f);
	Profile.Attacks.Add(Rifle);

	AutoPossessAI = EAutoPossessAI::Disabled;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = false;
	Move->MaxWalkSpeed = Profile.MoveSpeed;
	Move->NavAgentProps.bCanCrouch = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(12.f, 0.f, 64.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->FieldOfView = 95.f;

	ArmsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ArmsRoot"));
	ArmsRoot->SetupAttachment(FirstPersonCamera);
	ArmsRoot->SetRelativeLocation(FVector(18.f, 8.f, -18.f));

	ShieldBubble = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldBubble"));
	ShieldBubble->SetupAttachment(GetCapsuleComponent());
	ShieldBubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldBubble->SetCastShadow(false);
	ShieldBubble->SetVisibility(false);
}

void ACombatantAstronaut::BeginPlay()
{
	Super::BeginPlay();
	if (Health)
	{
		Health->SetImmortal(true, 1.f);
	}
	Shield = MaxShield;
	SetFirstPersonVisible(true);
}

void ACombatantAstronaut::PrepareAsPlayer(const FVector& InArenaCenter, int32 InSeed)
{
	LaunchEntrance(InArenaCenter, InArenaCenter, InSeed);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();
	ForceArrive(); // NotifyArrived is ignored for the player; lands them in Ready
	SetFirstPersonVisible(true);
}

void ACombatantAstronaut::BuildBody()
{
	const FLinearColor Suit(0.92f, 0.92f, 0.9f);
	const FLinearColor Orange(0.95f, 0.4f, 0.08f);
	const FLinearColor Dark(0.12f, 0.13f, 0.15f);
	const FLinearColor Gold(0.95f, 0.75f, 0.25f);
	const FLinearColor Visor(0.15f, 0.25f, 0.45f);

	// Third-person suit (visible in attract / debug camera, hidden in first person).
	AddPart(TEXT("TP_LegL"), EMarsShape::Cylinder, FVector(0.f, -16.f, 38.f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.72f), Dark);
	AddPart(TEXT("TP_LegR"), EMarsShape::Cylinder, FVector(0.f, 16.f, 38.f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.72f), Dark);
	AddPart(TEXT("TP_Torso"), EMarsShape::Cube, FVector(0.f, 0.f, 108.f), FRotator::ZeroRotator, FVector(0.48f, 0.62f, 0.62f), Suit);
	AddPart(TEXT("TP_Chest"), EMarsShape::Cube, FVector(22.f, 0.f, 112.f), FRotator::ZeroRotator, FVector(0.04f, 0.4f, 0.28f), Orange);
	AddPart(TEXT("TP_Pack"), EMarsShape::Cube, FVector(-28.f, 0.f, 118.f), FRotator::ZeroRotator, FVector(0.2f, 0.42f, 0.46f), Suit);
	AddPart(TEXT("TP_Helmet"), EMarsShape::Sphere, FVector(0.f, 0.f, 168.f), FRotator::ZeroRotator, FVector(0.38f, 0.36f, 0.36f), Suit);
	AddPart(TEXT("TP_Visor"), EMarsShape::Sphere, FVector(14.f, 0.f, 168.f), FRotator::ZeroRotator, FVector(0.26f, 0.3f, 0.22f), Visor, 1.5f);
	AddPart(TEXT("TP_ArmL"), EMarsShape::Cylinder, FVector(0.f, -40.f, 110.f), FRotator::ZeroRotator, FVector(0.11f, 0.11f, 0.55f), Suit);
	AddPart(TEXT("TP_ArmR"), EMarsShape::Cylinder, FVector(0.f, 40.f, 110.f), FRotator::ZeroRotator, FVector(0.11f, 0.11f, 0.55f), Suit);
	AddPart(TEXT("TP_GloveL"), EMarsShape::Sphere, FVector(0.f, -40.f, 76.f), FRotator::ZeroRotator, FVector(0.14f), Orange);
	AddPart(TEXT("TP_GloveR"), EMarsShape::Sphere, FVector(0.f, 40.f, 76.f), FRotator::ZeroRotator, FVector(0.14f), Orange);

	// First-person arms and rifle, parented to the camera so they pitch with the look.
	auto FP = [this](FName Name, EMarsShape Shape, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color, float Glow = 0.f)
	{
		UStaticMeshComponent* Part = AddPart(Name, Shape, Loc, Rot, Scale, Color, Glow, ArmsRoot);
		if (Part)
		{
			Part->SetCastShadow(false);
			Part->SetOnlyOwnerSee(true);
		}
		return Part;
	};
	FP(TEXT("FP_ArmR"), EMarsShape::Cylinder, FVector(-8.f, 14.f, -6.f), FRotator(80.f, 8.f, 0.f), FVector(0.1f, 0.1f, 0.45f), Suit);
	FP(TEXT("FP_GloveR"), EMarsShape::Sphere, FVector(14.f, 16.f, -10.f), FRotator::ZeroRotator, FVector(0.12f), Orange);
	FP(TEXT("FP_ArmL"), EMarsShape::Cylinder, FVector(6.f, -16.f, -8.f), FRotator(70.f, -12.f, 0.f), FVector(0.09f, 0.09f, 0.32f), Suit);
	FP(TEXT("FP_GloveL"), EMarsShape::Sphere, FVector(18.f, -14.f, -14.f), FRotator::ZeroRotator, FVector(0.1f), Orange);
	FP(TEXT("FP_Rifle"), EMarsShape::Cube, FVector(28.f, 10.f, -8.f), FRotator::ZeroRotator, FVector(0.55f, 0.1f, 0.1f), Dark);
	FP(TEXT("FP_Barrel"), EMarsShape::Cylinder, FVector(62.f, 10.f, -8.f), FRotator(90.f, 0.f, 0.f), FVector(0.05f, 0.05f, 0.28f), Dark);
	FP(TEXT("FP_Stock"), EMarsShape::Cube, FVector(4.f, 10.f, -6.f), FRotator::ZeroRotator, FVector(0.16f, 0.08f, 0.12f), Dark);
	FP(TEXT("FP_Sight"), EMarsShape::Cube, FVector(22.f, 10.f, -1.f), FRotator::ZeroRotator, FVector(0.08f, 0.04f, 0.08f), Orange, 2.f);

	if (UStaticMesh* Sphere = MarsAssets::GetShape(EMarsShape::Sphere))
	{
		ShieldBubble->SetStaticMesh(Sphere);
	}
	ShieldBubble->SetRelativeScale3D(FVector(2.1f));
	ShieldBubble->SetOnlyOwnerSee(true);
	ShieldBubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UMaterialInstanceDynamic* Mid = ShieldBubble->CreateDynamicMaterialInstance(0))
	{
		Mid->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.4f, 0.85f, 1.f, 0.15f));
		Mid->SetScalarParameterValue(TEXT("Glow"), 4.f);
	}
}

void ACombatantAstronaut::SetFirstPersonVisible(bool bInFirstPerson)
{
	bFirstPerson = bInFirstPerson;
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(bFirstPerson);
	}
	for (const TPair<FName, TObjectPtr<UStaticMeshComponent>>& Pair : Parts)
	{
		UStaticMeshComponent* Part = Pair.Value.Get();
		if (!Part)
		{
			continue;
		}
		const bool bFP = Pair.Key.ToString().StartsWith(TEXT("FP_"));
		if (bFP)
		{
			Part->SetVisibility(bFirstPerson);
			Part->SetOnlyOwnerSee(true);
		}
		else
		{
			Part->SetVisibility(true);
			Part->SetOwnerNoSee(bFirstPerson);
		}
	}
}

void ACombatantAstronaut::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickShield(DeltaSeconds);
	FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);
	Recoil = FMath::FInterpTo(Recoil, 0.f, DeltaSeconds, 10.f);
	ShieldFlash = FMath::FInterpTo(ShieldFlash, 0.f, DeltaSeconds, 4.f);

	if (bWantsFire)
	{
		FireWeapon();
	}

	if (ArmsRoot)
	{
		const float Speed = GetMoveSpeedFraction();
		WalkBob += DeltaSeconds * (6.f + 8.f * Speed) * (bSprinting ? 1.3f : 1.f);
		const float BobY = 1.2f * Speed * FMath::Sin(WalkBob);
		const float BobZ = 0.8f * Speed * FMath::Abs(FMath::Sin(WalkBob * 2.f));
		ArmsRoot->SetRelativeLocation(FVector(18.f, 8.f + BobY, -18.f + BobZ - Recoil * 4.f));
		ArmsRoot->SetRelativeRotation(FRotator(-Recoil * 6.f, 0.f, 0.f));
	}

	GetCharacterMovement()->MaxWalkSpeed = Profile.MoveSpeed * (bSprinting ? SprintMultiplier : 1.f);

	if (ShieldBubble)
	{
		const bool bShow = ShieldFlash > 0.04f || (Shield > 0.f && Shield < MaxShield && TimeSinceHit < 0.6f);
		ShieldBubble->SetVisibility(bShow);
		if (bShow)
		{
			const float Pulse = 1.9f + 0.25f * ShieldFlash;
			ShieldBubble->SetRelativeScale3D(FVector(Pulse));
		}
	}
}

void ACombatantAstronaut::TickShield(float DeltaSeconds)
{
	TimeSinceHit += DeltaSeconds;
	if (TimeSinceHit >= ShieldRegenDelay && Shield < MaxShield)
	{
		Shield = FMath::Min(MaxShield, Shield + ShieldRegenPerSecond * DeltaSeconds);
	}
}

void ACombatantAstronaut::AnimateBody(float DeltaSeconds)
{
	const float T = GetAge();
	const float Speed = GetMoveSpeedFraction();
	const float Step = 22.f * Speed * FMath::Sin(T * 9.f);
	if (UStaticMeshComponent* L = GetPart(TEXT("TP_LegL"))) { L->SetRelativeRotation(FRotator(Step, 0.f, 0.f)); }
	if (UStaticMeshComponent* R = GetPart(TEXT("TP_LegR"))) { R->SetRelativeRotation(FRotator(-Step, 0.f, 0.f)); }
}

void ACombatantAstronaut::OnGreetingFlourish()
{
	FlourishTime = GetAge();
}

void ACombatantAstronaut::OnAttackFired(const FCombatantAttack& Attack)
{
	AttackPoseTime = GetAge();
	Recoil = 1.f;
}

void ACombatantAstronaut::StartFighting()
{
	ACombatantCharacter::StartFighting();
	bTargetPinned = false;
	Target = nullptr;
}

void ACombatantAstronaut::StartFightingAt(ACombatantCharacter* InitialTarget, bool bPinTarget, bool bReadyToFire)
{
	(void)InitialTarget;
	(void)bPinTarget;
	(void)bReadyToFire;
	StartFighting();
}

void ACombatantAstronaut::Celebrate()
{
	ACombatantCharacter::Celebrate();
	SpeakLine(MarsDialogueIds::VictoryHateKevin, nullptr, false, false);
}

void ACombatantAstronaut::ReceiveMarsDamage(float Amount, ACombatantCharacter* Source, FGameplayTag DamageType, const FVector& HitLocation, const FVector& KnockbackVelocity)
{
	if (GetCombatState() != ECombatantState::Fighting)
	{
		return;
	}

	UMarsShowSubsystem* Show = GetWorld() ? GetWorld()->GetSubsystem<UMarsShowSubsystem>() : nullptr;
	const float Scaled = Amount * (Show ? Show->GetDamageMultiplier() : 1.f);
	float Remaining = Scaled;
	if (Shield > 0.f)
	{
		const float Taken = FMath::Min(Shield, Remaining);
		Shield -= Taken;
		Remaining -= Taken;
	}
	float AppliedHealth = 0.f;
	if (Remaining > 0.f && Health)
	{
		AppliedHealth = -Health->ApplyDelta(-Remaining);
	}

	TimeSinceHit = 0.f;
	ShieldFlash = 1.f;
	const FLinearColor HitColor = Source ? Source->GetProfile().UIColor : FLinearColor(0.5f, 0.9f, 1.f);
	AMarsHitFlash::Spawn(GetWorld(), HitLocation, HitColor, FMath::Clamp(Scaled / 25.f, 0.6f, 2.2f), 0.3f, EMarsShape::Sphere);
	if (!KnockbackVelocity.IsNearlyZero())
	{
		KickVelocity(KnockbackVelocity * 0.45f, true, false); // reduced so the player stays controllable
	}
	if (Show)
	{
		Show->ReportDamage(this, Scaled, GetHeadPoint(), HitColor);
		Show->AddCameraShake(FMath::Clamp(Scaled / 90.f, 0.08f, 0.4f));
	}
	(void)AppliedHealth;
	// Never Die(): health is immortal-clamped and the shield absorbs the first blow.
}

FVector ACombatantAstronaut::GetMuzzleLocation() const
{
	if (FirstPersonCamera)
	{
		return FirstPersonCamera->GetComponentLocation() + FirstPersonCamera->GetForwardVector() * 70.f + FirstPersonCamera->GetRightVector() * 18.f + FirstPersonCamera->GetUpVector() * -8.f;
	}
	return ACombatantCharacter::GetMuzzleLocation();
}

const FCombatantAttack* ACombatantAstronaut::GetRifleAttack() const
{
	return Profile.Attacks.Num() > 0 ? &Profile.Attacks[0] : nullptr;
}

void ACombatantAstronaut::FireWeapon()
{
	if (GetCombatState() != ECombatantState::Fighting || !IsAlive() || FireCooldown > 0.f)
	{
		return;
	}
	const FCombatantAttack* Attack = GetRifleAttack();
	if (!Attack)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !FirstPersonCamera)
	{
		return;
	}

	FireCooldown = Attack->Cooldown;
	OnAttackFired(*Attack);
	if (UMarsShowSubsystem* Show = GetShow())
	{
		Show->NotifyFirstShot();
	}

	const FVector Start = GetMuzzleLocation();
	const FVector Aim = FirstPersonCamera->GetComponentLocation() + FirstPersonCamera->GetForwardVector() * 4000.f;
	const FVector Velocity = (Aim - Start).GetSafeNormal() * Attack->ProjectileSpeed;

	const FTransform SpawnTM(Velocity.Rotation(), Start);
	AMarsProjectile* P = World->SpawnActorDeferred<AMarsProjectile>(AMarsProjectile::StaticClass(), SpawnTM, this, this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (P)
	{
		P->InitProjectile(this, *Attack, Velocity, nullptr);
		UGameplayStatics::FinishSpawningActor(P, SpawnTM);
	}
	AMarsHitFlash::Spawn(World, Start, Attack->Color, 0.35f, 0.12f, EMarsShape::Sphere);
}

void ACombatantAstronaut::MoveForward(float Value)
{
	if (FMath::IsNearlyZero(Value) || GetCombatState() == ECombatantState::Dead)
	{
		return;
	}
	AddMovementInput(FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector(), Value);
}

void ACombatantAstronaut::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value) || GetCombatState() == ECombatantState::Dead)
	{
		return;
	}
	const FVector Right = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	AddMovementInput(Right, Value);
}

void ACombatantAstronaut::AddLook(FVector2D Axis)
{
	if (AController* C = GetController())
	{
		C->AddYawInput(Axis.X);
		C->AddPitchInput(Axis.Y);
	}
}

void ACombatantAstronaut::StartSprint()
{
	bSprinting = true;
}

void ACombatantAstronaut::StopSprint()
{
	bSprinting = false;
}

void ACombatantAstronaut::OnJump()
{
	if (GetCombatState() != ECombatantState::Dead)
	{
		Jump();
	}
}

void ACombatantAstronaut::StartFire()
{
	bWantsFire = true;
	FireWeapon();
}

void ACombatantAstronaut::StopFire()
{
	bWantsFire = false;
}

#undef LOCTEXT_NAMESPACE
