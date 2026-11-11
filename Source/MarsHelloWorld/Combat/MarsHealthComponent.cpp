// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Combat/MarsHealthComponent.h"

UMarsHealthComponent::UMarsHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMarsHealthComponent::InitHealth(float InMaxHealth)
{
	MaxHealth = FMath::Max(1.f, InMaxHealth);
	Health = MaxHealth;
	OnHealthChanged.Broadcast(this, Health, 0.f);
}

float UMarsHealthComponent::ApplyDelta(float Delta)
{
	if (!IsAlive() && Delta < 0.f)
	{
		return 0.f;
	}
	const float Old = Health;
	const float Floor = bImmortal ? MinHealth : 0.f;
	Health = FMath::Clamp(Health + Delta, Floor, MaxHealth);
	const float Actual = Health - Old;
	if (!FMath::IsNearlyZero(Actual))
	{
		OnHealthChanged.Broadcast(this, Health, Actual);
	}
	return Actual;
}

void UMarsHealthComponent::SetImmortal(bool bInImmortal, float InMinHealth)
{
	bImmortal = bInImmortal;
	MinHealth = FMath::Max(0.f, InMinHealth);
	if (bImmortal && Health < MinHealth)
	{
		Health = MinHealth;
	}
}
