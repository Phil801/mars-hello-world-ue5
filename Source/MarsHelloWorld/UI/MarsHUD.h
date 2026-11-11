// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MarsHUD.generated.h"

class UFont;
class UMarsShowSubsystem;

/**
 * Canvas HUD (no UMG assets needed): banners, "hello world" subtitles, health bars over heads,
 * rising damage numbers, kill feed, fight clock / sudden death, controls help.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	float UIScale() const;
	void DrawShadowedText(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, bool bCenterX = false);
	void DrawBanner(UMarsShowSubsystem* Show, float Now);
	void DrawCaption(UMarsShowSubsystem* Show, float Now);
	void DrawHealthBars(UMarsShowSubsystem* Show);
	void DrawFloatingTexts(UMarsShowSubsystem* Show, float Now);
	void DrawStatus(UMarsShowSubsystem* Show);
	void DrawKillFeed(UMarsShowSubsystem* Show);
	void DrawHelp();
};
