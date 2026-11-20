// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsRockField.h"
#include "Components/InstancedStaticMeshComponent.h"

AMarsRockField::AMarsRockField()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	Boulders = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boulders"));
	Boulders->SetupAttachment(Root);
	Boulders->SetMobility(EComponentMobility::Static);
	Boulders->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Slabs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Slabs"));
	Slabs->SetupAttachment(Root);
	Slabs->SetMobility(EComponentMobility::Static);
	Slabs->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}
