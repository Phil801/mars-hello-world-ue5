#!/usr/bin/env python3
"""Regenerate the Kevin-hunt voice lines in RawAudio/.

Pipeline (fully offline once the voice model is downloaded):
  1. Piper TTS (https://github.com/rhasspy/piper, GPL-3.0 tool; generated audio is ours)
     with the en_US-libritts_r-medium voice (dataset LibriTTS-R, CC BY 4.0) and a
     different speaker id per character.
  2. Character effects in numpy (ring modulation, bit-crush, vibrato) and ffmpeg
     (pitch shift, echo, chorus, reverb-ish delays, loudness normalisation).

Keep the spoken texts in sync with Source/MarsHelloWorld/Dialogue/MarsDialogueTable.cpp.

The original one-line "Hello, world!" recordings live in RawAudio/Archive/HelloWorld/
and are not overwritten by this script.

Usage:
  python3 -m venv .venv && .venv/bin/pip install piper-tts numpy
  curl -L -o voice.onnx      https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/libritts_r/medium/en_US-libritts_r-medium.onnx
  curl -L -o voice.onnx.json https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/libritts_r/medium/en_US-libritts_r-medium.onnx.json
  .venv/bin/python Tools/voice/generate_voices.py --model voice.onnx --out RawAudio
Requires ffmpeg on PATH.

Optional: --only Martian_ArrivalHello  (regenerate one file)
"""
import argparse, os, subprocess, tempfile, wave
import numpy as np

SR = 44100

# name: (speaker id, piper length_scale, {line_id: text})
CHARACTERS = {
    "Martian": (700, 1.35, {
        "ArrivalHello": "Hello world, I'm looking for Kevin.",
        "ArrivalSeenKevin": "Have you seen Kevin?",
        "ArrivalVariant": "Hello, world! Kevin was supposed to meet me here.",
        "AskAreYouKevin": "Are you Kevin?",
        "AskAreYouKevinVariant": "Wait. Are you Kevin?",
        "DenyPhil": "I'm Phil.",
        "DenyTyler": "I'm Tyler.",
        "DenyOther": "Kevin? No. Not me.",
        "KeepLooking": "Still looking for Kevin.",
        "Accuse": "Wait... you're Kevin!",
        "AccuseVariant": "That's Kevin! Get him!",
        "AllAreKevin": "You're all Kevin!",
        "KillKevin": "I killed Kevin!",
        "KillKevinVariant": "Got him! I killed Kevin!",
    }),
    "XBot": (333, 1.05, {
        "ArrivalHello": "Hello, world. I'm looking for Kevin.",
        "ArrivalSeenKevin": "Have you seen Kevin?",
        "ArrivalVariant": "Hello, world. Scanning for Kevin.",
        "AskAreYouKevin": "Are you Kevin?",
        "AskAreYouKevinVariant": "Identify. Are you Kevin?",
        "DenyPhil": "I'm Phil.",
        "DenyTyler": "I'm Tyler.",
        "DenyOther": "Negative. I am not Kevin.",
        "KeepLooking": "Kevin is not in this scan.",
        "Accuse": "Target identified. You are Kevin.",
        "AccuseVariant": "Kevin confirmed. Engaging.",
        "AllAreKevin": "Recalculating. All units are Kevin.",
        "KillKevin": "I killed Kevin!",
        "KillKevinVariant": "Kevin eliminated.",
    }),
    "Venusian": (850, 1.20, {
        "ArrivalHello": "Hello world, I'm looking for Kevin.",
        "ArrivalSeenKevin": "Have you seen Kevin?",
        "ArrivalVariant": "Hello, world! I'm looking for someone called Kevin.",
        "AskAreYouKevin": "Are you Kevin?",
        "AskAreYouKevinVariant": "You there... are you Kevin?",
        "DenyPhil": "I'm Phil.",
        "DenyTyler": "I'm Tyler.",
        "DenyOther": "Oh, no, I'm not Kevin.",
        "KeepLooking": "Has anyone seen Kevin?",
        "Accuse": "I knew it! You're Kevin!",
        "AccuseVariant": "Kevin! I found Kevin!",
        "AllAreKevin": "Wait... you're ALL Kevin!",
        "KillKevin": "I killed Kevin!",
        "KillKevinVariant": "I killed Kevin! Finally!",
    }),
}

# ffmpeg filter chains applied after the numpy stage
FFMPEG = {
    # deep, slow and warbly: pitch down ~4 semitones, wobble, cavernous echo
    "Martian": "asetrate=44100*0.79,aresample=44100,atempo=1.08,vibrato=f=6.5:d=0.35,"
               "aecho=0.8:0.7:90|180:0.35|0.2,lowpass=f=3800,loudnorm=I=-16:TP=-1.5",
    # robotic: tight metallic comb + band-limit (ring-mod/bitcrush done in numpy)
    "XBot":    "aecho=0.9:0.9:7|13:0.55|0.35,highpass=f=180,lowpass=f=6000,loudnorm=I=-16:TP=-1.5",
    # high and ethereal: pitch up ~5 semitones, chorus shimmer, long airy tail
    "Venusian": "asetrate=44100*1.33,aresample=44100,atempo=0.85,"
                "chorus=0.6:0.9:50|60|70:0.4|0.32|0.3:0.25|0.4|0.3:2|2.3|1.3,"
                "aecho=0.8:0.6:240|480|720:0.35|0.25|0.15,highpass=f=220,loudnorm=I=-16:TP=-1.5",
}


def read_wav(path):
    with wave.open(path) as w:
        sr = w.getframerate()
        x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float32) / 32768.0
    return x, sr


def write_wav(path, x, sr):
    x = np.clip(x, -1.0, 1.0)
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
        w.writeframes((x * 32767).astype(np.int16).tobytes())


def robotize(x, sr):
    t = np.arange(len(x)) / sr
    carrier = np.sin(2 * np.pi * 55.0 * t)           # ring modulator -> classic robot buzz
    y = 0.65 * x * carrier + 0.35 * x                # keep some dry signal for intelligibility
    steps = 2 ** 6                                   # 6-bit crush
    y = np.round(y * steps) / steps
    hold = 3                                         # sample-rate reduction (zero-order hold)
    y = np.repeat(y[::hold], hold)[: len(x)]
    return y


def synthesize(piper, model, name, spk, ls, text, out_path, tmp):
    raw = os.path.join(tmp, f"{name}_raw.wav")
    subprocess.run([piper, "-m", model, "--speaker", str(spk), "--length_scale", str(ls),
                    "--sentence_silence", "0.0", "-f", raw], input=text.encode(), check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    up = os.path.join(tmp, f"{name}_44k.wav")
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", raw, "-ar", str(SR), "-ac", "1", up], check=True)
    x, sr = read_wav(up)
    if name == "XBot":
        x = robotize(x, sr)
    mid = os.path.join(tmp, f"{name}_mid.wav")
    write_wav(mid, np.concatenate([np.zeros(int(0.05 * sr)), x, np.zeros(int(0.45 * sr))]), sr)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", mid, "-af", FFMPEG[name],
                    "-ar", str(SR), "-ac", "1", "-c:a", "pcm_s16le", out_path], check=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True)
    ap.add_argument("--out", default="RawAudio")
    ap.add_argument("--piper", default="piper")
    ap.add_argument("--only", default="", help="Regenerate one line: Character_LineId (e.g. Martian_ArrivalHello)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    wanted = a.only.strip() or None
    wrote = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name, (spk, ls, lines) in CHARACTERS.items():
            for line_id, text in lines.items():
                key = f"{name}_{line_id}"
                if wanted and key != wanted:
                    continue
                out = os.path.join(a.out, f"VO_{name}_{line_id}.wav")
                synthesize(a.piper, a.model, name, spk, ls, text, out, tmp)
                print("wrote", out)
                wrote += 1
    if wanted and wrote == 0:
        raise SystemExit(f"no line matched --only {wanted}")
    print(f"done ({wrote} files)")


if __name__ == "__main__":
    main()
