// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "UI/MarsHUD.h"
#include "Core/MarsShowSubsystem.h"
#include "Core/MarsPlayerController.h"
#include "Camera/MarsShowCamera.h"
#include "Combat/CombatantCharacter.h"
#include "Combat/CombatantAstronaut.h"
#include "Combat/MarsHealthComponent.h"
#include "InputCoreTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

float AMarsHUD::UIScale() const
{
	return Canvas ? FMath::Max(0.5f, Canvas->ClipY / 1080.f) : 1.f;
}

void AMarsHUD::DrawShadowedText(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, bool bCenterX)
{
	if (!Canvas || Text.IsEmpty())
	{
		return;
	}
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	const float DrawX = bCenterX ? X - W * 0.5f : X;
	const float Off = FMath::Max(1.f, 2.f * Scale * 0.5f);
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.85f), DrawX + Off, Y + Off, Font, Scale);
	DrawText(Text, Color, DrawX, Y, Font, Scale);
}

void AMarsHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	UWorld* World = GetWorld();
	UMarsShowSubsystem* Show = World ? World->GetSubsystem<UMarsShowSubsystem>() : nullptr;
	if (!Show)
	{
		return;
	}
	Show->PruneExpired();
	const float Now = World->GetTimeSeconds();

	DrawHealthBars(Show);
	DrawPlayerHud(Show);
	DrawFloatingTexts(Show, Now);
	DrawCaption(Show, Now);
	DrawBanner(Show, Now);
	DrawStatus(Show);
	DrawKillFeed(Show);
	DrawRespawnButton(Show);
	DrawHelp();
}

void AMarsHUD::DrawBanner(UMarsShowSubsystem* Show, float Now)
{
	const FMarsCaption& Banner = Show->GetBanner();
	if (Banner.Line.IsEmpty() || Now > Banner.ExpireTime)
	{
		return;
	}
	const float S = UIScale();
	// Fade out over the final 0.4 s.
	const float Remaining = Banner.ExpireTime - Now;
	const float Fade = FMath::Clamp(Remaining / 0.4f, 0.f, 1.f);
	FLinearColor C = Banner.Color;
	C.A = Fade;
	UFont* Font = GEngine->GetLargeFont();
	DrawShadowedText(Banner.Line.ToString(), C, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.24f, Font, Show->GetBannerScale() * 0.6f * S, true);
}

void AMarsHUD::DrawCaption(UMarsShowSubsystem* Show, float Now)
{
	const FMarsCaption& Cap = Show->GetCaption();
	if (Cap.Line.IsEmpty() || Now > Cap.ExpireTime)
	{
		return;
	}
	const float S = UIScale();
	UFont* Font = GEngine->GetLargeFont();
	const FString Speaker = Cap.Speaker.ToString() + TEXT(":");
	const FString Line = FString::Printf(TEXT("\"%s\""), *Cap.Line.ToString());
	float Scale = 1.6f * S;

	float SW = 0.f, SH = 0.f, LW = 0.f, LH = 0.f;
	GetTextSize(Speaker + TEXT(" "), SW, SH, Font, Scale);
	GetTextSize(Line, LW, LH, Font, Scale);
	float Total = SW + LW;
	const float MaxWidth = Canvas->ClipX * 0.9f;
	if (Total > MaxWidth && Total > 1.f)
	{
		Scale *= MaxWidth / Total;
		GetTextSize(Speaker + TEXT(" "), SW, SH, Font, Scale);
		GetTextSize(Line, LW, LH, Font, Scale);
		Total = SW + LW;
	}
	const float X = Canvas->ClipX * 0.5f - Total * 0.5f;
	const float Y = Canvas->ClipY * 0.82f;
	const float Pad = 14.f * S;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X - Pad, Y - Pad * 0.6f, Total + Pad * 2.f, FMath::Max(SH, LH) + Pad * 1.2f);
	DrawShadowedText(Speaker, Cap.Color, X, Y, Font, Scale);
	DrawShadowedText(Line, FLinearColor::White, X + SW, Y, Font, Scale);
}

void AMarsHUD::DrawHealthBars(UMarsShowSubsystem* Show)
{
	if (!PlayerOwner)
	{
		return;
	}
	const EMarsShowPhase Phase = Show->GetPhase();
	if (Phase == EMarsShowPhase::Arrivals)
	{
		return;
	}
	const float S = UIScale();
	UFont* Font = GEngine->GetSmallFont();
	for (ACombatantCharacter* C : Show->GetCombatants(false))
	{
		if (C->IsPlayerAstronaut())
		{
			continue; // first-person / Respawn HUD instead
		}
		const UMarsHealthComponent* HC = C->GetHealth();
		if (!HC)
		{
			continue;
		}
		FVector2D Screen;
		if (!PlayerOwner->ProjectWorldLocationToScreen(C->GetHeadPoint() + FVector(0.f, 0.f, 70.f), Screen, false))
		{
			continue;
		}
		const float W = 130.f * S;
		const float H = 11.f * S;
		const float X = Screen.X - W * 0.5f;
		const float Y = Screen.Y;
		const float Frac = HC->GetHealthFraction();
		const FLinearColor UI = C->GetProfile().UIColor;
		const FLinearColor Fill = Frac > 0.3f ? UI : FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.1f, 0.05f), UI, Frac / 0.3f);

		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 2.f, Y - 2.f, W + 4.f, H + 4.f);
		DrawRect(Fill, X, Y, W * Frac, H);
		const FString Label = C->IsAlive()
			? FString::Printf(TEXT("%s  %d"), *C->GetProfile().DisplayName.ToString(), FMath::CeilToInt(HC->GetHealth()))
			: FString::Printf(TEXT("%s  (destroyed)"), *C->GetProfile().DisplayName.ToString());
		DrawShadowedText(Label, C->IsAlive() ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.6f), Screen.X, Y - 18.f * S, Font, 1.1f * S, true);
	}
}

void AMarsHUD::DrawFloatingTexts(UMarsShowSubsystem* Show, float Now)
{
	if (!PlayerOwner)
	{
		return;
	}
	const float S = UIScale();
	UFont* Font = GEngine->GetMediumFont();
	for (const FMarsFloatingText& FT : Show->GetFloatingTexts())
	{
		const float Age = Now - FT.SpawnTime;
		const float T = FMath::Clamp(Age / FMath::Max(0.05f, FT.Lifetime), 0.f, 1.f);
		FVector2D Screen;
		if (!PlayerOwner->ProjectWorldLocationToScreen(FT.WorldLocation + FVector(0.f, 0.f, 60.f + 140.f * T), Screen, false))
		{
			continue;
		}
		FLinearColor C = FT.Color;
		C.A = 1.f - T * T;
		const float Pop = 1.f + 0.6f * FMath::Max(0.f, 1.f - Age * 8.f);
		DrawShadowedText(FT.Text, C, Screen.X, Screen.Y, Font, FT.Scale * Pop * 1.3f * S, true);
	}
}

void AMarsHUD::DrawStatus(UMarsShowSubsystem* Show)
{
	const float S = UIScale();
	UFont* Small = GEngine->GetSmallFont();
	UFont* Medium = GEngine->GetMediumFont();
	const float X = 24.f * S;
	float Y = 20.f * S;

	DrawShadowedText(TEXT("MARS HELLO WORLD"), FLinearColor(1.f, 0.55f, 0.25f), X, Y, Medium, 1.2f * S);
	Y += 30.f * S;

	FString PhaseText;
	switch (Show->GetPhase())
	{
	case EMarsShowPhase::Arrivals: PhaseText = TEXT("Arrivals"); break;
	case EMarsShowPhase::Conversation: PhaseText = TEXT("Looking for Kevin"); break;
	case EMarsShowPhase::Accusation: PhaseText = TEXT("Accusation"); break;
	case EMarsShowPhase::Fight: PhaseText = FString::Printf(TEXT("Fight  %0.1fs"), Show->GetFightElapsed()); break;
	case EMarsShowPhase::Victory: PhaseText = TEXT("Victory"); break;
	}
	DrawShadowedText(PhaseText, FLinearColor::White, X, Y, Small, 1.2f * S);
	Y += 22.f * S;

	const float G = GetWorld() ? -GetWorld()->GetGravityZ() / 100.f : 3.71f;
	DrawShadowedText(FString::Printf(TEXT("Gravity %.2f m/s^2 (Mars)"), G), FLinearColor(0.85f, 0.75f, 0.65f), X, Y, Small, 1.0f * S);
	Y += 20.f * S;

	if (Show->IsSuddenDeath() && Show->GetPhase() == EMarsShowPhase::Fight)
	{
		const float Pulse = 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f);
		DrawShadowedText(FString::Printf(TEXT("SUDDEN DEATH  x%.1f damage"), Show->GetDamageMultiplier()), FLinearColor(1.f, 0.2f, 0.1f, Pulse), X, Y, Medium, 1.1f * S);
	}
}

void AMarsHUD::DrawKillFeed(UMarsShowSubsystem* Show)
{
	const float S = UIScale();
	UFont* Font = GEngine->GetSmallFont();
	float Y = 20.f * S;
	for (const FMarsCaption& K : Show->GetKillFeed())
	{
		const FString Line = K.Line.ToString(); // already "<Killer> defeated <Victim>"
		float W = 0.f, H = 0.f;
		GetTextSize(Line, W, H, Font, 1.2f * S);
		DrawShadowedText(Line, K.Color, Canvas->ClipX - W - 24.f * S, Y, Font, 1.2f * S);
		Y += 24.f * S;
	}
}

void AMarsHUD::DrawPlayerHud(UMarsShowSubsystem* Show)
{
	const AMarsPlayerController* PC = Cast<AMarsPlayerController>(PlayerOwner);
	ACombatantAstronaut* Astro = PC ? Cast<ACombatantAstronaut>(PC->GetPawn()) : nullptr;
	if (!Astro || !Astro->GetHealth())
	{
		return;
	}
	const float S = UIScale();

	if (PC && !PC->IsAttractMode())
	{
		// Crosshair.
		const float Cx = Canvas->ClipX * 0.5f;
		const float Cy = Canvas->ClipY * 0.5f;
		const float Arm = 8.f * S;
		const float Gap = 4.f * S;
		const FLinearColor Cross(1.f, 1.f, 1.f, 0.85f);
		DrawRect(Cross, Cx - Arm - Gap, Cy - 1.f * S, Arm, 2.f * S);
		DrawRect(Cross, Cx + Gap, Cy - 1.f * S, Arm, 2.f * S);
		DrawRect(Cross, Cx - 1.f * S, Cy - Arm - Gap, 2.f * S, Arm);
		DrawRect(Cross, Cx - 1.f * S, Cy + Gap, 2.f * S, Arm);

		// Shield impact flash.
		if (Astro->GetShieldFlashAlpha() > 0.02f)
		{
			FLinearColor Flash(0.35f, 0.85f, 1.f, Astro->GetShieldFlashAlpha() * 0.28f);
			DrawRect(Flash, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		}
	}

	// Suit integrity + shield, bottom left.
	const float X = 24.f * S;
	const float Y = Canvas->ClipY - 110.f * S;
	const float W = 260.f * S;
	const float H = 14.f * S;
	DrawShadowedText(TEXT("SUIT"), FLinearColor(0.9f, 0.9f, 0.9f), X, Y - 20.f * S, GEngine->GetSmallFont(), 1.0f * S);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X - 2.f, Y - 2.f, W + 4.f, H + 4.f);
	DrawRect(FLinearColor(0.85f, 0.2f, 0.15f), X, Y, W * Astro->GetHealth()->GetHealthFraction(), H);

	const float Ys = Y + 28.f * S;
	DrawShadowedText(TEXT("SHIELD"), FLinearColor(0.5f, 0.9f, 1.f), X, Ys - 20.f * S, GEngine->GetSmallFont(), 1.0f * S);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X - 2.f, Ys - 2.f, W + 4.f, H + 4.f);
	DrawRect(FLinearColor(0.3f, 0.8f, 1.f), X, Ys, W * Astro->GetShieldFraction(), H);
}

void AMarsHUD::DrawRespawnButton(UMarsShowSubsystem* Show)
{
	if (!Show || Show->GetPhase() != EMarsShowPhase::Victory || !PlayerOwner)
	{
		bRespawnMouseWasDown = false;
		return;
	}
	const float S = UIScale();
	const FString Label = TEXT("Respawn");
	UFont* Font = GEngine->GetMediumFont();
	const float Scale = 1.8f * S;
	float TW = 0.f, TH = 0.f;
	GetTextSize(Label, TW, TH, Font, Scale);
	const float PadX = 36.f * S;
	const float PadY = 16.f * S;
	const float W = TW + PadX * 2.f;
	const float H = TH + PadY * 2.f;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	const float Y = Canvas->ClipY * 0.62f;

	float MX = 0.f, MY = 0.f;
	const bool bHaveMouse = PlayerOwner->GetMousePosition(MX, MY);
	const bool bHover = bHaveMouse && MX >= X && MX <= X + W && MY >= Y && MY <= Y + H;
	const FLinearColor Fill = bHover ? FLinearColor(0.95f, 0.55f, 0.15f, 0.95f) : FLinearColor(0.8f, 0.4f, 0.08f, 0.9f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), X - 3.f, Y - 3.f, W + 6.f, H + 6.f);
	DrawRect(Fill, X, Y, W, H);
	DrawShadowedText(Label, FLinearColor::White, X + PadX, Y + PadY, Font, Scale);

	const bool bDown = PlayerOwner->IsInputKeyDown(EKeys::LeftMouseButton);
	if (bHover && bDown && !bRespawnMouseWasDown)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		PlayerOwner->ConsoleCommand(TEXT("RestartLevel"));
	}
	bRespawnMouseWasDown = bDown;
}

void AMarsHUD::DrawHelp()
{
	const AMarsPlayerController* PC = Cast<AMarsPlayerController>(PlayerOwner);
	if (PC && !PC->IsHelpVisible())
	{
		return;
	}
	const float S = UIScale();
	FString CamName = TEXT("first person");
	if (PC)
	{
		switch (PC->GetViewMode())
		{
		case EMarsViewMode::Director:   CamName = TEXT("director"); break;
		case EMarsViewMode::WideOrbit:  CamName = TEXT("wide orbit"); break;
		case EMarsViewMode::LowAction:  CamName = TEXT("low action"); break;
		default:                        CamName = TEXT("first person"); break;
		}
	}
	const FString Help = FString::Printf(
		TEXT("WASD move   Mouse look   LMB fire   Space jump   Shift sprint   C camera (%s)   P slow-mo   R restart   H hide help   Esc quit"),
		*CamName);
	DrawShadowedText(Help, FLinearColor(1.f, 1.f, 1.f, 0.75f), 24.f * S, Canvas->ClipY - 36.f * S, GEngine->GetSmallFont(), 1.05f * S);
}
