// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantCharacter.h"
#include "CombatantAstronaut.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Playable Earth astronaut: first-person camera, primitive arms and rifle, regenerating
 * shield, and they cannot die. The show camera remains available as an attract / debug view.
 */
UCLASS()
class MARSHELLOWORLD_API ACombatantAstronaut : public ACombatantCharacter
{
	GENERATED_BODY()

public:
	ACombatantAstronaut();

	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsPlayerAstronaut() const override { return true; }

	virtual void StartFighting() override;
	virtual void StartFightingAt(ACombatantCharacter* InitialTarget, bool bPinTarget, bool bReadyToFire = false) override;
	virtual void Celebrate() override;
	virtual void ReceiveMarsDamage(float Amount, ACombatantCharacter* Source, FGameplayTag DamageType, const FVector& HitLocation, const FVector& KnockbackVelocity) override;

	void PrepareAsPlayer(const FVector& InArenaCenter, int32 InSeed);
	void SetFirstPersonVisible(bool bFirstPerson);

	void MoveForward(float Value);
	void MoveRight(float Value);
	void AddLook(FVector2D Axis);
	void StartSprint();
	void StopSprint();
	void OnJump();
	void StartFire();
	void StopFire();

	UFUNCTION(BlueprintPure, Category = "Astronaut")
	float GetShield() const { return Shield; }

	UFUNCTION(BlueprintPure, Category = "Astronaut")
	float GetMaxShield() const { return MaxShield; }

	UFUNCTION(BlueprintPure, Category = "Astronaut")
	float GetShieldFraction() const { return MaxShield > 0.f ? Shield / MaxShield : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Astronaut")
	float GetShieldFlashAlpha() const { return ShieldFlash; }

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void BuildBody() override;
	virtual void AnimateBody(float DeltaSeconds) override;
	virtual void OnGreetingFlourish() override;
	virtual void OnAttackFired(const FCombatantAttack& Attack) override;
	virtual FVector GetMuzzleLocation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astronaut")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astronaut")
	TObjectPtr<USceneComponent> ArmsRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astronaut")
	TObjectPtr<UStaticMeshComponent> ShieldBubble;

private:
	void TickShield(float DeltaSeconds);
	void FireWeapon();
	const FCombatantAttack* GetRifleAttack() const;
	float Shield = 150.f;
	float MaxShield = 150.f;
	float ShieldRegenPerSecond = 35.f;
	float ShieldRegenDelay = 2.8f;
	float TimeSinceHit = 100.f;
	float ShieldFlash = 0.f;
	float FireCooldown = 0.f;
	float Recoil = 0.f;
	float WalkBob = 0.f;
	float SprintMultiplier = 1.65f;
	bool bSprinting = false;
	bool bWantsFire = false;
	bool bFirstPerson = true;
	float FlourishTime = -10.f;
	float AttackPoseTime = -10.f;
};
