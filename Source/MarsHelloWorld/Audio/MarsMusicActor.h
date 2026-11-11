// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsMusicActor.generated.h"

class UAudioComponent;
class USoundWaveProcedural;
class USoundBase;

/**
 * Procedural / raw-WAV soundtrack:
 *  - ambient Mars loop during arrivals and the Kevin search
 *  - 132 BPM battle loop, crossfaded in on the first shot
 *  - short victory sting
 * Loops are seamless files from Tools/music/generate_music.py. Music ducks under voice lines.
 * A level restart (Respawn) destroys this actor and the next BeginShow starts ambient again.
 */
UCLASS()
class MARSHELLOWORLD_API AMarsMusicActor : public AActor
{
	GENERATED_BODY()

public:
	AMarsMusicActor();

	virtual void Tick(float DeltaSeconds) override;

	void StartAmbient();
	void NotifyFirstShot();
	void PlayVictory();
	void NotifyVoice(float DurationSeconds);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> AmbientComp;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> BattleComp;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> StingComp;

private:
	USoundBase* ResolveTrack(const TCHAR* FileStem, bool bLoop, TArray<uint8>& OutPcmForPump, int32& OutSampleRate, int32& OutChannels, float& OutDuration);
	void Pump(USoundWaveProcedural* Proc, const TArray<uint8>& LoopPcm);
	void ApplyVolumes();
	void StopLoops();

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> AmbientProc;

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> BattleProc;

	TArray<uint8> AmbientPcm;
	TArray<uint8> BattlePcm;

	float AmbientTarget = 0.f;
	float BattleTarget = 0.f;
	float AmbientVol = 0.f;
	float BattleVol = 0.f;
	float DuckCurrent = 1.f;
	float DuckUntil = -1.f;
	float CrossfadeSeconds = 1.25f;
	float AmbientLevel = 0.42f;
	float BattleLevel = 0.55f;
	float StingLevel = 0.72f;
	float DuckLevel = 0.28f;
	float MasterVolume = 0.85f;
	bool bBattleStarted = false;
	bool bVictoryPlayed = false;
};
