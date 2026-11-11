// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MarsPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class AMarsShowCamera;
class ACombatantAstronaut;

UENUM()
enum class EMarsViewMode : uint8
{
	FirstPerson, // default playable view
	Director,    // attract / debug: auto show camera
	WideOrbit,
	LowAction
};

/**
 * Player controller for the Earth astronaut, plus an optional attract / debug show camera.
 * Enhanced Input actions are created in C++ at runtime so the project needs no .uasset input files.
 *
 * Play: WASD move, mouse look, LMB fire, Space jump, Left Shift sprint.
 * Meta: C cycle camera (first person / director / wide / low), P slow-mo, R restart / Respawn, H help, Esc quit.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMarsPlayerController();

	void SetShowCamera(AMarsShowCamera* Camera);
	void UseFirstPersonView();
	void AttachToShowCamera(AMarsShowCamera* Camera);

	bool IsHelpVisible() const { return bShowHelp; }
	bool IsAttractMode() const { return ViewMode != EMarsViewMode::FirstPerson; }
	EMarsViewMode GetViewMode() const { return ViewMode; }
	bool IsVictoryUi() const { return bVictoryUi; }
	void EnterVictoryUi();

	/** Mouse look scale. Applied in OnLook. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float LookSensitivity = 1.0f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SlowMoAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CameraAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HelpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> QuitAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

private:
	void EnsureInputObjects();
	ACombatantAstronaut* GetAstronaut() const;
	bool IsGameplayInputAllowed() const;

	void OnRestart();
	void OnSlowMo();
	void OnCycleCamera();
	void OnToggleHelp();
	void OnQuit();
	void OnMoveForward(const struct FInputActionValue& Value);
	void OnMoveRight(const struct FInputActionValue& Value);
	void OnLook(const struct FInputActionValue& Value);
	void OnJump();
	void OnSprintStarted();
	void OnSprintCompleted();
	void OnFireStarted();
	void OnFireCompleted();

	void ApplyViewMode();

	UPROPERTY(Transient)
	TObjectPtr<AMarsShowCamera> ShowCamera;

	EMarsViewMode ViewMode = EMarsViewMode::FirstPerson;
	bool bSlowMo = false;
	bool bShowHelp = true;
	bool bVictoryUi = false;
};
