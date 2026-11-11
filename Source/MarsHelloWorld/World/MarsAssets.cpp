// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsAssets.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	template <typename T>
	T* LoadByPath(const TCHAR* Path)
	{
		// FindObject first: cheap when already loaded; LoadObject otherwise. Quiet on failure so
		// the optional /Game assets can be missing without log spam.
		if (T* Found = FindObject<T>(nullptr, Path))
		{
			return Found;
		}
		// Remember misses so optional assets are only probed on disk once per session.
		static TSet<FString> KnownMissing;
		const FString Key(Path);
		if (KnownMissing.Contains(Key))
		{
			return nullptr;
		}
		T* Loaded = LoadObject<T>(nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
		if (!Loaded)
		{
			KnownMissing.Add(Key);
		}
		return Loaded;
	}
}

UStaticMesh* MarsAssets::GetShape(EMarsShape Shape)
{
	switch (Shape)
	{
	case EMarsShape::Cube:     return LoadByPath<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube"));
	case EMarsShape::Sphere:   return LoadByPath<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	case EMarsShape::Cylinder: return LoadByPath<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	case EMarsShape::Cone:     return LoadByPath<UStaticMesh>(TEXT("/Engine/BasicShapes/Cone.Cone"));
	case EMarsShape::Plane:    return LoadByPath<UStaticMesh>(TEXT("/Engine/BasicShapes/Plane.Plane"));
	default:                   return nullptr;
	}
}

UMaterialInterface* MarsAssets::GetSurfaceMaterial()
{
	if (UMaterialInterface* M = LoadByPath<UMaterialInterface>(TEXT("/Game/Materials/M_MarsSurface.M_MarsSurface")))
	{
		return M;
	}
	return LoadByPath<UMaterialInterface>(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

UMaterialInterface* MarsAssets::GetEmissiveMaterial()
{
	if (UMaterialInterface* M = LoadByPath<UMaterialInterface>(TEXT("/Game/Materials/M_MarsEmissive.M_MarsEmissive")))
	{
		return M;
	}
	return GetSurfaceMaterial();
}

UMaterialInterface* MarsAssets::GetGroundMaterial()
{
	if (UMaterialInterface* M = LoadByPath<UMaterialInterface>(TEXT("/Game/Materials/M_MarsGround.M_MarsGround")))
	{
		return M;
	}
	return GetSurfaceMaterial();
}

bool MarsAssets::HasProjectMaterials()
{
	return LoadByPath<UMaterialInterface>(TEXT("/Game/Materials/M_MarsGround.M_MarsGround")) != nullptr;
}

UMaterialInstanceDynamic* MarsAssets::MakeColoredMaterial(UObject* Outer, UMaterialInterface* Parent, const FLinearColor& Color, float Glow)
{
	if (!Parent)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Glow"), Glow);
	}
	return MID;
}

UStaticMeshComponent* MarsAssets::AddShape(AActor* Owner, USceneComponent* Parent, FName Name, EMarsShape Shape,
	const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& Scale,
	const FLinearColor& Color, float Glow)
{
	if (!Owner || !Parent)
	{
		return nullptr;
	}

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner, Name);
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(GetShape(Shape));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetGenerateOverlapEvents(false);
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetupAttachment(Parent);
	Comp->SetRelativeLocation(RelativeLocation);
	Comp->SetRelativeRotation(RelativeRotation);
	Comp->SetRelativeScale3D(Scale);

	UMaterialInterface* Parent_Material = Glow > 0.f ? GetEmissiveMaterial() : GetSurfaceMaterial();
	if (UMaterialInstanceDynamic* MID = MakeColoredMaterial(Comp, Parent_Material, Color, Glow))
	{
		Comp->SetMaterial(0, MID);
	}

	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	return Comp;
}
