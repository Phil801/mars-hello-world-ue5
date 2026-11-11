// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/CombatantCharacter.h"
#include "Combat/MarsHealthComponent.h"
#include "Combat/MarsProjectile.h"
#include "Combat/MarsHitFlash.h"
#include "Core/MarsGameplayTags.h"
#include "Core/MarsShowSubsystem.h"
#include "Audio/MarsVoiceLibrary.h"
#include "MarsHelloWorld.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

ACombatantCharacter::ACombatantCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(45.f, 95.f);

	// The AI controller is only a possession shell: CharacterMovement needs a controller to
	// consume AddMovementInput. Decisions are made in TickFighting below.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->JumpZVelocity = 620.f;           // ~5 m hops in Mars gravity
	Move->AirControl = 0.55f;
	Move->BrakingDecelerationWalking = 1600.f;
	Move->BrakingDecelerationFlying = 1400.f;
	Move->GravityScale = 1.f;              // world gravity is already Mars (-371)
	Move->bRunPhysicsWithNoController = true;

	// Hook for real art: the inherited skeletal Mesh is unused by default.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(GetCapsuleComponent());
	BodyRoot->SetRelativeLocation(FVector(0.f, 0.f, -95.f)); // feet at capsule bottom

	AuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AuraLight"));
	AuraLight->SetupAttachment(BodyRoot);
	AuraLight->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	AuraLight->IntensityUnits = ELightUnits::Unitless;
	AuraLight->Intensity = 0.f;
	AuraLight->AttenuationRadius = 400.f;
	AuraLight->CastShadows = false;

	Health = CreateDefaultSubobject<UMarsHealthComponent>(TEXT("Health"));
}

void ACombatantCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (DefinitionOverride)
	{
		Profile = DefinitionOverride->Profile;
	}

	Health->InitHealth(Profile.MaxHealth);
	AttackCooldowns.Init(0.f, Profile.Attacks.Num());

	GetCharacterMovement()->MaxWalkSpeed = Profile.MoveSpeed;
	GetCharacterMovement()->MaxFlySpeed = Profile.FlySpeed;

	if (bUsePrimitiveBody)
	{
		BuildBody();
		GetMesh()->SetVisibility(false);
	}

	AuraLight->SetLightColor(Profile.UIColor);

	if (UMarsShowSubsystem* Show = GetShow())
	{
		Show->RegisterCombatant(this);
	}
	SetCombatState(ECombatantState::Entering);
}

UMarsShowSubsystem* ACombatantCharacter::GetShow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UMarsShowSubsystem>() : nullptr;
}

bool ACombatantCharacter::IsAlive() const
{
	return Health && Health->IsAlive() && CombatState != ECombatantState::Dead;
}

FVector ACombatantCharacter::GetAimPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 10.f);
}

FVector ACombatantCharacter::GetHeadPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 150.f);
}

FVector ACombatantCharacter::GetMuzzleLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * 70.f + FVector(0.f, 0.f, 40.f);
}

float ACombatantCharacter::GetMoveSpeedFraction() const
{
	const float Max = GetCharacterMovement()->IsFlying() ? Profile.FlySpeed : Profile.MoveSpeed;
	return Max > 0.f ? FMath::Clamp(static_cast<float>(GetVelocity().Size2D()) / Max, 0.f, 1.f) : 0.f;
}

void ACombatantCharacter::KickVelocity(const FVector& DeltaVelocity, bool bOverrideXY, bool bOverrideZ)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move->IsFlying())
	{
		FVector V = Move->Velocity;
		V.X = bOverrideXY ? DeltaVelocity.X : V.X + DeltaVelocity.X;
		V.Y = bOverrideXY ? DeltaVelocity.Y : V.Y + DeltaVelocity.Y;
		V.Z = bOverrideZ ? DeltaVelocity.Z * 0.5f : V.Z + DeltaVelocity.Z * 0.5f;
		Move->Velocity = V;
	}
	else
	{
		LaunchCharacter(DeltaVelocity, bOverrideXY, bOverrideZ);
	}
}

UStaticMeshComponent* ACombatantCharacter::AddPart(FName Name, EMarsShape Shape, const FVector& Location, const FRotator& Rotation,
	const FVector& Scale, const FLinearColor& Color, float Glow, USceneComponent* Parent)
{
	UStaticMeshComponent* Part = MarsAssets::AddShape(this, Parent ? Parent : BodyRoot.Get(), Name, Shape, Location, Rotation, Scale, Color, Glow);
	if (Part)
	{
		Parts.Add(Name, Part);
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0)))
		{
			BodyColors.Emplace(MID, Color);
		}
	}
	return Part;
}

UStaticMeshComponent* ACombatantCharacter::GetPart(FName Name) const
{
	const TObjectPtr<UStaticMeshComponent>* Found = Parts.Find(Name);
	return Found ? Found->Get() : nullptr;
}

// ------------------------------------------------------------------------------------------------
// State machine
// ------------------------------------------------------------------------------------------------

void ACombatantCharacter::SetCombatState(ECombatantState NewState)
{
	CombatState = NewState;
	switch (NewState)
	{
	case ECombatantState::Entering:   CurrentStateTag = MarsTags::State_Entering; break;
	case ECombatantState::Greeting:   CurrentStateTag = MarsTags::State_Greeting; break;
	case ECombatantState::Ready:      CurrentStateTag = MarsTags::State_Ready; break;
	case ECombatantState::Fighting:   CurrentStateTag = MarsTags::State_Fighting; break;
	case ECombatantState::Dead:       CurrentStateTag = MarsTags::State_Dead; break;
	case ECombatantState::Victorious: CurrentStateTag = MarsTags::State_Victorious; break;
	}
	UE_LOG(LogMars, Verbose, TEXT("%s -> %s"), *GetName(), *CurrentStateTag.ToString());
}

void ACombatantCharacter::LaunchEntrance(const FVector& InArrivalPoint, const FVector& InArenaCenter, int32 InSeed)
{
	ArrivalPoint = InArrivalPoint;
	ArenaCenter = InArenaCenter;
	Stream.Initialize(InSeed);
	SetCombatState(ECombatantState::Entering);

	if (Profile.EntryStyle == EEntryStyle::Run)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
}

void ACombatantCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;

	// Undo the white hit flash.
	if (HitFlashTimer > 0.f)
	{
		HitFlashTimer -= DeltaSeconds;
		if (HitFlashTimer <= 0.f)
		{
			for (const TPair<TWeakObjectPtr<UMaterialInstanceDynamic>, FLinearColor>& Pair : BodyColors)
			{
				if (UMaterialInstanceDynamic* MID = Pair.Key.Get())
				{
					MID->SetVectorParameterValue(TEXT("Color"), Pair.Value);
				}
			}
		}
	}

	switch (CombatState)
	{
	case ECombatantState::Entering:
		TickEntering(DeltaSeconds);
		break;
	case ECombatantState::Fighting:
		TickFighting(DeltaSeconds);
		break;
	case ECombatantState::Greeting:
	case ECombatantState::Ready:
		TickHover(DeltaSeconds);
		if (CombatState == ECombatantState::Ready || CombatState == ECombatantState::Greeting)
		{
			// Face the middle of the arena while waiting.
			const FVector To = (ArenaCenter - GetActorLocation()).GetSafeNormal2D();
			if (!To.IsNearlyZero())
			{
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), To.Rotation(), DeltaSeconds, 4.f));
			}
		}
		break;
	case ECombatantState::Victorious:
		TickHover(DeltaSeconds);
		HopTimer -= DeltaSeconds;
		if (HopTimer <= 0.f && GetCharacterMovement()->IsMovingOnGround())
		{
			Jump();
			HopTimer = 1.6f;
		}
		AddActorWorldRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
		break;
	default:
		break;
	}

	if (CombatState != ECombatantState::Dead && bUsePrimitiveBody)
	{
		AnimateBody(DeltaSeconds);
	}
}

void ACombatantCharacter::TickEntering(float DeltaSeconds)
{
	const FVector Loc = GetActorLocation();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const UMarsShowSubsystem* Show = GetShow();
	const float GroundZ = Show ? Show->GetGroundZ(ArrivalPoint.X, ArrivalPoint.Y, ArrivalPoint.Z) : ArrivalPoint.Z;

	switch (Profile.EntryStyle)
	{
	case EEntryStyle::Run:
	{
		const FVector To = ArrivalPoint - Loc;
		if (To.Size2D() < 90.f)
		{
			ArriveNow();
			return;
		}
		AddMovementInput(To.GetSafeNormal2D(), 1.f);
		// Low gravity: the Martian bounds across the plain.
		HopTimer -= DeltaSeconds;
		if (HopTimer <= 0.f && Move->IsMovingOnGround() && To.Size2D() > 900.f)
		{
			Jump();
			HopTimer = Stream.FRandRange(1.1f, 1.8f);
		}
		break;
	}
	case EEntryStyle::Fly:
	{
		if (bLandingFromFlight)
		{
			if (Move->IsMovingOnGround())
			{
				ArriveNow();
			}
			return;
		}
		const FVector Goal(ArrivalPoint.X, ArrivalPoint.Y, GroundZ + 450.f);
		const FVector To = Goal - Loc;
		if (To.Size() < 220.f)
		{
			// Cut thrusters and drop the last few metres in Mars gravity.
			bLandingFromFlight = true;
			Move->SetMovementMode(MOVE_Falling);
			return;
		}
		AddMovementInput(To.GetSafeNormal(), 1.f);
		break;
	}
	case EEntryStyle::Hover:
	{
		const FVector Goal(ArrivalPoint.X, ArrivalPoint.Y, GroundZ + Profile.HoverHeight + 95.f);
		const FVector To = Goal - Loc;
		if (To.Size() < 120.f)
		{
			ArriveNow();
			return;
		}
		// Ease in: slow down near the goal so it glides to a stop.
		AddMovementInput(To.GetSafeNormal(), FMath::Clamp(static_cast<float>(To.Size()) / 900.f, 0.25f, 1.f));
		break;
	}
	}
}

void ACombatantCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (Profile.EntryStyle == EEntryStyle::Hover && IsAlive())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying); // Venusians never stay on the ground
		return;
	}
	if (bLandingFromFlight && CombatState == ECombatantState::Entering)
	{
		if (UMarsShowSubsystem* Show = GetShow())
		{
			Show->AddCameraShake(0.25f);
		}
		AMarsHitFlash::Spawn(GetWorld(), GetActorLocation() - FVector(0.f, 0.f, 90.f), FLinearColor(0.6f, 0.3f, 0.15f), 2.5f, 0.6f, EMarsShape::Cylinder);
	}
}

void ACombatantCharacter::ArriveNow()
{
	GetCharacterMovement()->StopMovementImmediately();
	SetCombatState(ECombatantState::Ready);
	if (UMarsShowSubsystem* Show = GetShow())
	{
		Show->NotifyArrived(this);
	}
}

void ACombatantCharacter::ForceArrive()
{
	const UMarsShowSubsystem* Show = GetShow();
	const float GroundZ = Show ? Show->GetGroundZ(ArrivalPoint.X, ArrivalPoint.Y, ArrivalPoint.Z) : ArrivalPoint.Z;
	SetActorLocation(FVector(ArrivalPoint.X, ArrivalPoint.Y, GroundZ + 100.f + Profile.HoverHeight), false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->SetMovementMode(Profile.EntryStyle == EEntryStyle::Hover ? MOVE_Flying : MOVE_Falling);
	ArriveNow();
}

void ACombatantCharacter::TickHover(float DeltaSeconds)
{
	if (Profile.EntryStyle != EEntryStyle::Hover || !GetCharacterMovement()->IsFlying())
	{
		return;
	}
	// Hold altitude with a gentle bob.
	const UMarsShowSubsystem* Show = GetShow();
	const FVector Loc = GetActorLocation();
	const float GroundZ = Show ? Show->GetGroundZ(Loc.X, Loc.Y, Loc.Z - Profile.HoverHeight) : Loc.Z - Profile.HoverHeight;
	const float DesiredZ = GroundZ + Profile.HoverHeight + 95.f + 25.f * FMath::Sin(Age * 1.7f);
	const float Error = DesiredZ - Loc.Z;
	AddMovementInput(FVector::UpVector, FMath::Clamp(Error / 150.f, -1.f, 1.f));
}

// ------------------------------------------------------------------------------------------------
// Greeting
// ------------------------------------------------------------------------------------------------

void ACombatantCharacter::StartGreeting()
{
	SetCombatState(ECombatantState::Greeting);
	AuraLight->SetIntensity(6000.f);
	OnGreetingFlourish();
	PlayVoice(true);
}

void ACombatantCharacter::PlayVoice(bool bShowSubtitle)
{
	float Duration = 1.8f;
	USoundBase* Voice = MarsVoice::ResolveVoice(this, Profile, Duration);
	if (Voice)
	{
		UGameplayStatics::SpawnSound2D(this, Voice);
	}

	UMarsShowSubsystem* Show = GetShow();
	if (Show && bShowSubtitle)
	{
		Show->ShowCaption(Profile.DisplayName, Profile.GreetingLine, Profile.UIColor, Duration + 0.6f);
		Show->AddFloatingText(GetHeadPoint() + FVector(0.f, 0.f, 40.f), Profile.GreetingLine.ToString(), Profile.UIColor, 1.6f, Duration + 0.6f);
	}

	if (CombatState == ECombatantState::Greeting)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			AuraLight->SetIntensity(0.f);
			if (CombatState == ECombatantState::Greeting)
			{
				SetCombatState(ECombatantState::Ready);
			}
			if (UMarsShowSubsystem* S = GetShow())
			{
				S->NotifyGreetingFinished(this);
			}
		}), Duration + 0.5f, false);
	}
}

// ------------------------------------------------------------------------------------------------
// Fighting
// ------------------------------------------------------------------------------------------------

void ACombatantCharacter::StartFighting()
{
	if (!IsAlive())
	{
		return;
	}
	SetCombatState(ECombatantState::Fighting);
	GetCharacterMovement()->bOrientRotationToMovement = false; // we face our target manually
	for (int32 i = 0; i < AttackCooldowns.Num(); ++i)
	{
		AttackCooldowns[i] = Stream.FRandRange(0.2f, 1.2f); // desync the first volley
	}
	RetargetTimer = 0.f;
	StrafeSign = Stream.FRand() < 0.5f ? -1.f : 1.f;
}

void ACombatantCharacter::ChooseTarget()
{
	const UMarsShowSubsystem* Show = GetShow();
	if (!Show)
	{
		return;
	}
	ACombatantCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (ACombatantCharacter* Other : Show->GetCombatants(true))
	{
		if (Other == this)
		{
			continue;
		}
		float Score = static_cast<float>(FVector::Dist(Other->GetActorLocation(), GetActorLocation()));
		if (Other == LastAttacker)
		{
			Score -= 700.f; // grudges
		}
		if (Other == Target)
		{
			Score -= 250.f; // some focus
		}
		Score += Stream.FRandRange(0.f, 500.f);
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Other;
		}
	}
	Target = Best;
	RetargetTimer = Stream.FRandRange(2.5f, 4.5f);
}

void ACombatantCharacter::TickFighting(float DeltaSeconds)
{
	for (float& Cd : AttackCooldowns)
	{
		Cd -= DeltaSeconds;
	}
	GlobalAttackLockout -= DeltaSeconds;
	RetargetTimer -= DeltaSeconds;
	StrafeTimer -= DeltaSeconds;
	ChargeTimer -= DeltaSeconds;
	HopTimer -= DeltaSeconds;

	if (!Target || !Target->IsAlive() || RetargetTimer <= 0.f)
	{
		ChooseTarget();
	}
	if (!Target)
	{
		return;
	}

	const FVector MyLoc = GetActorLocation();
	const FVector ToTarget = Target->GetActorLocation() - MyLoc;
	const float Dist = static_cast<float>(ToTarget.Size2D());
	const FVector Dir = ToTarget.GetSafeNormal2D();

	// Decide whether to charge in for melee.
	if (ChargeTimer <= 0.f && Stream.FRand() < Profile.Aggression * DeltaSeconds)
	{
		ChargeTimer = Stream.FRandRange(1.5f, 2.5f);
	}
	float DesiredRange = Profile.PreferredRange;
	if (ChargeTimer > 0.f)
	{
		for (const FCombatantAttack& A : Profile.Attacks)
		{
			if (A.Kind == EAttackKind::Melee)
			{
				DesiredRange = A.MaxRange * 0.6f;
				break;
			}
		}
	}

	if (StrafeTimer <= 0.f)
	{
		StrafeSign = Stream.FRand() < 0.5f ? -1.f : 1.f;
		StrafeTimer = Stream.FRandRange(1.2f, 2.8f);
	}

	const FVector Right = FVector::CrossProduct(FVector::UpVector, Dir);
	const float Radial = FMath::Clamp((Dist - DesiredRange) / 300.f, -1.f, 1.f);
	FVector Move = Dir * Radial + Right * StrafeSign * 0.7f;

	// Stay inside the arena so the camera can frame everyone.
	const FVector FromCenter = (MyLoc - ArenaCenter) * FVector(1.f, 1.f, 0.f);
	const float CenterDist = static_cast<float>(FromCenter.Size());
	if (CenterDist > 1500.f)
	{
		Move += -FromCenter.GetSafeNormal() * ((CenterDist - 1500.f) / 300.f);
	}
	AddMovementInput(Move.GetSafeNormal(), 1.f);

	// Occasional low-gravity hop (dodges projectiles, looks great).
	if (Profile.EntryStyle != EEntryStyle::Hover && HopTimer <= 0.f && GetCharacterMovement()->IsMovingOnGround() && Stream.FRand() < 0.35f * DeltaSeconds * 4.f)
	{
		Jump();
		HopTimer = Stream.FRandRange(1.5f, 3.5f);
	}

	TickHover(DeltaSeconds);

	// Face the target.
	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Dir.Rotation(), DeltaSeconds, 8.f));
	}

	TryAttack();
}

bool ACombatantCharacter::TryAttack()
{
	if (GlobalAttackLockout > 0.f || !Target)
	{
		return false;
	}
	const float Dist = static_cast<float>(FVector::Dist(Target->GetActorLocation(), GetActorLocation()));

	float TotalWeight = 0.f;
	TArray<int32, TInlineAllocator<8>> Ready;
	for (int32 i = 0; i < Profile.Attacks.Num(); ++i)
	{
		const FCombatantAttack& A = Profile.Attacks[i];
		if (AttackCooldowns[i] <= 0.f && Dist >= A.MinRange && Dist <= A.MaxRange)
		{
			Ready.Add(i);
			TotalWeight += A.Weight;
		}
	}
	if (Ready.Num() == 0 || TotalWeight <= 0.f)
	{
		return false;
	}

	float Pick = Stream.FRandRange(0.f, TotalWeight);
	int32 Chosen = Ready.Last();
	for (int32 Index : Ready)
	{
		Pick -= Profile.Attacks[Index].Weight;
		if (Pick <= 0.f)
		{
			Chosen = Index;
			break;
		}
	}

	const FCombatantAttack& Attack = Profile.Attacks[Chosen];
	AttackCooldowns[Chosen] = Attack.Cooldown * Stream.FRandRange(0.85f, 1.15f);
	GlobalAttackLockout = 0.45f;
	ExecuteAttack(Attack);
	return true;
}

void ACombatantCharacter::ExecuteAttack(const FCombatantAttack& Attack)
{
	OnAttackFired(Attack);
	switch (Attack.Kind)
	{
	case EAttackKind::Melee:      ExecuteMelee(Attack, Target); break;
	case EAttackKind::Projectile: ExecuteProjectile(Attack, Target); break;
	case EAttackKind::Pulse:      ExecutePulse(Attack); break;
	}
}

void ACombatantCharacter::ExecuteMelee(const FCombatantAttack& Attack, ACombatantCharacter* Victim)
{
	if (!Victim)
	{
		return;
	}
	// Lunge, then resolve the hit after the wind-up if the victim is still close.
	const FVector Dir = (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	KickVelocity(Dir * 900.f + FVector(0.f, 0.f, 160.f), true, false);

	TWeakObjectPtr<ACombatantCharacter> WeakVictim(Victim);
	const FCombatantAttack AttackCopy = Attack;
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, WeakVictim, AttackCopy]()
	{
		ACombatantCharacter* V = WeakVictim.Get();
		if (!V || !V->IsAlive() || !IsAlive())
		{
			return;
		}
		const float D = static_cast<float>(FVector::Dist(V->GetActorLocation(), GetActorLocation()));
		if (D > AttackCopy.MaxRange * 1.4f)
		{
			if (UMarsShowSubsystem* Show = GetShow())
			{
				Show->AddFloatingText(V->GetHeadPoint(), TEXT("miss"), FLinearColor(0.8f, 0.8f, 0.8f), 0.9f, 0.8f);
			}
			return;
		}
		const FVector Push = (V->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * AttackCopy.Knockback + FVector(0.f, 0.f, AttackCopy.Knockback * 0.45f);
		const FVector HitAt = (GetActorLocation() + V->GetActorLocation()) * 0.5f + FVector(0.f, 0.f, 30.f);
		V->ReceiveMarsDamage(AttackCopy.Damage, this, AttackCopy.DamageType, HitAt, Push);
	}), FMath::Max(0.05f, Attack.WindUp), false);
}

void ACombatantCharacter::ExecuteProjectile(const FCombatantAttack& Attack, ACombatantCharacter* Victim)
{
	UWorld* World = GetWorld();
	if (!World || !Victim)
	{
		return;
	}
	const int32 Count = FMath::Max(1, Attack.ProjectileCount);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Start = GetMuzzleLocation();
		const float Dist = static_cast<float>(FVector::Dist(Start, Victim->GetAimPoint()));
		const float Lead = Attack.ProjectileSpeed > 0.f ? Dist / Attack.ProjectileSpeed : 0.f;
		FVector Aim = Victim->GetAimPoint() + Victim->GetVelocity() * Lead * 0.7f;
		// Spread for multi-shot volleys.
		Aim += FVector(Stream.FRandRange(-60.f, 60.f), Stream.FRandRange(-60.f, 60.f), Stream.FRandRange(-20.f, 40.f)) * (Count > 1 ? 1.f : 0.4f);

		FVector Velocity;
		if (Attack.ProjectileGravityScale > 0.f)
		{
			const float GravityZ = World->GetGravityZ() * Attack.ProjectileGravityScale;
			if (!UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, Velocity, Start, Aim, GravityZ, 0.35f))
			{
				Velocity = (Aim - Start).GetSafeNormal() * Attack.ProjectileSpeed;
			}
		}
		else
		{
			Velocity = (Aim - Start).GetSafeNormal() * Attack.ProjectileSpeed;
		}

		const FTransform SpawnTM(Velocity.Rotation(), Start);
		AMarsProjectile* P = World->SpawnActorDeferred<AMarsProjectile>(AMarsProjectile::StaticClass(), SpawnTM, this, this,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (P)
		{
			P->InitProjectile(this, Attack, Velocity, Attack.HomingAcceleration > 0.f ? Victim : nullptr);
			UGameplayStatics::FinishSpawningActor(P, SpawnTM);
		}
	}
}

void ACombatantCharacter::ExecutePulse(const FCombatantAttack& Attack)
{
	const FVector Center = GetActorLocation();
	AMarsHitFlash::Spawn(GetWorld(), Center - FVector(0.f, 0.f, 60.f), Attack.Color, Attack.Radius / 50.f, 0.45f, EMarsShape::Cylinder);
	AMarsHitFlash::Spawn(GetWorld(), Center, Attack.Color, Attack.Radius / 110.f, 0.3f, EMarsShape::Sphere);

	if (const UMarsShowSubsystem* Show = GetShow())
	{
		for (ACombatantCharacter* Other : Show->GetCombatants(true))
		{
			if (Other == this)
			{
				continue;
			}
			const FVector Delta = Other->GetActorLocation() - Center;
			const float D = static_cast<float>(Delta.Size());
			if (D <= Attack.Radius)
			{
				const float Falloff = FMath::Lerp(1.f, 0.5f, D / FMath::Max(1.f, Attack.Radius));
				const FVector Push = Delta.GetSafeNormal2D() * Attack.Knockback + FVector(0.f, 0.f, Attack.Knockback * 0.6f);
				Other->ReceiveMarsDamage(Attack.Damage * Falloff, this, Attack.DamageType, Other->GetAimPoint(), Push);
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Damage and death
// ------------------------------------------------------------------------------------------------

void ACombatantCharacter::ReceiveMarsDamage(float Amount, ACombatantCharacter* Source, FGameplayTag DamageType, const FVector& HitLocation, const FVector& KnockbackVelocity)
{
	if (!IsAlive() || CombatState != ECombatantState::Fighting)
	{
		return; // no damage before "FIGHT!" or after the end
	}
	UMarsShowSubsystem* Show = GetShow();
	const float Scaled = Amount * (Show ? Show->GetDamageMultiplier() : 1.f);
	const float Applied = -Health->ApplyDelta(-Scaled);
	if (Source && Source != this)
	{
		LastAttacker = Source;
	}

	const FLinearColor HitColor = Source ? Source->GetProfile().UIColor : FLinearColor::White;
	AMarsHitFlash::Spawn(GetWorld(), HitLocation, HitColor, FMath::Clamp(Applied / 25.f, 0.6f, 2.2f), 0.3f, EMarsShape::Sphere);
	FlashHit();
	if (!KnockbackVelocity.IsNearlyZero())
	{
		KickVelocity(KnockbackVelocity, true, true);
	}
	if (Show)
	{
		Show->ReportDamage(this, Applied, GetHeadPoint(), HitColor);
	}

	if (!Health->IsAlive())
	{
		Die(Source);
	}
}

void ACombatantCharacter::FlashHit()
{
	for (const TPair<TWeakObjectPtr<UMaterialInstanceDynamic>, FLinearColor>& Pair : BodyColors)
	{
		if (UMaterialInstanceDynamic* MID = Pair.Key.Get())
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 1.f, 1.f));
		}
	}
	HitFlashTimer = 0.08f;
}

void ACombatantCharacter::Die(ACombatantCharacter* Killer)
{
	SetCombatState(ECombatantState::Dead);
	AuraLight->SetIntensity(0.f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// "Ragdoll" for a primitive body: every part becomes its own physics body and tumbles in Mars gravity.
	const FVector Center = GetActorLocation();
	for (const TPair<FName, TObjectPtr<UStaticMeshComponent>>& Pair : Parts)
	{
		UStaticMeshComponent* Part = Pair.Value.Get();
		if (!Part)
		{
			continue;
		}
		Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Part->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetSimulatePhysics(true);
		const FVector Out = (Part->GetComponentLocation() - Center).GetSafeNormal() * Stream.FRandRange(150.f, 420.f) + FVector(0.f, 0.f, Stream.FRandRange(150.f, 450.f));
		Part->AddImpulse(Out, NAME_None, /*bVelChange*/ true);
		Part->AddAngularImpulseInDegrees(FVector(Stream.FRandRange(-360.f, 360.f), Stream.FRandRange(-360.f, 360.f), Stream.FRandRange(-360.f, 360.f)), NAME_None, true);
	}

	AMarsHitFlash::Spawn(GetWorld(), Center, Profile.UIColor, 4.f, 0.6f, EMarsShape::Sphere);
	if (UMarsShowSubsystem* Show = GetShow())
	{
		Show->NotifyDeath(this, Killer);
	}
}

void ACombatantCharacter::Celebrate()
{
	SetCombatState(ECombatantState::Victorious);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	AuraLight->SetIntensity(9000.f);
	HopTimer = 0.4f;
	PlayVoice(true); // a victorious second "Hello world"
}
