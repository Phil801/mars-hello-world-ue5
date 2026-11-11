// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Combat/CombatantTypes.h"
#include "World/MarsAssets.h"
#include "CombatantCharacter.generated.h"

class UMarsHealthComponent;
class UMarsShowSubsystem;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * Base for the Martian, X-Bot and Venusian.
 *
 * - Body: built at runtime from engine basic shapes (BuildBody) so it runs with zero art.
 *   Swap-in hook: give a Blueprint subclass a SkeletalMesh + AnimBP on the inherited Mesh
 *   component and untick bUsePrimitiveBody.
 * - Brain: a small C++ state machine (Entering -> Greeting -> Ready -> Fighting -> Dead/Victorious),
 *   mirrored as a gameplay tag (CurrentStateTag) so a StateTree can take over later.
 *   Greeting now means "holding for a conversation line", not a one-shot hello.
 * - Data: everything tunable is in FCombatantProfile (or a UCombatantDefinition data asset).
 */
UCLASS(Abstract)
class MARSHELLOWORLD_API ACombatantCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACombatantCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// --- Director API (called by AMarsGameMode / UMarsConversationDirector) --------------------
	void LaunchEntrance(const FVector& InArrivalPoint, const FVector& InArenaCenter, int32 InSeed);
	/** Play a table line with a HUD subtitle. Optional hold in the Greeting state until it ends.
	 *  Returns the hold time in seconds (audio duration + a short tail) so the director can time the next cue
	 *  even if this actor is destroyed mid-line. */
	float SpeakLine(FName LineId, ACombatantCharacter* AddressTo, bool bHoldForConversation, bool bFlourish);
	void StartFighting();
	void StartFightingAt(ACombatantCharacter* InitialTarget, bool bPinTarget, bool bReadyToFire = false);
	void ClearTargetPin();
	void Celebrate();
	/** Safety net: teleport to the arrival point if something blocked the entrance. */
	void ForceArrive();

	/** Applies damage from another combatant (or the world when Source is null). */
	void ReceiveMarsDamage(float Amount, ACombatantCharacter* Source, FGameplayTag DamageType, const FVector& HitLocation, const FVector& KnockbackVelocity);

	// --- Queries --------------------------------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Combatant")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "Combatant")
	ECombatantState GetCombatState() const { return CombatState; }

	UFUNCTION(BlueprintPure, Category = "Combatant")
	FGameplayTag GetCurrentStateTag() const { return CurrentStateTag; }

	const FCombatantProfile& GetProfile() const { return Profile; }
	UMarsHealthComponent* GetHealth() const { return Health; }

	/** Point projectiles aim at (middle of the body). */
	FVector GetAimPoint() const;

	/** Where the HUD draws the health bar. */
	FVector GetHeadPoint() const;

protected:
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;

	/** Subclasses add their primitive body parts here (use AddPart). */
	virtual void BuildBody() {}

	/** Procedural animation every frame (bob, limb swing, thruster flicker...). */
	virtual void AnimateBody(float DeltaSeconds) {}

	/** Little character moment while saying hello. */
	virtual void OnGreetingFlourish() {}

	/** Called when an attack fires (for muzzle flashes etc.). */
	virtual void OnAttackFired(const FCombatantAttack& Attack) {}

	UStaticMeshComponent* AddPart(FName Name, EMarsShape Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale,
		const FLinearColor& Color, float Glow = 0.f, USceneComponent* Parent = nullptr);

	UStaticMeshComponent* GetPart(FName Name) const;

	/** Where projectiles come out (component-space offset from the actor, rotated with it). */
	virtual FVector GetMuzzleLocation() const;

	float GetAge() const { return Age; }

	/** Velocity kick that respects flying: LaunchCharacter would drop a hovering Venusian into Falling. */
	void KickVelocity(const FVector& DeltaVelocity, bool bOverrideXY, bool bOverrideZ);
	float GetMoveSpeedFraction() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combatant")
	FCombatantProfile Profile;

	/** Optional designer override; replaces Profile at BeginPlay when set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combatant")
	TObjectPtr<UCombatantDefinition> DefinitionOverride;

	/** Turn off when you give this character a real skeletal mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combatant")
	bool bUsePrimitiveBody = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combatant")
	TObjectPtr<USceneComponent> BodyRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combatant")
	TObjectPtr<UMarsHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combatant")
	TObjectPtr<UPointLightComponent> AuraLight;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY(Transient)
	TObjectPtr<ACombatantCharacter> Target;

	UPROPERTY(Transient)
	TObjectPtr<ACombatantCharacter> LastAttacker;

	/** Who this combatant is addressing during a conversation line (face them). */
	TWeakObjectPtr<ACombatantCharacter> AddressTarget;

	/** When true, ChooseTarget will not drop Target until it dies or ClearTargetPin is called. */
	bool bTargetPinned = false;

	FRandomStream Stream;

private:
	void SetCombatState(ECombatantState NewState);
	UMarsShowSubsystem* GetShow() const;

	void TickEntering(float DeltaSeconds);
	void TickFighting(float DeltaSeconds);
	void TickHover(float DeltaSeconds);
	void ArriveNow();

	void ChooseTarget();
	bool TryAttack();
	void ExecuteAttack(const FCombatantAttack& Attack);
	void ExecuteMelee(const FCombatantAttack& Attack, ACombatantCharacter* Victim);
	void ExecuteProjectile(const FCombatantAttack& Attack, ACombatantCharacter* Victim);
	void ExecutePulse(const FCombatantAttack& Attack);

	void FlashHit();
	void Die(ACombatantCharacter* Killer);

	ECombatantState CombatState = ECombatantState::Entering;
	FGameplayTag CurrentStateTag;

	FVector ArrivalPoint = FVector::ZeroVector;
	FVector ArenaCenter = FVector::ZeroVector;
	bool bLandingFromFlight = false;

	TArray<float> AttackCooldowns;
	float GlobalAttackLockout = 0.f;
	float RetargetTimer = 0.f;
	float StrafeTimer = 0.f;
	float StrafeSign = 1.f;
	float ChargeTimer = 0.f;
	float HopTimer = 0.f;
	float HitFlashTimer = 0.f;
	float Age = 0.f;

	/** Original colours so the white hit-flash can be undone. */
	TArray<TPair<TWeakObjectPtr<UMaterialInstanceDynamic>, FLinearColor>> BodyColors;
};
