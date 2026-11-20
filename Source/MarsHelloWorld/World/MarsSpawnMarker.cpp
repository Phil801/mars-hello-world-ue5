// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsSpawnMarker.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AMarsSpawnMarker::AMarsSpawnMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	Arrow->ArrowColor = FColor(255, 120, 40);
	Arrow->ArrowSize = 2.f;
	Arrow->bIsScreenSizeScaled = true;

#if WITH_EDITORONLY_DATA
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("/Engine/EditorResources/S_TargetPoint.S_TargetPoint"));
		if (Icon.Succeeded())
		{
			Billboard->Sprite = Icon.Object;
		}
		Billboard->SetupAttachment(Root);
		Billboard->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
		Billboard->bIsScreenSizeScaled = true;
	}
#endif
}
