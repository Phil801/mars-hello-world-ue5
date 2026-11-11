// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Audio/MarsVoiceLibrary.h"
#include "Combat/CombatantTypes.h"
#include "MarsHelloWorld.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	struct FPcmWav
	{
		int32 SampleRate = 0;
		int32 NumChannels = 0;
		TArray<uint8> Pcm16; // interleaved little-endian int16
	};

	uint32 ReadU32(const TArray<uint8>& B, int32 At) { return B[At] | (B[At + 1] << 8) | (B[At + 2] << 16) | (uint32(B[At + 3]) << 24); }
	uint16 ReadU16(const TArray<uint8>& B, int32 At) { return uint16(B[At] | (B[At + 1] << 8)); }

	/** Minimal RIFF/WAVE reader for 16-bit PCM (what Tools/voice/generate_voices.py writes). */
	bool LoadPcm16Wav(const FString& Path, FPcmWav& Out)
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
				Out.NumChannels = ReadU16(Bytes, Body + 2);
				Out.SampleRate = int32(ReadU32(Bytes, Body + 4));
				const uint16 Bits = ReadU16(Bytes, Body + 14);
				if (Format != 1 || Bits != 16)
				{
					UE_LOG(LogMars, Warning, TEXT("%s is not 16-bit PCM (format %d, %d bits)"), *Path, Format, Bits);
					return false;
				}
				bHaveFormat = true;
			}
			else if (FMemory::Memcmp(Bytes.GetData() + Pos, "data", 4) == 0 && bHaveFormat)
			{
				Out.Pcm16.Append(Bytes.GetData() + Body, int32(ChunkSize));
				return Out.SampleRate > 0 && Out.NumChannels > 0;
			}
			Pos = Body + int32(ChunkSize) + (ChunkSize & 1); // chunks are word-aligned
		}
		return false;
	}
}

USoundBase* MarsVoice::ResolveVoice(UObject* Outer, const FCombatantProfile& Profile, float& OutDurationSeconds)
{
	OutDurationSeconds = 1.8f; // reading time for the subtitle when there is no audio

	// 1) Imported asset.
	if (Profile.VoiceAsset.IsValid())
	{
		if (USoundWave* Wave = Cast<USoundWave>(Profile.VoiceAsset.TryLoad()))
		{
			const float WaveDuration = Wave->GetDuration();
			OutDurationSeconds = (WaveDuration > 0.f && WaveDuration < 60.f) ? WaveDuration : OutDurationSeconds;
			return Wave;
		}
	}

	// 2) Raw WAV from disk -> procedural sound wave.
	if (!Profile.RawVoiceFile.IsEmpty())
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("RawAudio"), Profile.RawVoiceFile);
		FPcmWav Wav;
		if (LoadPcm16Wav(Path, Wav))
		{
			USoundWaveProcedural* Proc = NewObject<USoundWaveProcedural>(Outer);
			Proc->SetSampleRate(Wav.SampleRate);
			Proc->NumChannels = Wav.NumChannels;
			const int32 Frames = Wav.Pcm16.Num() / (2 * Wav.NumChannels);
			Proc->Duration = float(Frames) / float(Wav.SampleRate);
			Proc->SoundGroup = SOUNDGROUP_Voice;
			Proc->bLooping = false;
			Proc->QueueAudio(Wav.Pcm16.GetData(), Wav.Pcm16.Num());
			OutDurationSeconds = Proc->Duration;
			return Proc;
		}
		UE_LOG(LogMars, Warning, TEXT("Voice line not found or unreadable: %s (subtitle only)"), *Path);
	}

	return nullptr;
}
