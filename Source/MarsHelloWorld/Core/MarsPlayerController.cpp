// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsPlayerController.h"
#include "Core/MarsGameMode.h"
#include "Camera/MarsShowCamera.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AMarsPlayerController::AMarsPlayerController()
{
	bAutoManageActiveCameraTarget = false; // we pick the view target (the show camera)
	bShowMouseCursor = false;
}

void AMarsPlayerController::EnsureInputObjects()
{
	auto MakeAction = [this](TObjectPtr<UInputAction>& Action, const TCHAR* Name)
	{
		if (!Action)
		{
			Action = NewObject<UInputAction>(this, Name);
			Action->ValueType = EInputActionValueType::Boolean;
		}
	};
	MakeAction(RestartAction, TEXT("IA_Restart"));
	MakeAction(SlowMoAction, TEXT("IA_SlowMo"));
	MakeAction(CameraAction, TEXT("IA_Camera"));
	MakeAction(HelpAction, TEXT("IA_Help"));
	MakeAction(QuitAction, TEXT("IA_Quit"));

	if (!MappingContext)
	{
		MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_MarsShow"));
		MappingContext->MapKey(RestartAction, EKeys::R);
		MappingContext->MapKey(RestartAction, EKeys::Gamepad_Special_Right);
		MappingContext->MapKey(SlowMoAction, EKeys::SpaceBar);
		MappingContext->MapKey(SlowMoAction, EKeys::Gamepad_FaceButton_Bottom);
		MappingContext->MapKey(CameraAction, EKeys::C);
		MappingContext->MapKey(CameraAction, EKeys::Gamepad_FaceButton_Right);
		MappingContext->MapKey(HelpAction, EKeys::H);
		MappingContext->MapKey(QuitAction, EKeys::Escape);
	}
}

void AMarsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	EnsureInputObjects();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(RestartAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnRestart);
		EIC->BindAction(SlowMoAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnSlowMo);
		EIC->BindAction(CameraAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnCycleCamera);
		EIC->BindAction(HelpAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnToggleHelp);
		EIC->BindAction(QuitAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnQuit);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Enhanced Input is not the default input component; check Config/DefaultInput.ini"));
	}
}

void AMarsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		EnsureInputObjects();
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
		SetInputMode(FInputModeGameOnly());
	}

	if (!ShowCamera)
	{
		if (const AMarsGameMode* GM = GetWorld()->GetAuthGameMode<AMarsGameMode>())
		{
			AttachToShowCamera(GM->GetShowCamera());
		}
	}
}

void AMarsPlayerController::AttachToShowCamera(AMarsShowCamera* Camera)
{
	if (Camera)
	{
		ShowCamera = Camera;
		SetViewTargetWithBlend(Camera, 0.f);
	}
}

void AMarsPlayerController::OnRestart()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	ConsoleCommand(TEXT("RestartLevel"));
}

void AMarsPlayerController::OnSlowMo()
{
	bSlowMo = !bSlowMo;
	UGameplayStatics::SetGlobalTimeDilation(this, bSlowMo ? 0.3f : 1.f);
}

void AMarsPlayerController::OnCycleCamera()
{
	if (ShowCamera)
	{
		ShowCamera->CycleMode();
	}
}

void AMarsPlayerController::OnToggleHelp()
{
	bShowHelp = !bShowHelp;
}

void AMarsPlayerController::OnQuit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
