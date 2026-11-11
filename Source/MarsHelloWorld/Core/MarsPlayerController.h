// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MarsPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class AMarsShowCamera;

/**
 * Spectator controller. Enhanced Input actions are created in C++ at runtime so the project needs
 * no .uasset input files: R restart, Space slow-motion, C camera mode, H help, Esc quit.
 * (Swap-in hook: assign real UInputAction / UInputMappingContext assets in a Blueprint subclass.)
 */
UCLASS()
class MARSHELLOWORLD_API AMarsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMarsPlayerController();

	void AttachToShowCamera(AMarsShowCamera* Camera);

	bool IsHelpVisible() const { return bShowHelp; }

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

private:
	void EnsureInputObjects();
	void OnRestart();
	void OnSlowMo();
	void OnCycleCamera();
	void OnToggleHelp();
	void OnQuit();

	UPROPERTY(Transient)
	TObjectPtr<AMarsShowCamera> ShowCamera;

	bool bSlowMo = false;
	bool bShowHelp = true;
};
