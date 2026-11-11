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
	Health = FMath::Clamp(Health + Delta, 0.f, MaxHealth);
	const float Actual = Health - Old;
	if (!FMath::IsNearlyZero(Actual))
	{
		OnHealthChanged.Broadcast(this, Health, Actual);
	}
	return Actual;
}
