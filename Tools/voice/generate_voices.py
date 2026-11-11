#!/usr/bin/env python3
"""Regenerate the three "Hello world" voice lines in RawAudio/.

Pipeline (fully offline once the voice model is downloaded):
  1. Piper TTS (https://github.com/rhasspy/piper, GPL-3.0 tool; generated audio is ours)
     with the en_US-libritts_r-medium voice (dataset LibriTTS-R, CC BY 4.0) and a
     different speaker id per character.
  2. Character effects in numpy (ring modulation, bit-crush, vibrato) and ffmpeg
     (pitch shift, echo, chorus, reverb-ish delays, loudness normalisation).

Usage:
  python3 -m venv .venv && .venv/bin/pip install piper-tts numpy
  curl -L -o voice.onnx      https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/libritts_r/medium/en_US-libritts_r-medium.onnx
  curl -L -o voice.onnx.json https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/libritts_r/medium/en_US-libritts_r-medium.onnx.json
  .venv/bin/python Tools/voice/generate_voices.py --model voice.onnx --out RawAudio
Requires ffmpeg on PATH.
"""
import argparse, os, subprocess, tempfile, wave
import numpy as np

SR = 44100

CHARACTERS = {
    # name: (speaker id, piper length_scale, text)
    "Martian":  (700, 1.35, "Hello, world!"),
    "XBot":     (333, 1.05, "Hello, world."),
    "Venusian": (850, 1.20, "Hello, world!"),
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True)
    ap.add_argument("--out", default="RawAudio")
    ap.add_argument("--piper", default="piper")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        for name, (spk, ls, text) in CHARACTERS.items():
            raw = os.path.join(tmp, f"{name}_raw.wav")
            subprocess.run([a.piper, "-m", a.model, "--speaker", str(spk), "--length_scale", str(ls),
                            "--sentence_silence", "0.0", "-f", raw], input=text.encode(), check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            up = os.path.join(tmp, f"{name}_44k.wav")
            subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", raw, "-ar", str(SR), "-ac", "1", up], check=True)
            x, sr = read_wav(up)
            if name == "XBot":
                x = robotize(x, sr)
            mid = os.path.join(tmp, f"{name}_mid.wav")
            write_wav(mid, np.concatenate([np.zeros(int(0.05 * sr)), x, np.zeros(int(0.6 * sr))]), sr)
            out = os.path.join(a.out, f"VO_{name}_HelloWorld.wav")
            subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", mid, "-af", FFMPEG[name],
                            "-ar", str(SR), "-ac", "1", "-c:a", "pcm_s16le", out], check=True)
            print("wrote", out)


if __name__ == "__main__":
    main()
