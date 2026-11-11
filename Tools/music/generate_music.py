#!/usr/bin/env python3
"""Generate original, copyright-free background music for Mars Hello World.

All audio is synthesized with numpy. No samples, no downloads, no copyrighted
material. Output is 44.1 kHz, 16-bit stereo PCM WAV, loudness-normalised.

  MUS_AmbientMars.wav  – calm looping bed for arrivals / the Kevin search
  MUS_Battle.wav       – 132 BPM driving loop (kick, snare, hats, bass, lead)
                         that the game crossfades in on the first shot
  MUS_VictorySting.wav – short one-shot sting at victory (not looped)

Loops are integer bars (battle) or an overlap-add seam (ambient) so they
repeat without a click.

Usage:
  python3 Tools/music/generate_music.py --out RawAudio/Music
"""
from __future__ import annotations

import argparse
import os
import wave

import numpy as np

SR = 44100


# --- primitives ----------------------------------------------------------------

def time_vec(n):
    return np.arange(n, dtype=np.float64) / SR


def midi(n):
    return 440.0 * (2.0 ** ((n - 69) / 12.0))


def env_exp(n, attack=0.003, decay=0.2, amp=1.0):
    t = time_vec(n)
    a = np.clip(t / max(attack, 1e-4), 0.0, 1.0)
    d = np.exp(-t / max(decay, 1e-4))
    return (a * d * amp).astype(np.float64)


def ads(n, a=0.01, d=0.08, s=0.7, r=0.12, hold=None):
    """Simple ADSR. `hold` is sustain length in seconds; default fills the rest."""
    t = time_vec(n)
    dur = n / SR
    if hold is None:
        hold = max(0.0, dur - a - d - r)
    out = np.zeros(n, dtype=np.float64)
    for i, ti in enumerate(t):
        if ti < a:
            out[i] = ti / max(a, 1e-6)
        elif ti < a + d:
            out[i] = 1.0 - (1.0 - s) * ((ti - a) / max(d, 1e-6))
        elif ti < a + d + hold:
            out[i] = s
        else:
            out[i] = s * max(0.0, 1.0 - (ti - a - d - hold) / max(r, 1e-6))
    return out


def _one_pole(x, cutoff, high=False, sr=SR):
    """Tight 1-pole LP (or HP as x - lp). cutoff is a scalar Hz."""
    x = np.asarray(x, dtype=np.float64)
    if x.ndim == 2:
        return np.column_stack([_one_pole(x[:, c], cutoff, high, sr) for c in range(x.shape[1])])
    coeff = np.exp(-2.0 * np.pi * max(float(np.mean(cutoff)), 10.0) / sr)
    y = np.empty_like(x)
    acc = 0.0
    keep, put = coeff, 1.0 - coeff
    for i, v in enumerate(x):
        acc = acc * keep + v * put
        y[i] = acc
    return (x - y) if high else y


def biquad_lp(x, cutoff, q=0.7, sr=SR):
    return _one_pole(x, cutoff, high=False, sr=sr)


def biquad_hp(x, cutoff, q=0.7, sr=SR):
    return _one_pole(x, cutoff, high=True, sr=sr)


def sine(freq, n, phase=0.0):
    return np.sin(2 * np.pi * freq * time_vec(n) + phase)


def saw(freq, n, phase=0.0):
    t = time_vec(n)
    return 2.0 * ((t * freq + phase / (2 * np.pi)) % 1.0) - 1.0


def square(freq, n, phase=0.0):
    return np.sign(sine(freq, n, phase) + 1e-12)


def noise(n, rng):
    return rng.uniform(-1.0, 1.0, size=n)


def mix_stereo(layers):
    """layers: list of (array mono-or-stereo, pan -1..1, gain)."""
    length = max(len(a) if a.ndim == 1 else a.shape[0] for a, _, _ in layers)
    out = np.zeros((length, 2), dtype=np.float64)
    for arr, pan, gain in layers:
        if arr.ndim == 1:
            n = len(arr)
            left = arr * (0.5 - 0.5 * pan) ** 0.5 * 1.414  # equal-ish power
            # actually equal power: L = cos((pan+1)*pi/4), R = sin((pan+1)*pi/4)
            ang = (pan + 1.0) * 0.25 * np.pi
            left = arr * np.cos(ang)
            right = arr * np.sin(ang)
            out[:n, 0] += left * gain
            out[:n, 1] += right * gain
        else:
            n = arr.shape[0]
            out[:n] += arr * gain
    return out


def soft_clip(x, drive=1.0):
    return np.tanh(x * drive)


def normalize(x, peak=0.89):
    m = np.max(np.abs(x))
    if m < 1e-9:
        return x
    return x * (peak / m)


def loop_seam(x, fade_s=0.08):
    """Overlap-add the tail onto the head so a cycle repeats without a click."""
    fade = int(fade_s * SR)
    fade = min(fade, x.shape[0] // 8)
    if fade < 8:
        return x
    w = np.linspace(0.0, 1.0, fade)
    w = w * w * (3 - 2 * w)  # smoothstep
    if x.ndim == 1:
        body = x[:-fade].copy()
        body[:fade] = body[:fade] * w + x[-fade:] * (1 - w)
        return body
    body = x[:-fade].copy()
    ww = w[:, None]
    body[:fade] = body[:fade] * ww + x[-fade:] * (1 - ww)
    return body


def write_wav(path, x, sr=SR):
    x = np.asarray(x, dtype=np.float64)
    if x.ndim == 1:
        x = np.stack([x, x], axis=1)
    x = np.clip(x, -1.0, 1.0)
    pcm = (x * 32767.0).astype(np.int16)
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm.tobytes())
    print(f"wrote {path}  ({x.shape[0] / sr:.2f}s)")


def add_at(dest, src, at, gain=1.0):
    if src.ndim == 1 and dest.ndim == 2:
        src = np.stack([src, src], axis=1)
    n = src.shape[0]
    start = int(at)
    end = min(dest.shape[0], start + n)
    if start >= dest.shape[0] or end <= start:
        return
    dest[start:end] += src[: end - start] * gain


# --- instruments ---------------------------------------------------------------

def kick(rng, dur=0.32):
    n = int(dur * SR)
    t = time_vec(n)
    freq = 140 * np.exp(-t * 18) + 38
    phase = 2 * np.pi * np.cumsum(freq) / SR
    body = np.sin(phase) * np.exp(-t * 8.5)
    click = biquad_hp(rng.uniform(-1, 1, n) * np.exp(-t * 80), 2000, 0.6) * 0.4
    return soft_clip(body * 1.4 + click, 1.3)


def snare(rng, dur=0.28):
    n = int(dur * SR)
    t = time_vec(n)
    tone = sine(190, n) * np.exp(-t * 18) * 0.35
    noise_b = biquad_hp(rng.uniform(-1, 1, n), 1200, 0.6) * np.exp(-t * 14)
    return soft_clip(tone + noise_b * 0.9, 1.2)


def hat(rng, dur=0.07, open_=False):
    n = int(dur * SR)
    t = time_vec(n)
    decay = 9 if open_ else 55
    x = biquad_hp(rng.uniform(-1, 1, n), 6000, 0.5) * np.exp(-t * decay)
    return x * (0.45 if open_ else 0.28)


def tom(freq, dur=0.25):
    n = int(dur * SR)
    t = time_vec(n)
    f = freq * np.exp(-t * 6)
    phase = 2 * np.pi * np.cumsum(f) / SR
    return np.sin(phase) * np.exp(-t * 9) * 0.6


def bass_note(freq, dur, rng):
    n = int(dur * SR)
    osc = 0.6 * saw(freq, n) + 0.3 * saw(freq * 1.002, n, 0.3) + 0.2 * sine(freq, n)
    osc = biquad_lp(osc, 420, 0.8)
    e = ads(n, a=0.008, d=0.06, s=0.75, r=min(0.08, dur * 0.3))
    return soft_clip(osc * e, 1.1)


def lead_note(freq, dur, rng, brightness=1800):
    n = int(dur * SR)
    osc = 0.55 * saw(freq, n) + 0.35 * saw(freq * 1.007, n, 1.2) + 0.2 * square(freq * 2, n)
    osc = biquad_lp(osc, brightness, 0.9)
    e = ads(n, a=0.01, d=0.08, s=0.55, r=min(0.12, dur * 0.35))
    return osc * e


def pad_chord(freqs, n, rng, cutoff=700):
    t = time_vec(n)
    x = np.zeros(n, dtype=np.float64)
    for i, f in enumerate(freqs):
        x += 0.22 * sine(f, n, i * 0.4)
        x += 0.12 * saw(f * (1.0 + 0.0015 * (i - 1)), n, i)
    # slow amplitude breathe
    x *= 0.6 + 0.4 * np.sin(2 * np.pi * t / 7.0 + 0.3)
    x = biquad_lp(x, cutoff, 0.6)
    # fade in/out of the block itself; the loop seam handles the wrap
    fade = int(0.4 * SR)
    fade = min(fade, n // 6)
    w = np.linspace(0, 1, fade)
    w = w * w * (3 - 2 * w)
    x[:fade] *= w
    x[-fade:] *= w[::-1]
    return x


# --- pieces --------------------------------------------------------------------

def make_ambient(rng):
    # ~40 s bed in D dorian, then a 2 s seam so it loops.
    seconds = 42.0
    n = int(seconds * SR)
    out = np.zeros((n, 2), dtype=np.float64)

    # Drones
    t = time_vec(n)
    drone = 0.35 * sine(midi(38), n) + 0.22 * sine(midi(45), n) + 0.08 * sine(midi(50), n)
    drone *= 0.75 + 0.25 * np.sin(2 * np.pi * t / 13.0)
    add_at(out, mix_stereo([(drone, 0.0, 1.0)]), 0)

    # Wind / dust
    wind = biquad_lp(biquad_hp(rng.uniform(-1, 1, n), 180, 0.5), 900, 0.5)
    wind *= 0.09 * (0.7 + 0.3 * np.sin(2 * np.pi * t / 9.5 + 1.1))
    add_at(out, mix_stereo([(wind, -0.15, 1.0), (np.roll(wind, 2048) * 0.7, 0.25, 1.0)]), 0)

    # Slow pad chords: Dm, Bb, C, Am  (each ~8 s with overlap)
    chords = [
        (0.0,  [midi(50), midi(53), midi(57), midi(60)]),   # D F A C
        (10.0, [midi(46), midi(50), midi(53), midi(57)]),   # Bb D F A
        (20.0, [midi(48), midi(52), midi(55), midi(60)]),   # C E G C
        (30.0, [midi(45), midi(48), midi(52), midi(57)]),   # A C E A
    ]
    for start, freqs in chords:
        block = pad_chord(freqs, int(11.5 * SR), rng, cutoff=640)
        stereo = mix_stereo([
            (block, -0.35, 0.7),
            (np.roll(block, 1100) * 0.85, 0.4, 0.7),
        ])
        add_at(out, stereo, int(start * SR))

    # Sparse high motif (a falling fifth, very quiet)
    motif_times = [4.5, 15.8, 27.0, 36.6]
    motif_notes = [midi(74), midi(69), midi(67), midi(62)]
    for tm, f in zip(motif_times, motif_notes):
        note = sine(f, int(2.4 * SR)) * ads(int(2.4 * SR), a=0.15, d=0.4, s=0.35, r=1.2)
        add_at(out, mix_stereo([(note, 0.2, 0.16)]), int(tm * SR))

    # Distant soft pulse, not a groove — just a heartbeat of the planet
    for tm in np.arange(2.0, seconds - 2.0, 5.5):
        p = sine(48, int(0.8 * SR)) * np.exp(-time_vec(int(0.8 * SR)) * 5)
        add_at(out, mix_stereo([(p, 0.0, 0.18)]), int(tm * SR))

    out = soft_clip(out, 0.9)
    out = normalize(out, 0.72)
    out = loop_seam(out, fade_s=2.0)
    return out


def make_battle(rng):
    bpm = 132.0
    beat = 60.0 / bpm
    bars = 16
    beats = bars * 4
    seconds = beats * beat  # exactly 64 beats at 132 BPM = 29.0909...
    n = int(round(seconds * SR))
    # snap to exact samples-per-beat * beats so the loop lands on a kick
    spb = int(round(beat * SR))
    n = spb * beats
    seconds = n / SR

    drums = np.zeros((n, 2), dtype=np.float64)
    bass = np.zeros((n, 2), dtype=np.float64)
    lead = np.zeros((n, 2), dtype=np.float64)
    pad = np.zeros((n, 2), dtype=np.float64)
    duck = np.ones(n, dtype=np.float64)

    def at_beat(b):
        return int(round(b * spb))

    # Kick: four-on-the-floor, plus an extra on the "and" of 3 every other bar
    for b in range(beats):
        add_at(drums, mix_stereo([(kick(rng), 0.0, 1.0)]), at_beat(b), 1.0)
        # duck envelope for bass/pad
        start = at_beat(b)
        dn = int(0.22 * SR)
        end = min(n, start + dn)
        duck[start:end] *= np.linspace(0.25, 1.0, end - start)
        if (b % 8) == 6:
            add_at(drums, mix_stereo([(kick(rng), 0.0, 0.7)]), at_beat(b + 0.5), 0.85)

    # Snare on 2 and 4
    for b in range(beats):
        if (b % 4) in (1, 3):
            add_at(drums, mix_stereo([(snare(rng), 0.05, 1.0)]), at_beat(b), 0.95)

    # Hats: 8th notes, accent offbeats; open hat on the last offbeat of even bars
    for b in range(beats * 2):
        tbeat = b * 0.5
        open_ = (b % 16) == 15
        h = hat(rng, dur=0.18 if open_ else 0.06, open_=open_)
        gain = 0.7 if (b % 2 == 1) else 0.45
        pan = -0.15 if (b % 2 == 0) else 0.2
        add_at(drums, mix_stereo([(h, pan, 1.0)]), at_beat(tbeat), gain)

    # Occasional low tom fill at the end of bar 8 and 16
    for bar in (7, 15):
        add_at(drums, mix_stereo([(tom(midi(50), 0.2), -0.3, 0.8)]), at_beat(bar * 4 + 2.0))
        add_at(drums, mix_stereo([(tom(midi(47), 0.22), 0.25, 0.8)]), at_beat(bar * 4 + 2.5))
        add_at(drums, mix_stereo([(tom(midi(43), 0.28), 0.0, 0.9)]), at_beat(bar * 4 + 3.0))

    # Bass riff in E minor, two-bar cell
    # E E G A | B A G E  (eighths, some held)
    bass_cell = [
        (0.0, midi(28), 0.5), (0.5, midi(28), 0.5),
        (1.0, midi(31), 0.5), (1.5, midi(33), 0.5),
        (2.0, midi(35), 0.5), (2.5, midi(33), 0.5),
        (3.0, midi(31), 0.5), (3.5, midi(28), 0.5),
        (4.0, midi(28), 0.75),
        (5.0, midi(31), 0.5), (5.5, midi(28), 0.5),
        (6.0, midi(33), 0.5), (6.5, midi(35), 0.5),
        (7.0, midi(36), 0.5), (7.5, midi(35), 0.5),
    ]
    for bar0 in range(0, bars, 2):
        for off, freq, dur in bass_cell:
            note = bass_note(freq, dur * beat * 0.96, rng)
            add_at(bass, mix_stereo([(note, 0.0, 1.0)]), at_beat(bar0 * 4 + off), 1.0)

    # Sidechain the bass
    bass[:, 0] *= duck
    bass[:, 1] *= duck

    # Pad stabs, one chord per two bars
    pad_chords = [
        [midi(52), midi(55), midi(59)],  # E G B
        [midi(48), midi(52), midi(55)],  # C E G
        [midi(53), midi(57), midi(60)],  # F A C
        [midi(50), midi(55), midi(59)],  # D G B
    ]
    for i, bar0 in enumerate(range(0, bars, 2)):
        freqs = pad_chords[i % 4]
        block = pad_chord(freqs, int(2 * 4 * beat * SR * 0.95), rng, cutoff=900)
        stereo = mix_stereo([(block, -0.4, 0.35), (np.roll(block, 700), 0.4, 0.35)])
        add_at(pad, stereo, at_beat(bar0 * 4))
    pad[:, 0] *= 0.5 + 0.5 * duck
    pad[:, 1] *= 0.5 + 0.5 * duck

    # Synth lead: punchy hook, answers every other bar
    # E4 G4 A4 B4  A4 G4 E4 D4   then a higher variant
    lead_a = [(0, 64), (1, 67), (2, 69), (3, 71), (4, 69), (5, 67), (6, 64), (7, 62)]
    lead_b = [(0, 71), (1, 74), (2, 71), (3, 69), (4, 67), (5, 69), (6, 71), (7, 64)]
    for bar0 in range(0, bars, 2):
        pattern = lead_a if (bar0 // 2) % 2 == 0 else lead_b
        for off, note_n in pattern:
            dur = beat * 0.92
            note = lead_note(midi(note_n), dur, rng, brightness=2200 if bar0 >= 8 else 1700)
            # a short slap delay
            delayed = np.zeros(len(note) + int(0.23 * SR))
            delayed[: len(note)] += note
            delayed[int(0.23 * SR) : int(0.23 * SR) + len(note)] += note * 0.28
            add_at(lead, mix_stereo([(delayed, 0.18, 0.72)]), at_beat(bar0 * 4 + off))

    out = drums * 0.95 + bass * 0.85 + lead * 1.0 + pad * 0.7
    out = soft_clip(out, 0.95)
    out = normalize(out, 0.86)
    # Tiny seam in case of accumulated DC / filter state
    out = loop_seam(out, fade_s=0.006)
    return out, 132


def make_sting(rng):
    bpm = 132.0
    beat = 60.0 / bpm
    seconds = 4.2
    n = int(seconds * SR)
    out = np.zeros((n, 2), dtype=np.float64)

    # Rising hits: G, B, D, G  then a held G major
    hits = [(0.00, 55), (0.28, 59), (0.56, 62), (0.84, 67)]
    for tm, note_n in hits:
        k = kick(rng, 0.28)
        add_at(out, mix_stereo([(k, 0.0, 0.7)]), int(tm * SR))
        ln = lead_note(midi(note_n), 0.45, rng, brightness=2600)
        add_at(out, mix_stereo([(ln, -0.1, 0.9)]), int(tm * SR))
        ln2 = lead_note(midi(note_n + 12), 0.4, rng, brightness=2800)
        add_at(out, mix_stereo([(ln2, 0.2, 0.5)]), int(tm * SR))

    # Resolve: G major pile-up
    start = int(1.2 * SR)
    for f in (midi(43), midi(47), midi(50), midi(55), midi(59), midi(62), midi(67)):
        nlen = int(2.6 * SR)
        tone = sine(f, nlen) * ads(nlen, a=0.02, d=0.2, s=0.55, r=1.6)
        add_at(out, mix_stereo([(tone, 0.0, 0.22)]), start)
    # A bright last snare + hat
    add_at(out, mix_stereo([(snare(rng), 0.0, 0.8)]), start)
    add_at(out, mix_stereo([(hat(rng, 0.4, open_=True), 0.1, 1.0)]), start)

    # Fade the tail to silence
    fade = int(1.1 * SR)
    w = np.linspace(1.0, 0.0, fade)
    w = w * w
    out[-fade:] *= w[:, None]
    out = soft_clip(out, 0.9)
    return normalize(out, 0.88)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="RawAudio/Music")
    ap.add_argument("--seed", type=int, default=801)
    a = ap.parse_args()
    rng = np.random.default_rng(a.seed)
    os.makedirs(a.out, exist_ok=True)
    write_wav(os.path.join(a.out, "MUS_AmbientMars.wav"), make_ambient(rng))
    battle, bpm = make_battle(rng)
    write_wav(os.path.join(a.out, "MUS_Battle.wav"), battle)
    write_wav(os.path.join(a.out, "MUS_VictorySting.wav"), make_sting(rng))
    print(f"done (battle {bpm} BPM, seed {a.seed})")


if __name__ == "__main__":
    main()
