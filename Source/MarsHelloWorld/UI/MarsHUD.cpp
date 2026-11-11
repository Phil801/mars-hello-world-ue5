// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "UI/MarsHUD.h"
#include "Core/MarsShowSubsystem.h"
#include "Core/MarsPlayerController.h"
#include "Camera/MarsShowCamera.h"
#include "Combat/CombatantCharacter.h"
#include "Combat/MarsHealthComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
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
	DrawFloatingTexts(Show, Now);
	DrawCaption(Show, Now);
	DrawBanner(Show, Now);
	DrawStatus(Show);
	DrawKillFeed(Show);
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
	const float Scale = 1.6f * S;

	float SW = 0.f, SH = 0.f, LW = 0.f, LH = 0.f;
	GetTextSize(Speaker + TEXT(" "), SW, SH, Font, Scale);
	GetTextSize(Line, LW, LH, Font, Scale);
	const float Total = SW + LW;
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
	case EMarsShowPhase::Greetings: PhaseText = TEXT("Greetings"); break;
	case EMarsShowPhase::Countdown: PhaseText = TEXT("Countdown"); break;
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

void AMarsHUD::DrawHelp()
{
	const AMarsPlayerController* PC = Cast<AMarsPlayerController>(PlayerOwner);
	if (PC && !PC->IsHelpVisible())
	{
		return;
	}
	const float S = UIScale();
	FString CamName = TEXT("director");
	if (const AMarsShowCamera* Cam = PC ? Cast<AMarsShowCamera>(PC->GetViewTarget()) : nullptr)
	{
		CamName = Cam->GetMode() == EMarsCameraMode::WideOrbit ? TEXT("wide orbit") : (Cam->GetMode() == EMarsCameraMode::LowAction ? TEXT("low action") : TEXT("director"));
	}
	const FString Help = FString::Printf(TEXT("R restart   Space slow-mo   C camera (%s)   H hide help   Esc quit"), *CamName);
	DrawShadowedText(Help, FLinearColor(1.f, 1.f, 1.f, 0.75f), 24.f * S, Canvas->ClipY - 36.f * S, GEngine->GetSmallFont(), 1.1f * S);
}
