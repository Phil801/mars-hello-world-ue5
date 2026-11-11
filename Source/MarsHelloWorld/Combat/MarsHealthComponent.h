// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MarsHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMarsHealthChanged, UMarsHealthComponent*, HealthComponent, float, NewHealth, float, Delta);

/** Minimal attribute component (ActionRoguelike's AttributeComponent idea, cut down to health). */
UCLASS(ClassGroup = (Mars), meta = (BlueprintSpawnableComponent))
class MARSHELLOWORLD_API UMarsHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMarsHealthComponent();

	void InitHealth(float InMaxHealth);

	/** Applies a change (negative = damage). Returns the change actually applied. */
	float ApplyDelta(float Delta);

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return Health > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnMarsHealthChanged OnHealthChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float Health = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;
};
