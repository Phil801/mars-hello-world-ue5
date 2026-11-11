// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "World/MarsTerrain.h"
#include "World/MarsAssets.h"
#include "MarsHelloWorld.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AMarsTerrain::AMarsTerrain()
{
	PrimaryActorTick.bCanEverTick = false;

	Ground = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Ground"));
	Ground->bUseComplexAsSimpleCollision = true;
	Ground->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Ground->SetCastShadow(true);
	RootComponent = Ground;

	Boulders = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boulders"));
	Boulders->SetupAttachment(Ground);
	Boulders->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Slabs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Slabs"));
	Slabs->SetupAttachment(Ground);
	Slabs->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

float AMarsTerrain::SampleFractal(float X, float Y, int32 Octaves) const
{
	float Sum = 0.f;
	float Amp = 1.f;
	float Freq = 1.f;
	float Norm = 0.f;
	for (int32 i = 0; i < Octaves; ++i)
	{
		Sum += Amp * FMath::PerlinNoise2D(FVector2D(X * Freq, Y * Freq) + NoiseOffset * (1.f + i));
		Norm += Amp;
		Amp *= 0.5f;
		Freq *= 2.03f;
	}
	return Norm > 0.f ? Sum / Norm : 0.f;
}

float AMarsTerrain::GetHeightAt(float X, float Y) const
{
	const float R = FMath::Sqrt(X * X + Y * Y);

	// Rolling dunes and plains.
	float H = SampleFractal(X * 0.00005f, Y * 0.00005f, 5) * 900.f;

	// Sharp ridges that grow with distance: frames the horizon like a crater-floor vista.
	const float RidgeNoise = 1.f - FMath::Abs(FMath::PerlinNoise2D(FVector2D(X * 0.000035f, Y * 0.000035f) + NoiseOffset * 3.f));
	H += RidgeNoise * RidgeNoise * 2600.f * FMath::SmoothStep(7000.f, 26000.f, R);

	// Bowl: the far edge rises so the world reads as a big crater basin.
	H += FMath::SmoothStep(16000.f, HalfExtent, R) * 3200.f;

	// Craters: depression plus a raised rim.
	for (const FMarsCrater& C : Craters)
	{
		const float D = static_cast<float>(FVector2D::Distance(FVector2D(X, Y), C.Center)) / C.Radius;
		if (D < 1.f)
		{
			H -= C.Depth * (1.f - D * D);
		}
		const float RimT = (D - 1.f) / 0.22f;
		H += C.Depth * 0.45f * FMath::Exp(-RimT * RimT);
	}

	// Arena: nearly flat with a gentle texture, blended into the wild terrain.
	const float Arena = FMath::PerlinNoise2D(FVector2D(X * 0.0005f, Y * 0.0005f) + NoiseOffset) * 18.f;
	const float T = FMath::SmoothStep(ArenaRadius, ArenaRadius * 3.f, R);
	return FMath::Lerp(Arena, H, T);
}

FLinearColor AMarsTerrain::GroundColorAt(float X, float Y, float Height, const FVector& Normal) const
{
	// Linear-space rust palette (sampled from NASA Curiosity/Perseverance true-colour imagery tones).
	const FLinearColor Dust(0.42f, 0.16f, 0.065f);
	const FLinearColor Ochre(0.60f, 0.30f, 0.12f);
	const FLinearColor Basalt(0.16f, 0.075f, 0.045f);

	const float Variation = 0.5f + 0.5f * FMath::PerlinNoise2D(FVector2D(X * 0.0003f, Y * 0.0003f) + NoiseOffset * 5.f);
	FLinearColor Col = FMath::Lerp(Dust, Ochre, FMath::Clamp(Variation, 0.f, 1.f));

	// Steep slopes expose darker rock; low crater floors collect darker sand.
	const float Steep = FMath::Clamp((0.92f - static_cast<float>(Normal.Z)) * 4.f, 0.f, 1.f);
	Col = FMath::Lerp(Col, Basalt, Steep * 0.8f);
	const float Low = FMath::Clamp(-Height / 600.f, 0.f, 1.f);
	Col = FMath::Lerp(Col, Basalt, Low * 0.35f);
	Col.A = 1.f;
	return Col;
}

void AMarsTerrain::BuildTerrain(int32 Seed)
{
	FRandomStream Stream(Seed);
	NoiseOffset = FVector2D(Stream.FRandRange(-500.f, 500.f), Stream.FRandRange(-500.f, 500.f));

	// Craters (kept out of the arena).
	Craters.Reset();
	for (int32 i = 0; i < 14; ++i)
	{
		FMarsCrater C;
		const float Angle = Stream.FRandRange(0.f, 2.f * PI);
		const float Dist = Stream.FRandRange(5500.f, HalfExtent * 0.85f);
		C.Center = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Dist;
		C.Radius = Stream.FRandRange(900.f, 4200.f);
		C.Depth = C.Radius * Stream.FRandRange(0.12f, 0.22f);
		Craters.Add(C);
	}

	// Grid from the engine helper (winding matches the engine's own up-facing plane),
	// then displace it with our height function.
	TArray<int32> Triangles;
	TArray<FVector> Vertices;
	TArray<FVector2D> UVs;
	const float Spacing = (2.f * HalfExtent) / float(GridResolution - 1);
	UKismetProceduralMeshLibrary::CreateGridMeshWelded(GridResolution, GridResolution, Triangles, Vertices, UVs, Spacing);

	if (bFlipWinding)
	{
		for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
		{
			Swap(Triangles[i + 1], Triangles[i + 2]);
		}
	}

	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> Colors;
	Normals.SetNum(Vertices.Num());
	Tangents.SetNum(Vertices.Num());
	Colors.SetNum(Vertices.Num());

	// The helper lays the grid out from (0,0); centre it on the origin.
	FVector Min = Vertices.Num() > 0 ? Vertices[0] : FVector::ZeroVector;
	for (const FVector& V : Vertices)
	{
		Min = Min.ComponentMin(V);
	}
	const FVector Centering(-Min.X - HalfExtent, -Min.Y - HalfExtent, 0.f);

	const float Eps = Spacing * 0.5f;
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		FVector& V = Vertices[i];
		V += Centering;
		const float VX = static_cast<float>(V.X);
		const float VY = static_cast<float>(V.Y);
		V.Z = GetHeightAt(VX, VY);

		const float Dx = (GetHeightAt(VX + Eps, VY) - GetHeightAt(VX - Eps, VY)) / (2.f * Eps);
		const float Dy = (GetHeightAt(VX, VY + Eps) - GetHeightAt(VX, VY - Eps)) / (2.f * Eps);
		const FVector N = FVector(-Dx, -Dy, 1.f).GetSafeNormal();
		Normals[i] = N;
		Tangents[i] = FProcMeshTangent(FVector(1.f, 0.f, Dx).GetSafeNormal(), false);
		Colors[i] = GroundColorAt(VX, VY, static_cast<float>(V.Z), N);
		UVs[i] = FVector2D(V.X, V.Y) / 400.f; // 4 m texture tiling for future detail textures
	}

	Ground->ClearAllMeshSections();
	Ground->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, /*bCreateCollision*/ true);

	// Ground material: vertex-coloured M_MarsGround when the setup script has run, else a flat rust MID.
	UMaterialInterface* GroundMat = MarsAssets::GetGroundMaterial();
	const bool bVertexColorMaterial = MarsAssets::HasProjectMaterials();
	Ground->SetMaterial(0, bVertexColorMaterial
		? GroundMat
		: MarsAssets::MakeColoredMaterial(this, GroundMat, FLinearColor(0.45f, 0.18f, 0.075f)));

	ScatterRocks(Stream);

	UE_LOG(LogMars, Log, TEXT("Mars terrain built: %d verts, %d tris, %d craters, %d rocks (seed %d)"),
		Vertices.Num(), Triangles.Num() / 3, Craters.Num(), Boulders->GetInstanceCount() + Slabs->GetInstanceCount(), Seed);
}

void AMarsTerrain::ScatterRocks(FRandomStream& Stream)
{
	Boulders->ClearInstances();
	Slabs->ClearInstances();
	Boulders->SetStaticMesh(MarsAssets::GetShape(EMarsShape::Sphere));
	Slabs->SetStaticMesh(MarsAssets::GetShape(EMarsShape::Cube));

	UMaterialInterface* RockParent = MarsAssets::GetSurfaceMaterial();
	Boulders->SetMaterial(0, MarsAssets::MakeColoredMaterial(this, RockParent, FLinearColor(0.20f, 0.085f, 0.045f)));
	Slabs->SetMaterial(0, MarsAssets::MakeColoredMaterial(this, RockParent, FLinearColor(0.30f, 0.12f, 0.06f)));

	for (int32 i = 0; i < RockCount; ++i)
	{
		// Dense near the arena edge (good foreground for the camera), sparse far away.
		const float Angle = Stream.FRandRange(0.f, 2.f * PI);
		const float U = Stream.FRand();
		const float Dist = FMath::Lerp(ArenaRadius * 1.15f, HalfExtent * 0.8f, U * U);
		const float X = FMath::Cos(Angle) * Dist;
		const float Y = FMath::Sin(Angle) * Dist;
		const float Size = FMath::Lerp(0.25f, 2.6f, FMath::Pow(Stream.FRand(), 2.2f)) * (1.f + Dist / 15000.f);

		const FVector Scale(Size * Stream.FRandRange(0.8f, 1.6f), Size * Stream.FRandRange(0.8f, 1.4f), Size * Stream.FRandRange(0.45f, 0.9f));
		const FRotator Rot(Stream.FRandRange(-15.f, 15.f), Stream.FRandRange(0.f, 360.f), Stream.FRandRange(-15.f, 15.f));
		// Engine basic shapes are 100 cm across; sink rocks 30% into the ground.
		const FVector Loc(X, Y, GetHeightAt(X, Y) + Scale.Z * 50.f * 0.4f);

		UInstancedStaticMeshComponent* Target = (Stream.FRand() < 0.7f) ? Boulders.Get() : Slabs.Get();
		Target->AddInstance(FTransform(Rot, Loc, Scale), /*bWorldSpace*/ true);
	}
}
