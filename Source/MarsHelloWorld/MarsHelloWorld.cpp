// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "MarsHelloWorld.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogMars);

// Thin game module: all behaviour lives in gameplay classes, subsystems and settings
// (the "game module is glue" pattern from the top-20 UE5 study).
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MarsHelloWorld, "MarsHelloWorld");
