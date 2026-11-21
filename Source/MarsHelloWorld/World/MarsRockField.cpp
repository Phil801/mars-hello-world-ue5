// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsRockField.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MarsHelloWorld.h"
#include "World/MarsTerrain.h"
#include "Core/MarsShowSettings.h"
#include "Engine/World.h"

namespace
{
	TSoftObjectPtr<UStaticMesh> RockMesh(const TCHAR* Name)
	{
		return TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/Meshes/SM_%s.SM_%s"), Name, Name)));
	}

	TArray<UStaticMesh*> LoadMeshes(const TArray<TSoftObjectPtr<UStaticMesh>>& Paths)
	{
		TArray<UStaticMesh*> Out;
		for (const TSoftObjectPtr<UStaticMesh>& Path : Paths)
		{
			if (UStaticMesh* Mesh = Path.LoadSynchronous())
			{
				Out.Add(Mesh);
			}
		}
		return Out;
	}
}

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

	// LowPoly Rock Pack: rounded rocks replace the spheres, flat and tall ones replace the cubes.
	for (const TCHAR* Name : { TEXT("Boulder_01"), TEXT("Boulder_02"), TEXT("Boulder_03"), TEXT("Boulder_04"), TEXT("Boulder_05"), TEXT("Boulder_06"),
		TEXT("Rock_01"), TEXT("Rock_02"), TEXT("Rock_03"), TEXT("Rock_04"), TEXT("Rock_05"), TEXT("Rock_06"),
		TEXT("Pebble_01"), TEXT("Pebble_02"), TEXT("Pebble_03"), TEXT("Pebble_04") })
	{
		BoulderMeshes.Add(RockMesh(Name));
	}
	for (const TCHAR* Name : { TEXT("Slab_01"), TEXT("Slab_02"), TEXT("Slab_03"), TEXT("Slab_04"),
		TEXT("Pillar_01"), TEXT("Pillar_02"), TEXT("Pillar_03"), TEXT("Pillar_04") })
	{
		SlabMeshes.Add(RockMesh(Name));
	}
}

void AMarsRockField::PlaceRocks(const TArray<FTransform>& BoulderShapes, const TArray<FTransform>& SlabShapes, int32 Seed,
	const TArray<FTransform>& PebbleCandidates, const TArray<FTransform>& BoulderCandidates,
	const TArray<FTransform>& SlabCandidates)
{
	ClearRockComponents();

	const TArray<UStaticMesh*> BoulderSet = LoadMeshes(BoulderMeshes);
	TArray<UStaticMesh*> SlabSet = LoadMeshes(SlabMeshes);
	if (SlabSet.IsEmpty())
	{
		SlabSet = BoulderSet;
	}
	if (BoulderSet.IsEmpty() && SlabSet.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("MarsRockField: no rock meshes could be loaded; nothing placed."));
		return;
	}

	FRandomStream Stream(Seed);
	TMap<UStaticMesh*, TArray<FTransform>> PerMesh;

	int32 PebblesPlaced = 0;
	FRandomStream PebbleSizeStream(Seed ^ 0xBEEF);
	FRandomStream RockSizeStream(Seed ^ 0xCAFE);
	FRandomStream BoulderSizeStream(Seed ^ 0xB0B0);
	auto PlaceOne = [&](const FTransform& Shape, UStaticMesh* Mesh)
	{
		const FBox Bounds = Mesh->GetBoundingBox();
		const FVector MeshSize = Bounds.GetSize().ComponentMax(FVector(1.f));
		const FVector S = Shape.GetScale3D().GetAbs();

		// Same volume as the 100 cm shape it replaces, with the mesh's own proportions.
		const float ShapeVolume = 1.0e6f * S.X * S.Y * S.Z;
		float Uniform = FMath::Pow(ShapeVolume / (MeshSize.X * MeshSize.Y * MeshSize.Z), 1.f / 3.f);
		if (Mesh->GetName().Contains(TEXT("Boulder")) || Mesh->GetName().Contains(TEXT("Pillar")))
		{
			Uniform = BoulderSizeStream.FRandRange(BoulderScale.X, FMath::Max(BoulderScale.X, BoulderScale.Y));
		}
		else if (Mesh->GetName().Contains(TEXT("Pebble")))
		{
			Uniform = PebbleSizeStream.FRandRange(PebbleScale.X, FMath::Max(PebbleScale.X, PebbleScale.Y));
		}
		else if (Mesh->GetName().StartsWith(TEXT("SM_Rock_")))
		{
			Uniform = RockSizeStream.FRandRange(RockScale.X, FMath::Max(RockScale.X, RockScale.Y));
		}

		// The builder raised shapes by Scale.Z * 20 cm above the ground; rock pivots sit at the base.
		const FVector L = Shape.GetLocation();
		const float Ground = L.Z - S.Z * 20.f;
		const FVector Loc(L.X, L.Y, Ground - Bounds.Min.Z * Uniform - MeshSize.Z * Uniform * SinkFraction);
		PerMesh.FindOrAdd(Mesh).Add(FTransform(Shape.GetRotation(), Loc, FVector(Uniform)));
		if (Mesh->GetName().Contains(TEXT("Pebble")))
		{
			++PebblesPlaced;
		}
	};

	auto Place = [&](const TArray<FTransform>& Shapes, const TArray<UStaticMesh*>& Meshes)
	{
		if (Meshes.IsEmpty())
		{
			return;
		}
		for (const FTransform& Shape : Shapes)
		{
			// Same random pick as before so rocks keep their meshes, but pebbles are placed separately below.
			UStaticMesh* Mesh = Meshes[Stream.RandRange(0, Meshes.Num() - 1)];
			if (!Mesh->GetName().Contains(TEXT("Pebble")))
			{
				PlaceOne(Shape, Mesh);
			}
		}
	};
	// Round layout spots get Rock meshes only; boulders and pebbles have their own scatter below.
	TArray<UStaticMesh*> RockSet;
	for (UStaticMesh* Mesh : BoulderSet)
	{
		if (Mesh->GetName().StartsWith(TEXT("SM_Rock_")))
		{
			RockSet.Add(Mesh);
		}
	}
	Place(BoulderShapes, RockSet.IsEmpty() ? SlabSet : RockSet);
	Place(SlabShapes, RockSet.IsEmpty() ? SlabSet : RockSet);

	// Pebbles: their own count and spots, separate from the rock layout.
	TArray<UStaticMesh*> PebbleSet;
	for (UStaticMesh* Mesh : BoulderSet)
	{
		if (Mesh->GetName().Contains(TEXT("Pebble")))
		{
			PebbleSet.Add(Mesh);
		}
	}
	const int32 NumPebbles = PebbleSet.IsEmpty() ? 0 : FMath::Min(GetPebbleCount(), PebbleCandidates.Num());
	FRandomStream PebbleStream(Seed ^ 0x5EB1E);
	for (int32 i = 0; i < NumPebbles; ++i)
	{
		PlaceOne(PebbleCandidates[i], PebbleSet[PebbleStream.RandRange(0, PebbleSet.Num() - 1)]);
	}
	UE_LOG(LogTemp, Display, TEXT("MarsRockField: %d pebbles placed."), PebblesPlaced);

	// Boulders and pillars: their own count and spots.
	TArray<UStaticMesh*> BoulderOnlySet;
	for (UStaticMesh* Mesh : BoulderSet)
	{
		if (Mesh->GetName().Contains(TEXT("Boulder")))
		{
			BoulderOnlySet.Add(Mesh);
		}
	}
	// Pillars share the boulder scatter.
	for (UStaticMesh* Mesh : SlabSet)
	{
		if (Mesh->GetName().Contains(TEXT("Pillar")))
		{
			BoulderOnlySet.Add(Mesh);
		}
	}
	const int32 NumBoulders = BoulderOnlySet.IsEmpty() ? 0 : FMath::Min(GetBoulderCount(), BoulderCandidates.Num());
	FRandomStream BoulderStream(Seed ^ 0xB01D);
	for (int32 i = 0; i < NumBoulders; ++i)
	{
		PlaceOne(BoulderCandidates[i], BoulderOnlySet[BoulderStream.RandRange(0, BoulderOnlySet.Num() - 1)]);
	}
	UE_LOG(LogTemp, Display, TEXT("MarsRockField: %d boulders placed."), NumBoulders);

	// Slabs: their own count and spots.
	TArray<UStaticMesh*> SlabOnlySet;
	for (UStaticMesh* Mesh : SlabSet)
	{
		if (Mesh->GetName().Contains(TEXT("Slab")))
		{
			SlabOnlySet.Add(Mesh);
		}
	}
	const int32 NumSlabs = SlabOnlySet.IsEmpty() ? 0 : FMath::Min(GetSlabCount(), SlabCandidates.Num());
	FRandomStream SlabStream(Seed ^ 0x51AB);
	for (int32 i = 0; i < NumSlabs; ++i)
	{
		PlaceOne(SlabCandidates[i], SlabOnlySet[SlabStream.RandRange(0, SlabOnlySet.Num() - 1)]);
	}
	UE_LOG(LogTemp, Display, TEXT("MarsRockField: %d slabs placed."), NumSlabs);

	for (TPair<UStaticMesh*, TArray<FTransform>>& Pair : PerMesh)
	{
		if (UInstancedStaticMeshComponent* Comp = CreateRockComponent(Pair.Key))
		{
			Comp->AddInstances(Pair.Value, false, false);
		}
	}

	Boulders->ClearInstances();
	Slabs->ClearInstances();
	Boulders->SetStaticMesh(nullptr);
	Slabs->SetStaticMesh(nullptr);
}

void AMarsRockField::ClearRockComponents()
{
	for (UInstancedStaticMeshComponent* Comp : RockComponents)
	{
		if (Comp)
		{
			RemoveInstanceComponent(Comp);
			Comp->DestroyComponent();
		}
	}
	RockComponents.Reset();
}

UInstancedStaticMeshComponent* AMarsRockField::CreateRockComponent(UStaticMesh* Mesh)
{
	const FName Name = MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), FName(*FString::Printf(TEXT("Rocks_%s"), *Mesh->GetName())));
	UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(this, Name, RF_Transactional);
	Comp->SetMobility(EComponentMobility::Static);
	Comp->SetupAttachment(Root);
	Comp->SetStaticMesh(Mesh);
	// Pebbles are decoration only: no collision.
	const bool bPebble = Mesh->GetName().Contains(TEXT("Pebble"));
	Comp->SetCollisionProfileName(bPebble ? UCollisionProfile::NoCollision_ProfileName : UCollisionProfile::BlockAll_ProfileName);
	AddInstanceComponent(Comp);
	Comp->RegisterComponent();
	RockComponents.Add(Comp);
	return Comp;
}

void AMarsRockField::ComputePebbleCandidates(const AMarsTerrain* Height, int32 Seed, int32 Count, TArray<FTransform>& Out)
{
	Out.Reset();
	if (!Height)
	{
		return;
	}
	// Radial scatter (unlike the even rock layout), on a separate stream.
	FRandomStream Stream(Seed + 7841);
	const float ArenaRadius = Height->GetArenaRadius();
	const float HalfExtent = Height->GetHalfExtent();
	for (int32 i = 0; i < Count; ++i)
	{
		// Radial: dense near the arena edge, sparse far away.
		const float Angle = Stream.FRandRange(0.f, 2.f * PI);
		const float U = Stream.FRand();
		const float Dist = FMath::Lerp(ArenaRadius * 1.15f, HalfExtent * 0.8f, U * U);
		const float X = FMath::Cos(Angle) * Dist;
		const float Y = FMath::Sin(Angle) * Dist;
		const float Size = FMath::Lerp(0.25f, 2.6f, FMath::Pow(Stream.FRand(), 2.2f));
		const FVector Scale(Size * Stream.FRandRange(0.8f, 1.6f), Size * Stream.FRandRange(0.8f, 1.4f), Size * Stream.FRandRange(0.45f, 0.9f));
		const FRotator Rot(Stream.FRandRange(-15.f, 15.f), Stream.FRandRange(0.f, 360.f), Stream.FRandRange(-15.f, 15.f));
		const FVector Loc(X, Y, Height->GetHeightAt(X, Y) + Scale.Z * 50.f * 0.4f);
		Out.Add(FTransform(Rot, Loc, Scale));
	}
}

void AMarsRockField::ComputeBoulderCandidates(const AMarsTerrain* Height, int32 Seed, int32 Count, TArray<FTransform>& Out)
{
	Out.Reset();
	if (!Height)
	{
		return;
	}
	// Even spread like the rock layout, on a separate stream.
	FRandomStream Stream(Seed + 9173);
	const float ArenaRadius = Height->GetArenaRadius();
	const float Extent = Height->GetHalfExtent() * 0.9f;
	for (int32 i = 0; i < Count; ++i)
	{
		float X, Y;
		do
		{
			X = Stream.FRandRange(-Extent, Extent);
			Y = Stream.FRandRange(-Extent, Extent);
		} while (FMath::Sqrt(X * X + Y * Y) < ArenaRadius * 1.15f);
		const float Size = FMath::Lerp(0.25f, 2.6f, FMath::Pow(Stream.FRand(), 2.2f));
		const FVector Scale(Size * Stream.FRandRange(0.8f, 1.6f), Size * Stream.FRandRange(0.8f, 1.4f), Size * Stream.FRandRange(0.45f, 0.9f));
		const FRotator Rot(Stream.FRandRange(-15.f, 15.f), Stream.FRandRange(0.f, 360.f), Stream.FRandRange(-15.f, 15.f));
		const FVector Loc(X, Y, Height->GetHeightAt(X, Y) + Scale.Z * 50.f * 0.4f);
		Out.Add(FTransform(Rot, Loc, Scale));
	}
}

#if WITH_EDITOR
void AMarsRockField::ReplaceShapesWithRocks()
{
	TArray<FTransform> BoulderShapes, SlabShapes;
	auto Gather = [](const UInstancedStaticMeshComponent* Comp, TArray<FTransform>& Out)
	{
		for (int32 i = 0; Comp && i < Comp->GetInstanceCount(); ++i)
		{
			FTransform Xform;
			if (Comp->GetInstanceTransform(i, Xform, false))
			{
				Out.Add(Xform * Comp->GetRelativeTransform());
			}
		}
	};
	Gather(Boulders, BoulderShapes);
	Gather(Slabs, SlabShapes);
	if (BoulderShapes.IsEmpty() && SlabShapes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("MarsRockField: no sphere or cube instances to replace."));
		return;
	}

	Modify();
	Boulders->Modify();
	Slabs->Modify();
	PlaceRocks(BoulderShapes, SlabShapes, BoulderShapes.Num() * 7919 + SlabShapes.Num());
	UE_LOG(LogTemp, Display, TEXT("MarsRockField: replaced %d shapes with rocks in %d meshes."), BoulderShapes.Num() + SlabShapes.Num(), RockComponents.Num());
}

void AMarsRockField::RegenerateRocks()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Same sequence as UMarsArenaBuilder: seed -> height field -> rock layout.
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.bHideFromSceneOutliner = true;
	AMarsTerrain* Height = World->SpawnActor<AMarsTerrain>(AMarsTerrain::StaticClass(), FTransform::Identity, Params);
	if (!Height)
	{
		UE_LOG(LogTemp, Warning, TEXT("MarsRockField: could not spawn the temporary AMarsTerrain."));
		return;
	}
	const int32 Seed = GetDefault<UMarsShowSettings>()->RandomSeed;
	FRandomStream Stream(Seed);
	Height->InitHeightField(Stream);
	Height->SetRockCount(FMath::RoundToInt(Height->GetRockCount() * RockDensity));
	TArray<FTransform> BoulderShapes, SlabShapes;
	Height->ComputeRockTransforms(Stream, BoulderShapes, SlabShapes);
	TArray<FTransform> PebbleCandidates;
	ComputePebbleCandidates(Height, Seed, GetPebbleCount(), PebbleCandidates);
	TArray<FTransform> BoulderCandidates;
	ComputeBoulderCandidates(Height, Seed, GetBoulderCount(), BoulderCandidates);
	TArray<FTransform> SlabCandidates;
	ComputeBoulderCandidates(Height, Seed ^ 0x51AB, GetSlabCount(), SlabCandidates);
	World->EditorDestroyActor(Height, false);

	Modify();
	Boulders->Modify();
	Slabs->Modify();
	PlaceRocks(BoulderShapes, SlabShapes, Seed, PebbleCandidates, BoulderCandidates, SlabCandidates);
	UE_LOG(LogTemp, Display, TEXT("MarsRockField: regenerated %d rocks in %d meshes (seed %d)."), BoulderShapes.Num() + SlabShapes.Num(), RockComponents.Num(), Seed);
}
#endif