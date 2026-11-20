// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsRockField.generated.h"

class UInstancedStaticMeshComponent;

/** Level-placed boulders and slabs (two instanced meshes), filled by UMarsArenaBuilder from the terrain seed. */
UCLASS(meta = (DisplayName = "Mars Rock Field"))
class MARSHELLOWORLD_API AMarsRockField : public AActor
{
	GENERATED_BODY()

public:
	AMarsRockField();

	UInstancedStaticMeshComponent* GetBoulders() const { return Boulders; }
	UInstancedStaticMeshComponent* GetSlabs() const { return Slabs; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UInstancedStaticMeshComponent> Boulders;

	UPROPERTY(VisibleAnywhere, Category = "Mars")
	TObjectPtr<UInstancedStaticMeshComponent> Slabs;
};
