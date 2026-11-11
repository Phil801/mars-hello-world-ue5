// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/CombatantTypes.h"
#include "MarsProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class ACombatantCharacter;

/** Acid blobs, laser bolts and plasma orbs: one class, configured by FCombatantAttack. */
UCLASS()
class MARSHELLOWORLD_API AMarsProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMarsProjectile();

	/** Call between SpawnActorDeferred and FinishSpawning. */
	void InitProjectile(ACombatantCharacter* InShooter, const FCombatantAttack& InAttack, const FVector& InVelocity, ACombatantCharacter* HomingTarget);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UPointLightComponent> Glow;

private:
	void Impact(const FVector& Where, ACombatantCharacter* Victim);

	UPROPERTY(Transient)
	TObjectPtr<ACombatantCharacter> Shooter;

	FCombatantAttack Attack;
	bool bSpent = false;
};
