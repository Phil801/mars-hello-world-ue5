// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Audio/MarsMusicActor.h"
#include "Core/MarsShowSettings.h"
#include "MarsHelloWorld.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	uint32 ReadU32(const TArray<uint8>& B, int32 At) { return B[At] | (B[At + 1] << 8) | (B[At + 2] << 16) | (uint32(B[At + 3]) << 24); }
	uint16 ReadU16(const TArray<uint8>& B, int32 At) { return uint16(B[At] | (B[At + 1] << 8)); }

	bool LoadPcm16Wav(const FString& Path, TArray<uint8>& OutPcm, int32& OutRate, int32& OutChannels, float& OutDuration)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() < 44)
		{
			return false;
		}
		if (FMemory::Memcmp(Bytes.GetData(), "RIFF", 4) != 0 || FMemory::Memcmp(Bytes.GetData() + 8, "WAVE", 4) != 0)
		{
			return false;
		}
		int32 Pos = 12;
		bool bHaveFormat = false;
		OutRate = 0;
		OutChannels = 0;
		while (Pos + 8 <= Bytes.Num())
		{
			const uint32 ChunkSize = ReadU32(Bytes, Pos + 4);
			const int32 Body = Pos + 8;
			if (Body + int64(ChunkSize) > Bytes.Num())
			{
				break;
			}
			if (FMemory::Memcmp(Bytes.GetData() + Pos, "fmt ", 4) == 0 && ChunkSize >= 16)
			{
				const uint16 Format = ReadU16(Bytes, Body);
				OutChannels = ReadU16(Bytes, Body + 2);
				OutRate = int32(ReadU32(Bytes, Body + 4));
				const uint16 Bits = ReadU16(Bytes, Body + 14);
				if (Format != 1 || Bits != 16)
				{
					UE_LOG(LogMars, Warning, TEXT("%s is not 16-bit PCM"), *Path);
					return false;
				}
				bHaveFormat = true;
			}
			else if (FMemory::Memcmp(Bytes.GetData() + Pos, "data", 4) == 0 && bHaveFormat)
			{
				OutPcm.Reset();
				OutPcm.Append(Bytes.GetData() + Body, int32(ChunkSize));
				const int32 Frames = OutPcm.Num() / (2 * FMath::Max(1, OutChannels));
				OutDuration = OutRate > 0 ? float(Frames) / float(OutRate) : 0.f;
				return OutRate > 0 && OutChannels > 0;
			}
			Pos = Body + int32(ChunkSize) + (ChunkSize & 1);
		}
		return false;
	}
}

AMarsMusicActor::AMarsMusicActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetHidden(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	auto MakeComp = [this](TObjectPtr<UAudioComponent>& Comp, const TCHAR* Name)
	{
		Comp = CreateDefaultSubobject<UAudioComponent>(Name);
		Comp->SetupAttachment(SceneRoot);
		Comp->bAutoActivate = false;
		Comp->bIsUISound = true;
		Comp->SetUISound(true);
		Comp->bAllowSpatialization = false;
	};
	MakeComp(AmbientComp, TEXT("Ambient"));
	MakeComp(BattleComp, TEXT("Battle"));
	MakeComp(StingComp, TEXT("Sting"));
}

void AMarsMusicActor::BeginPlay()
{
	Super::BeginPlay();
	if (const UMarsShowSettings* Settings = GetDefault<UMarsShowSettings>())
	{
		MasterVolume = Settings->MusicMasterVolume;
		AmbientLevel = Settings->MusicAmbientVolume;
		BattleLevel = Settings->MusicBattleVolume;
		StingLevel = Settings->MusicStingVolume;
		CrossfadeSeconds = FMath::Max(0.15f, Settings->MusicCrossfadeSeconds);
	}
}

void AMarsMusicActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLoops();
	Super::EndPlay(EndPlayReason);
}

USoundBase* AMarsMusicActor::ResolveTrack(const TCHAR* FileStem, bool bLoop, TArray<uint8>& OutPcmForPump, int32& OutSampleRate, int32& OutChannels, float& OutDuration)
{
	OutDuration = 1.f;
	OutSampleRate = 44100;
	OutChannels = 2;
	OutPcmForPump.Reset();

	const FSoftObjectPath Asset(FString::Printf(TEXT("/Game/Audio/Music/%s.%s"), FileStem, FileStem));
	if (Asset.IsValid())
	{
		if (USoundWave* Wave = Cast<USoundWave>(Asset.TryLoad()))
		{
			Wave->bLooping = bLoop;
			const float WaveDuration = Wave->GetDuration();
			OutDuration = (WaveDuration > 0.f && WaveDuration < 600.f) ? WaveDuration : OutDuration;
			return Wave;
		}
	}

	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("RawAudio"), TEXT("Music"), FString(FileStem) + TEXT(".wav"));
	TArray<uint8> Pcm;
	int32 Rate = 0, Channels = 0;
	float Duration = 0.f;
	if (!LoadPcm16Wav(Path, Pcm, Rate, Channels, Duration))
	{
		UE_LOG(LogMars, Warning, TEXT("Music track missing or unreadable: %s"), *Path);
		return nullptr;
	}

	OutPcmForPump = MoveTemp(Pcm);
	OutSampleRate = Rate;
	OutChannels = Channels;
	OutDuration = Duration;

	USoundWaveProcedural* Proc = NewObject<USoundWaveProcedural>(this);
	Proc->SetSampleRate(Rate);
	Proc->NumChannels = Channels;
	Proc->Duration = bLoop ? 100000.f : Duration; // long duration so a loop isn't treated as finished
	Proc->SoundGroup = SOUNDGROUP_Music;
	Proc->bLooping = false;
	// Prime a couple of loops (or the whole sting) so playback starts immediately.
	const int32 Copies = bLoop ? 2 : 1;
	for (int32 i = 0; i < Copies; ++i)
	{
		Proc->QueueAudio(OutPcmForPump.GetData(), OutPcmForPump.Num());
	}
	return Proc;
}

void AMarsMusicActor::Pump(USoundWaveProcedural* Proc, const TArray<uint8>& LoopPcm)
{
	if (!Proc || LoopPcm.Num() == 0)
	{
		return;
	}
	const int32 Want = LoopPcm.Num(); // keep at least one full loop queued
	while (Proc->GetAvailableAudioByteCount() < Want)
	{
		Proc->QueueAudio(LoopPcm.GetData(), LoopPcm.Num());
	}
}

void AMarsMusicActor::StartAmbient()
{
	float Duration = 0.f;
	int32 Rate = 44100, Channels = 2;
	USoundBase* Sound = ResolveTrack(TEXT("MUS_AmbientMars"), true, AmbientPcm, Rate, Channels, Duration);
	AmbientProc = Cast<USoundWaveProcedural>(Sound);
	if (AmbientComp && Sound)
	{
		AmbientComp->SetSound(Sound);
		AmbientComp->SetVolumeMultiplier(0.f);
		AmbientComp->Play();
	}
	AmbientTarget = AmbientLevel;
	BattleTarget = 0.f;
	bBattleStarted = false;
	bVictoryPlayed = false;
	ApplyVolumes();
	MarsShowLog(TEXT("Music: ambient Mars loop"));
}

void AMarsMusicActor::NotifyFirstShot()
{
	if (bBattleStarted || bVictoryPlayed)
	{
		return;
	}
	bBattleStarted = true;

	if (!BattleComp->IsPlaying())
	{
		float Duration = 0.f;
		int32 Rate = 44100, Channels = 2;
		USoundBase* Sound = ResolveTrack(TEXT("MUS_Battle"), true, BattlePcm, Rate, Channels, Duration);
		BattleProc = Cast<USoundWaveProcedural>(Sound);
		if (BattleComp && Sound)
		{
			BattleComp->SetSound(Sound);
			BattleComp->SetVolumeMultiplier(0.f);
			BattleComp->Play();
		}
	}
	AmbientTarget = 0.f;
	BattleTarget = BattleLevel;
	MarsShowLog(TEXT("Music: battle loop (first shot)"));
}

void AMarsMusicActor::PlayVictory()
{
	if (bVictoryPlayed)
	{
		return;
	}
	bVictoryPlayed = true;
	AmbientTarget = 0.f;
	BattleTarget = 0.f;

	TArray<uint8> Unused;
	float Duration = 0.f;
	int32 Rate = 44100, Channels = 2;
	USoundBase* Sound = ResolveTrack(TEXT("MUS_VictorySting"), false, Unused, Rate, Channels, Duration);
	if (StingComp && Sound)
	{
		StingComp->SetSound(Sound);
		StingComp->SetVolumeMultiplier(MasterVolume * StingLevel);
		StingComp->Play();
	}
	MarsShowLog(TEXT("Music: victory sting"));
}

void AMarsMusicActor::NotifyVoice(float DurationSeconds)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	DuckUntil = FMath::Max(DuckUntil, Now + FMath::Max(0.2f, DurationSeconds));
}

void AMarsMusicActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Pump(AmbientProc, AmbientPcm);
	Pump(BattleProc, BattlePcm);

	const float FadeRate = 1.f / FMath::Max(0.05f, CrossfadeSeconds);
	AmbientVol = FMath::FInterpConstantTo(AmbientVol, AmbientTarget, DeltaSeconds, FadeRate * FMath::Max(AmbientLevel, 0.01f));
	BattleVol = FMath::FInterpConstantTo(BattleVol, BattleTarget, DeltaSeconds, FadeRate * FMath::Max(BattleLevel, 0.01f));

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float DuckTarget = (DuckUntil > 0.f && Now < DuckUntil) ? DuckLevel : 1.f;
	DuckCurrent = FMath::FInterpTo(DuckCurrent, DuckTarget, DeltaSeconds, 8.f);

	if (bVictoryPlayed && AmbientVol < 0.01f && BattleVol < 0.01f)
	{
		// Loops have faded out; stop them so they don't keep pumping under the sting.
		if (AmbientComp && AmbientComp->IsPlaying() && AmbientTarget <= 0.f)
		{
			AmbientComp->Stop();
		}
		if (BattleComp && BattleComp->IsPlaying() && BattleTarget <= 0.f)
		{
			BattleComp->Stop();
		}
	}

	ApplyVolumes();
}

void AMarsMusicActor::ApplyVolumes()
{
	if (AmbientComp)
	{
		AmbientComp->SetVolumeMultiplier(MasterVolume * AmbientVol * DuckCurrent);
	}
	if (BattleComp)
	{
		BattleComp->SetVolumeMultiplier(MasterVolume * BattleVol * DuckCurrent);
	}
}

void AMarsMusicActor::StopLoops()
{
	if (AmbientComp) { AmbientComp->Stop(); }
	if (BattleComp) { BattleComp->Stop(); }
	if (StingComp) { StingComp->Stop(); }
}
