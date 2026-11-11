// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"

/** One log category for the whole game: `Log LogMars Verbose` in the console to see the show's director notes. */
MARSHELLOWORLD_API DECLARE_LOG_CATEGORY_EXTERN(LogMars, Log, All);

/**
 * Always writes a show event (stdout + LogMars). UE_LOG is compiled out of
 * Shipping when the target shares UnrealGame, so headless packaged runs
 * still need this.
 */
MARSHELLOWORLD_API void MarsShowLog(const FString& Line);
