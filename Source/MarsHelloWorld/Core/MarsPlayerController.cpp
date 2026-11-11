// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsPlayerController.h"
#include "Core/MarsGameMode.h"
#include "Camera/MarsShowCamera.h"
#include "Combat/CombatantAstronaut.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AMarsPlayerController::AMarsPlayerController()
{
	bAutoManageActiveCameraTarget = true;
	bShowMouseCursor = false;
}

void AMarsPlayerController::EnsureInputObjects()
{
	auto MakeBool = [this](TObjectPtr<UInputAction>& Action, const TCHAR* Name)
	{
		if (!Action)
		{
			Action = NewObject<UInputAction>(this, Name);
			Action->ValueType = EInputActionValueType::Boolean;
		}
	};
	auto MakeAxis = [this](TObjectPtr<UInputAction>& Action, const TCHAR* Name, EInputActionValueType Type)
	{
		if (!Action)
		{
			Action = NewObject<UInputAction>(this, Name);
			Action->ValueType = Type;
		}
	};

	MakeBool(RestartAction, TEXT("IA_Restart"));
	MakeBool(SlowMoAction, TEXT("IA_SlowMo"));
	MakeBool(CameraAction, TEXT("IA_Camera"));
	MakeBool(HelpAction, TEXT("IA_Help"));
	MakeBool(QuitAction, TEXT("IA_Quit"));
	MakeBool(JumpAction, TEXT("IA_Jump"));
	MakeBool(SprintAction, TEXT("IA_Sprint"));
	MakeBool(FireAction, TEXT("IA_Fire"));
	MakeAxis(MoveForwardAction, TEXT("IA_MoveForward"), EInputActionValueType::Axis1D);
	MakeAxis(MoveRightAction, TEXT("IA_MoveRight"), EInputActionValueType::Axis1D);
	MakeAxis(LookAction, TEXT("IA_Look"), EInputActionValueType::Axis2D);

	if (!MappingContext)
	{
		MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_MarsShow"));
		MappingContext->MapKey(RestartAction, EKeys::R);
		MappingContext->MapKey(RestartAction, EKeys::Gamepad_Special_Right);
		MappingContext->MapKey(SlowMoAction, EKeys::P);
		MappingContext->MapKey(SlowMoAction, EKeys::Gamepad_Special_Left);
		MappingContext->MapKey(CameraAction, EKeys::C);
		MappingContext->MapKey(CameraAction, EKeys::Gamepad_FaceButton_Right);
		MappingContext->MapKey(HelpAction, EKeys::H);
		MappingContext->MapKey(QuitAction, EKeys::Escape);

		MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
		MappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
		MappingContext->MapKey(SprintAction, EKeys::LeftShift);
		MappingContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
		MappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
		MappingContext->MapKey(FireAction, EKeys::Gamepad_RightTrigger);

		auto AddNegated = [this](UInputAction* Action, FKey Key)
		{
			FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(Action, Key);
			UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(MappingContext);
			Mapping.Modifiers.Add(Neg);
		};

		MappingContext->MapKey(MoveForwardAction, EKeys::W);
		MappingContext->MapKey(MoveForwardAction, EKeys::Up);
		AddNegated(MoveForwardAction, EKeys::S);
		AddNegated(MoveForwardAction, EKeys::Down);
		MappingContext->MapKey(MoveForwardAction, EKeys::Gamepad_LeftY);

		MappingContext->MapKey(MoveRightAction, EKeys::D);
		MappingContext->MapKey(MoveRightAction, EKeys::Right);
		AddNegated(MoveRightAction, EKeys::A);
		AddNegated(MoveRightAction, EKeys::Left);
		MappingContext->MapKey(MoveRightAction, EKeys::Gamepad_LeftX);

		MappingContext->MapKey(LookAction, EKeys::Mouse2D);
		MappingContext->MapKey(LookAction, EKeys::Gamepad_RightX);
		{
			FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(LookAction, EKeys::Gamepad_RightY);
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(MappingContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(MappingContext);
			Mapping.Modifiers.Add(Swizzle);
			Mapping.Modifiers.Add(Neg); // typical invert-Y for a stick
		}
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
		EIC->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AMarsPlayerController::OnMoveForward);
		EIC->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AMarsPlayerController::OnMoveRight);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMarsPlayerController::OnLook);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnJump);
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnSprintStarted);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMarsPlayerController::OnSprintCompleted);
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AMarsPlayerController::OnFireStarted);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AMarsPlayerController::OnFireCompleted);
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
		if (const AMarsGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMarsGameMode>() : nullptr)
		{
			SetShowCamera(GM->GetShowCamera());
		}
	}
	if (ViewMode == EMarsViewMode::FirstPerson)
	{
		UseFirstPersonView();
	}
}

void AMarsPlayerController::SetShowCamera(AMarsShowCamera* Camera)
{
	ShowCamera = Camera;
}

void AMarsPlayerController::UseFirstPersonView()
{
	ViewMode = EMarsViewMode::FirstPerson;
	ApplyViewMode();
}

void AMarsPlayerController::AttachToShowCamera(AMarsShowCamera* Camera)
{
	SetShowCamera(Camera);
	ViewMode = EMarsViewMode::Director;
	ApplyViewMode();
}

void AMarsPlayerController::ApplyViewMode()
{
	ACombatantAstronaut* Astro = GetAstronaut();
	const bool bFP = ViewMode == EMarsViewMode::FirstPerson && !bVictoryUi;

	bAutoManageActiveCameraTarget = bFP;
	if (Astro)
	{
		Astro->SetFirstPersonVisible(bFP);
	}

	if (bFP && Astro)
	{
		SetViewTargetWithBlend(Astro, 0.2f);
	}
	else if (ShowCamera)
	{
		if (ViewMode == EMarsViewMode::WideOrbit)
		{
			ShowCamera->SetMode(EMarsCameraMode::WideOrbit);
		}
		else if (ViewMode == EMarsViewMode::LowAction)
		{
			ShowCamera->SetMode(EMarsCameraMode::LowAction);
		}
		else
		{
			ShowCamera->SetMode(EMarsCameraMode::Director);
		}
		SetViewTargetWithBlend(ShowCamera, 0.3f);
	}
}

ACombatantAstronaut* AMarsPlayerController::GetAstronaut() const
{
	return Cast<ACombatantAstronaut>(GetPawn());
}

bool AMarsPlayerController::IsGameplayInputAllowed() const
{
	return ViewMode == EMarsViewMode::FirstPerson && !bVictoryUi;
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
	switch (ViewMode)
	{
	case EMarsViewMode::FirstPerson: ViewMode = EMarsViewMode::Director; break;
	case EMarsViewMode::Director:    ViewMode = EMarsViewMode::WideOrbit; break;
	case EMarsViewMode::WideOrbit:   ViewMode = EMarsViewMode::LowAction; break;
	default:                         ViewMode = EMarsViewMode::FirstPerson; break;
	}
	ApplyViewMode();
}

void AMarsPlayerController::OnToggleHelp()
{
	bShowHelp = !bShowHelp;
}

void AMarsPlayerController::OnQuit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AMarsPlayerController::OnMoveForward(const FInputActionValue& Value)
{
	if (IsGameplayInputAllowed())
	{
		if (ACombatantAstronaut* A = GetAstronaut())
		{
			A->MoveForward(Value.Get<float>());
		}
	}
}

void AMarsPlayerController::OnMoveRight(const FInputActionValue& Value)
{
	if (IsGameplayInputAllowed())
	{
		if (ACombatantAstronaut* A = GetAstronaut())
		{
			A->MoveRight(Value.Get<float>());
		}
	}
}

void AMarsPlayerController::OnLook(const FInputActionValue& Value)
{
	if (!IsGameplayInputAllowed())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	if (ACombatantAstronaut* A = GetAstronaut())
	{
		A->AddLook(Axis);
	}
}

void AMarsPlayerController::OnJump()
{
	if (IsGameplayInputAllowed())
	{
		if (ACombatantAstronaut* A = GetAstronaut())
		{
			A->OnJump();
		}
	}
}

void AMarsPlayerController::OnSprintStarted()
{
	if (ACombatantAstronaut* A = GetAstronaut())
	{
		A->StartSprint();
	}
}

void AMarsPlayerController::OnSprintCompleted()
{
	if (ACombatantAstronaut* A = GetAstronaut())
	{
		A->StopSprint();
	}
}

void AMarsPlayerController::OnFireStarted()
{
	if (IsGameplayInputAllowed())
	{
		if (ACombatantAstronaut* A = GetAstronaut())
		{
			A->StartFire();
		}
	}
}

void AMarsPlayerController::OnFireCompleted()
{
	if (ACombatantAstronaut* A = GetAstronaut())
	{
		A->StopFire();
	}
}

void AMarsPlayerController::EnterVictoryUi()
{
	bVictoryUi = true;
	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	if (ACombatantAstronaut* A = GetAstronaut())
	{
		A->StopFire();
		A->StopSprint();
	}
}
