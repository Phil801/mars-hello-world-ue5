// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "MarsHelloWorld.h"
#include "Modules/ModuleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include <stdio.h>

DEFINE_LOG_CATEGORY(LogMars);

// Thin game module: all behaviour lives in gameplay classes, subsystems and settings
// (the "game module is glue" pattern from the top-20 UE5 study).
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MarsHelloWorld, "MarsHelloWorld");

void MarsShowLog(const FString& Line)
{
	UE_LOG(LogMars, Log, TEXT("%s"), *Line);
	printf("%s\n", TCHAR_TO_UTF8(*Line));
	fflush(stdout);

	// Shipping Windows GUI apps have no console, so stdout never reaches a
	// terminal. Always append show events to Saved/Logs/MarsShow.log as well.
	const FString ShowLogFile = FPaths::ProjectLogDir() / TEXT("MarsShow.log");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ShowLogFile), true);
	FFileHelper::SaveStringToFile(
		Line + LINE_TERMINATOR,
		*ShowLogFile,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(),
		FILEWRITE_Append | FILEWRITE_AllowRead);
}
