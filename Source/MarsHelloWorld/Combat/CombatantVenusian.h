// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantCharacter.h"
#include "CombatantVenusian.generated.h"

UCLASS()
class MARSHELLOWORLD_API ACombatantVenusian : public ACombatantCharacter
{
	GENERATED_BODY()

public:
	ACombatantVenusian();

protected:
	virtual void BuildBody() override;
	virtual void AnimateBody(float DeltaSeconds) override;
	virtual void OnGreetingFlourish() override;
	virtual void OnAttackFired(const FCombatantAttack& Attack) override;
	virtual FVector GetMuzzleLocation() const override;

private:
	float FlourishTime = -10.f;
	float AttackPoseTime = -10.f;
};
