// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsShowSettings.h"
#include "Combat/CombatantMartian.h"
#include "Combat/CombatantXBot.h"
#include "Combat/CombatantVenusian.h"

UMarsShowSettings::UMarsShowSettings()
{
	CombatantClasses.Add(TSoftClassPtr<ACombatantCharacter>(ACombatantMartian::StaticClass()));
	CombatantClasses.Add(TSoftClassPtr<ACombatantCharacter>(ACombatantXBot::StaticClass()));
	CombatantClasses.Add(TSoftClassPtr<ACombatantCharacter>(ACombatantVenusian::StaticClass()));

	EntranceDelays = { 1.0f, 2.2f, 3.4f };
}
