// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"

class USoundBase;
struct FCombatantProfile;

/**
 * Resolves a combatant's "Hello world" voice line:
 *  1. the imported SoundWave asset (Content/Python/mars_setup.py imports RawAudio/*.wav into /Game/Audio), else
 *  2. the raw WAV under <Project>/RawAudio streamed through a USoundWaveProcedural (works in a fresh
 *     clone before any import, in editor and Development builds), else
 *  3. nullptr (the subtitle still shows, so the show never blocks on audio).
 */
namespace MarsVoice
{
	/** Returns a playable sound and its length in seconds. A new procedural wave is made per call (they are single-use). */
	MARSHELLOWORLD_API USoundBase* ResolveVoice(UObject* Outer, const FCombatantProfile& Profile, float& OutDurationSeconds);
}
