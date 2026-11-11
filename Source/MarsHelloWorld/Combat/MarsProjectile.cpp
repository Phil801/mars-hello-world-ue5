// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/MarsProjectile.h"
#include "Combat/CombatantCharacter.h"
#include "Combat/MarsHitFlash.h"
#include "World/MarsAssets.h"
#include "Core/MarsGameplayTags.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AMarsProjectile::AMarsProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 6.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(18.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetNotifyRigidBodyCollision(true);
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 0.f; // velocity is supplied by InitProjectile
	Movement->MaxSpeed = 0.f;     // no clamp
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.f;

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->IntensityUnits = ELightUnits::Unitless;
	Glow->Intensity = 5000.f;
	Glow->AttenuationRadius = 350.f;
	Glow->CastShadows = false;
}

void AMarsProjectile::InitProjectile(ACombatantCharacter* InShooter, const FCombatantAttack& InAttack, const FVector& InVelocity, ACombatantCharacter* HomingTarget)
{
	Shooter = InShooter;
	Attack = InAttack;

	Collision->SetSphereRadius(Attack.ProjectileRadius);
	if (Shooter)
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}

	Movement->Velocity = InVelocity;
	Movement->ProjectileGravityScale = Attack.ProjectileGravityScale;
	if (HomingTarget && Attack.HomingAcceleration > 0.f)
	{
		Movement->bIsHomingProjectile = true;
		Movement->HomingAccelerationMagnitude = Attack.HomingAcceleration;
		Movement->HomingTargetComponent = HomingTarget->GetRootComponent();
	}

	Glow->SetLightColor(Attack.Color);

	// Lasers are long thin bolts; everything else is a glowing ball.
	const bool bBolt = Attack.DamageType.MatchesTagExact(MarsTags::Damage_Laser);
	const float D = Attack.ProjectileRadius / 50.f; // basic sphere is 100 cm across
	MarsAssets::AddShape(this, Collision, TEXT("Visual"), bBolt ? EMarsShape::Cylinder : EMarsShape::Sphere, FVector::ZeroVector,
		bBolt ? FRotator(90.f, 0.f, 0.f) : FRotator::ZeroRotator,
		bBolt ? FVector(D * 0.5f, D * 0.5f, D * 4.f) : FVector(D),
		Attack.Color, /*Glow*/ 10.f);
}

void AMarsProjectile::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AMarsProjectile::OnOverlap);
	Collision->OnComponentHit.AddDynamic(this, &AMarsProjectile::OnHit);
}

void AMarsProjectile::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACombatantCharacter* Victim = Cast<ACombatantCharacter>(OtherActor);
	if (!Victim || Victim == Shooter || !Victim->IsAlive())
	{
		return;
	}
	Impact(bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation(), Victim);
}

void AMarsProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == Shooter)
	{
		return;
	}
	Impact(Hit.ImpactPoint, Cast<ACombatantCharacter>(OtherActor));
}

void AMarsProjectile::Impact(const FVector& Where, ACombatantCharacter* Victim)
{
	if (bSpent)
	{
		return;
	}
	bSpent = true;

	if (Victim && Victim->IsAlive())
	{
		const FVector Push = Movement->Velocity.GetSafeNormal2D() * Attack.Knockback + FVector(0.f, 0.f, Attack.Knockback * 0.3f);
		Victim->ReceiveMarsDamage(Attack.Damage, Shooter, Attack.DamageType, Where, Push);
	}
	else
	{
		// Splash on the ground / rocks.
		AMarsHitFlash::Spawn(GetWorld(), Where, Attack.Color, 0.9f, 0.35f, EMarsShape::Sphere);
	}
	Destroy();
}
