#!/usr/bin/env python3
"""Generate original, copyright-free background music for Mars Hello World.

All audio is synthesized with numpy. No samples, no downloads, no copyrighted
material. Output is 44.1 kHz, 16-bit stereo PCM WAV, loudness-normalised.

  MUS_AmbientMars.wav  – calm looping bed for arrivals / the Kevin search
  MUS_Battle.wav       – 144 BPM industrial loop in E phrygian, 16 bars: distorted
                         kick, gated snare, metallic FM/ring-mod clangs, crushed
                         16th bass, power-chord stabs, siren lead and a riser;
                         the game crossfades it in on the first shot
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


# --- industrial battle kit (vectorized, numpy-only) ----------------------------
#
# Everything below is used only by make_battle. Filtering is done in the FFT
# domain (zero-phase, no per-sample Python loops). Whole-track buses are
# filtered *circularly*, which is exactly what a seamless loop wants.

def _legacy_battle_rng_advance(rng):
    """Consume the shared rng exactly as the old 132 BPM make_battle did, so the
    victory sting (generated after the battle) stays bit-identical."""
    for b in range(64):
        rng.uniform(-1, 1, int(0.32 * SR))          # kick click
        if (b % 8) == 6:
            rng.uniform(-1, 1, int(0.32 * SR))      # extra kick
    for b in range(64):
        if (b % 4) in (1, 3):
            rng.uniform(-1, 1, int(0.28 * SR))      # snare
    for b in range(128):
        open_ = (b % 16) == 15
        rng.uniform(-1, 1, int((0.18 if open_ else 0.06) * SR))  # hats


def fft_filter(x, lo=None, hi=None, order=2, circular=False, peak=None):
    """Zero-phase Butterworth-magnitude HP/LP (and optional resonant peak
    (freq, gain, q)). circular=True treats x as one loop cycle."""
    x = np.asarray(x, dtype=np.float64)
    n = x.shape[0]
    size = n if circular else 1 << int(np.ceil(np.log2(n + 2048)))
    X = np.fft.rfft(x, n=size, axis=0)
    f = np.fft.rfftfreq(size, 1.0 / SR)
    H = np.ones_like(f)
    if hi is not None:
        H /= np.sqrt(1.0 + (f / hi) ** (2 * order))
    if lo is not None:
        with np.errstate(divide="ignore"):
            H *= np.where(f > 0, 1.0 / np.sqrt(1.0 + (lo / np.maximum(f, 1e-9)) ** (2 * order)), 0.0)
    if peak is not None:
        pf, pg, pq = peak
        H *= 1.0 + pg / (1.0 + ((f - pf) / (pf / pq)) ** 2)
    if x.ndim == 2:
        H = H[:, None]
    y = np.fft.irfft(X * H, n=size, axis=0)
    return y[:n]


def drive(x, amount=4.0, bias=0.0):
    """Asymmetric tanh waveshaper (bias adds even-harmonic grit)."""
    y = np.tanh(amount * (x + bias)) - np.tanh(amount * bias)
    return y / np.tanh(amount)


def bitcrush(x, bits=6, hold=3):
    if hold > 1:
        x = np.repeat(x[::hold], hold, axis=0)[: x.shape[0]]
    q = 2.0 ** (bits - 1)
    return np.round(x * q) / q


def sweep_phase(freq):
    return 2 * np.pi * np.cumsum(freq) / SR


def add_wrap(dest, src, at, gain=1.0):
    """Add src into a loop buffer; anything past the end wraps to the start."""
    n = dest.shape[0]
    if src.ndim == 1 and dest.ndim == 2:
        src = np.stack([src, src], axis=1)
    start = int(at) % n
    m = min(src.shape[0], n)
    first = min(m, n - start)
    dest[start:start + first] += src[:first] * gain
    if m > first:
        dest[: m - first] += src[first:m] * gain


def pan2(x, pan=0.0, gain=1.0):
    ang = (pan + 1.0) * 0.25 * np.pi
    return np.stack([x * np.cos(ang) * gain, x * np.sin(ang) * gain], axis=1)


def ind_kick(rng, dur=0.42, punch=1.0):
    n = int(dur * SR)
    t = time_vec(n)
    freq = 230 * np.exp(-t * 32) + 95 * np.exp(-t * 9) + 41
    body = np.sin(sweep_phase(freq)) * np.exp(-t * 6.5)
    click = fft_filter(rng.uniform(-1, 1, n) * np.exp(-t * 140), lo=1500) * 0.9
    x = drive(body * 1.2 + click, 5.0 * punch, bias=0.08)
    x = fft_filter(x, lo=28, hi=9000)
    return x * np.minimum(1.0, (n - np.arange(n)) / 400.0)  # de-click tail


def ind_snare(rng, dur=0.3, gate=0.16):
    n = int(dur * SR)
    t = time_vec(n)
    nz = rng.uniform(-1, 1, n)
    body = np.sin(sweep_phase(260 * np.exp(-t * 30) + 175)) * np.exp(-t * 20)
    # gated noise: loud plateau, then a hard (but click-free) chop
    g = np.clip(1.0 - (t - gate) / 0.012, 0.0, 1.0) * (0.75 + 0.25 * np.exp(-t * 25))
    crack = fft_filter(nz, lo=900, hi=11000, peak=(3200, 2.0, 2.0)) * g
    ring = np.sin(2 * np.pi * 330 * t) * np.sin(2 * np.pi * 517 * t) * np.exp(-t * 22)
    x = drive(body * 0.8 + crack * 1.3 + ring * 0.35, 3.5)
    return bitcrush(x, bits=8, hold=2) * 0.9


def metal_hit(rng, f0, dur=0.5, decay=7.0, index=4.0, bright=1.0):
    """Inharmonic FM + ring-modulated squares: anvil / pipe / sheet metal."""
    n = int(dur * SR)
    t = time_vec(n)
    e = np.exp(-t * decay)
    mod = np.sin(2 * np.pi * f0 * 1.4142 * t) * index * np.exp(-t * decay * 1.5)
    fm = np.sin(2 * np.pi * f0 * t + mod)
    ratios = np.array([1.0, 1.483, 2.114, 2.756, 3.371, 4.07])
    sq = np.sign(np.sin(2 * np.pi * f0 * ratios[:, None] * t[None, :] + rng.uniform(0, 6.28, (6, 1))))
    rm = sq[0] * sq[1] + sq[2] * sq[3] + sq[4] * sq[5]
    rm = fft_filter(rm, lo=1800 * bright, hi=12000) * 0.25
    x = (fm * 0.8 + rm) * e
    x *= np.clip(t / 0.0015, 0, 1)
    return drive(x, 2.0)


def pipe_hit(f0, dur=0.6):
    """Struck free bar / pipe: modes at 1, 2.756, 5.404."""
    n = int(dur * SR)
    t = time_vec(n)
    x = (np.sin(2 * np.pi * f0 * t) * np.exp(-t * 6)
         + 0.6 * np.sin(2 * np.pi * f0 * 2.756 * t) * np.exp(-t * 11)
         + 0.4 * np.sin(2 * np.pi * f0 * 5.404 * t) * np.exp(-t * 18))
    return x * np.clip(t / 0.001, 0, 1)


def ind_hat(rng, dur=0.05, decay=70.0):
    n = int(dur * SR)
    t = time_vec(n)
    x = np.diff(rng.uniform(-1, 1, n + 1))  # crude bright HP
    x = fft_filter(x, lo=7000) * np.exp(-t * decay)
    return bitcrush(x, bits=7, hold=1)


def saw_bl(freq, n, phase=0.0, harm=None):
    """Naive saw is fine for grit; optional additive band-limit for dark tone."""
    if harm is None:
        return saw(freq, n, phase)
    t = time_vec(n)
    k = np.arange(1, harm + 1)[:, None]
    return (2 / np.pi) * np.sum(np.sin(2 * np.pi * freq * k * t[None, :] + phase * k) / k, axis=0)


def ind_bass(freq, dur):
    n = int(dur * SR)
    t = time_vec(n)
    osc = (saw(freq, n) * 0.5 + saw(freq * 1.009, n, 1.1) * 0.4
           + saw(freq * 0.995, n, 2.3) * 0.4 + np.sign(np.sin(2 * np.pi * freq * 0.5 * t)) * 0.35)
    e = np.clip(t / 0.003, 0, 1) * np.clip((n - np.arange(n)) / 250.0, 0, 1) * (0.55 + 0.45 * np.exp(-t * 18))
    return osc * e, np.exp(-t * 22)  # signal, filter-envelope


def stab_chord(root, dur, rng):
    """Detuned saw power chord (root, fifth, octave), hard-driven."""
    n = int(dur * SR)
    t = time_vec(n)
    x = np.zeros(n)
    for semi, g in ((0, 1.0), (7, 0.8), (12, 0.7), (-12, 0.5)):
        f = midi(root + semi)
        for d in (-0.012, 0.0, 0.011):
            x += g * saw(f * (1 + d), n, rng.uniform(0, 6.28))
    x = drive(x * 0.4, 6.0, bias=0.05)
    x = fft_filter(x, lo=90, hi=4200, peak=(1500, 1.5, 1.2))
    e = np.clip(t / 0.004, 0, 1) * np.exp(-t * 3.5) * np.clip((n - np.arange(n)) / 600.0, 0, 1)
    return x * e


def siren_note(n0, n1, dur):
    """Harsh detuned square lead with a pitch slide from n0 to n1."""
    n = int(dur * SR)
    t = time_vec(n)
    slide = np.clip(t / (dur * 0.5), 0, 1)
    f = midi(n0) * (midi(n1) / midi(n0)) ** slide
    f = f * (1 + 0.012 * np.sin(2 * np.pi * 6.5 * t))  # vibrato
    x = np.sign(np.sin(sweep_phase(f))) * 0.5 + np.sign(np.sin(sweep_phase(f * 1.006) + 1.0)) * 0.5
    x = drive(fft_filter(x, lo=300, hi=5500), 3.0)
    e = np.clip(t / 0.02, 0, 1) * np.clip((n - np.arange(n)) / 1200.0, 0, 1)
    return x * e


def hall_ir(rng, seconds=0.9, decay=5.0):
    n = int(seconds * SR)
    t = time_vec(n)
    ir = np.stack([rng.uniform(-1, 1, n), rng.uniform(-1, 1, n)], axis=1) * np.exp(-t * decay)[:, None]
    ir = fft_filter(ir, lo=250, hi=6000)
    ir[: int(0.012 * SR)] = 0.0  # pre-delay
    return ir / np.sqrt(np.sum(ir ** 2) / 2)


def circ_convolve(x, ir):
    n = x.shape[0]
    pad = np.zeros_like(x)
    pad[: ir.shape[0]] = ir[:n]
    return np.fft.irfft(np.fft.rfft(x, axis=0) * np.fft.rfft(pad, axis=0), n=n, axis=0)


def make_battle(rng):
    """Industrial battle loop: 144 BPM, E phrygian, 16 bars (8 drive + 8 rise).

    Uses its *own* rng (seeded separately) so the ambient/sting stay unchanged.
    Everything is rendered into loop-length buses; tails wrap to the start.
    """
    bpm = 144.0
    beat = 60.0 / bpm
    bars = 16
    beats = bars * 4
    spb = int(round(beat * SR))          # 18375 samples, exact at 144 BPM
    n = spb * beats
    s16 = spb / 4.0

    def at(b):                            # beat -> sample
        return int(round(b * spb))

    kick_bus = np.zeros((n, 2))
    drum_bus = np.zeros((n, 2))
    metal_bus = np.zeros((n, 2))
    bass_dry = np.zeros(n)
    bass_env = np.zeros(n)
    synth_bus = np.zeros((n, 2))
    fx_bus = np.zeros((n, 2))
    verb_send = np.zeros((n, 2))
    duck = np.ones(n)

    # ---- kick: four-on-the-floor + EBM 16th pickups; doubles in the rise ----
    kicks = []
    for bar in range(bars):
        rise = bar >= 8
        steps = [0, 4, 8, 12, 14] if bar % 2 == 0 else [0, 3, 4, 8, 11, 12]
        if rise:
            steps = sorted(set(steps + [7, 15] if bar % 2 == 0 else steps + [6, 15]))
        if bar == 15:
            steps = [0, 4, 8, 10, 12, 13, 14, 15]
        for s in steps:
            kicks.append((bar * 4 + s / 4.0, 1.0 if s % 4 == 0 else 0.75))
    for b, g in kicks:
        add_wrap(kick_bus, pan2(ind_kick(rng, punch=1.0 + 0.3 * g), 0.0, g), at(b))
        st = at(b)
        dn = int(0.16 * SR)
        idx = (st + np.arange(dn)) % n
        duck[idx] = np.minimum(duck[idx], np.linspace(0.15 + 0.5 * (1 - g), 1.0, dn) ** 0.7)

    # ---- snare: gated, crushed, on 2 & 4; ghost hits + roll in the rise ----
    for bar in range(bars):
        for s in (4, 12):
            sn = ind_snare(rng)
            add_wrap(drum_bus, pan2(sn, 0.05, 1.0), at(bar * 4 + s / 4))
            add_wrap(verb_send, pan2(sn, 0.0, 0.35), at(bar * 4 + s / 4))
        if bar >= 8 and bar % 2 == 1:
            add_wrap(drum_bus, pan2(ind_snare(rng, gate=0.05), -0.2, 0.45), at(bar * 4 + 3.75))
    # bar 15: 16th snare roll, rising
    for i in range(16):
        g = 0.25 + 0.75 * (i / 15.0) ** 1.5
        add_wrap(drum_bus, pan2(ind_snare(rng, gate=0.04), 0.3 * np.sin(i), g), at(15 * 4 + i / 4))

    # ---- hats: crushed 16th ticks, accented off-beats ----
    for st in range(beats * 4):
        acc = st % 4 == 2
        h = ind_hat(rng, dur=0.12 if acc else 0.045, decay=28 if acc else 80)
        g = (0.55 if acc else 0.28) * (1.0 + 0.2 * rng.uniform(-1, 1))
        add_wrap(drum_bus, pan2(h, 0.35 if st % 2 else -0.35, g), at(st / 4))

    # ---- metal: anvil clangs, pipe hits, sheet-metal crashes ----
    anvil_steps_a = [2, 6, 10, 14]                # off-beats
    anvil_steps_b = [2, 5, 6, 10, 13, 14]
    for bar in range(bars):
        steps = anvil_steps_a if bar < 4 else anvil_steps_b
        for j, s in enumerate(steps):
            f0 = (1180, 1390, 1180, 1660, 1390, 990)[j % 6] * (1.06 if bar >= 8 else 1.0)
            m = metal_hit(rng, f0, dur=0.35, decay=11, index=5.0)
            add_wrap(metal_bus, pan2(m, (-0.55, 0.55)[j % 2], 0.42), at(bar * 4 + s / 4))
            add_wrap(verb_send, pan2(m, 0.0, 0.25), at(bar * 4 + s / 4))
        # pipe "machine" figure on the last beat of every 2nd bar
        if bar % 2 == 1:
            for k, (s, f) in enumerate(((12, 220.0), (13.5, 196.0), (15, 165.0))):
                p = pipe_hit(f * (1.5 if bar >= 8 else 1.0), 0.55)
                add_wrap(metal_bus, pan2(drive(p, 1.8), (-0.4, 0.4, 0.0)[k], 0.45), at(bar * 4 + s / 4))
                add_wrap(verb_send, pan2(p, 0.0, 0.3), at(bar * 4 + s / 4))
    # big metal crash/impact on bar 1 and bar 9 (bar 1 is the loop downbeat)
    for bar in (0, 8):
        cr = metal_hit(rng, 410, dur=2.4, decay=1.6, index=9.0, bright=0.6)
        nz = fft_filter(rng.uniform(-1, 1, int(2.4 * SR)), lo=3000) * np.exp(-time_vec(int(2.4 * SR)) * 2.2)
        boom_t = time_vec(int(1.4 * SR))
        boom = np.sin(sweep_phase(70 * np.exp(-boom_t * 3) + 30)) * np.exp(-boom_t * 2.5)
        add_wrap(metal_bus, pan2(cr, -0.2, 0.35) + pan2(nz, 0.25, 0.3), at(bar * 4))
        add_wrap(kick_bus, pan2(drive(boom, 2.5), 0.0, 0.7), at(bar * 4))
        add_wrap(verb_send, pan2(cr + nz, 0.0, 0.4), at(bar * 4))

    # ---- bass: distorted detuned saws, 16th pulse in E phrygian ----
    E, F, G, Bb, D = 28, 29, 31, 34, 38
    cell_a = [E, E, E + 12, E, E, E, E + 12, E, E, E, E + 12, E, F, F, F + 12, F]
    cell_b = [E, E, E + 12, E, E, E, E + 12, E, G, G, G + 12, G, Bb, Bb - 1, G, F]
    for bar in range(bars):
        cell = cell_a if bar % 2 == 0 else cell_b
        if bar >= 12:                                  # climb: up a half step
            cell = [c + (1 if bar in (12, 13) else 3) if i < 8 else c for i, c in enumerate(cell)]
        if bar == 15:
            cell = [E + 12, E, E + 12, E, F + 12, F, F + 12, F, G + 12, G, G + 12, G, Bb + 12, Bb, D + 12, D]
        for s, note in enumerate(cell):
            sig, fenv = ind_bass(midi(note), s16 / SR * 0.92)
            st = int(round(bar * 4 * spb + s * s16))
            acc = 1.0 if s % 4 == 0 else 0.8
            seg = np.arange(len(sig))
            idx = (st + seg) % n
            bass_dry[idx] += sig * acc
            bass_env[idx] = np.maximum(bass_env[idx], fenv * acc)
    # time-varying "filter": crossfade dark and bright versions by the envelope,
    # opening up over the 8-bar rise
    section_open = np.clip((np.arange(n) / spb - 32.0) / 32.0, 0, 1)
    bass_dist = drive(bass_dry * 0.9, 5.0, bias=0.1)
    dark = fft_filter(bass_dist, hi=260, order=3, circular=True)
    bright = fft_filter(bass_dist, hi=3800, order=2, circular=True, peak=(900, 1.2, 1.5))
    mixc = np.clip(0.25 + 0.75 * bass_env * (0.5 + 0.5 * section_open), 0, 1)
    bass = dark * (1 - mixc) + bright * mixc * 0.7
    bass = bitcrush(bass, bits=9, hold=2) * 0.6 + bass * 0.4
    sub = fft_filter(bass_dry, hi=90, order=4, circular=True)
    bass = (bass + drive(sub, 1.5) * 0.8) * duck
    bass_st = np.stack([bass, bass], axis=1)
    bass_st[:, 1] = np.roll(bass, 9)   # tiny width; circular, seam-safe

    # ---- stabs: power-chord hits (sparse in drive section, syncopated in rise) ----
    stab_roots = {0: E, 1: E, 2: F, 3: G}
    for bar in range(bars):
        root = stab_roots[(bar // 2) % 4] + 24
        if bar < 8:
            hits = [(0, 1.6)] if bar % 2 == 0 else [(10, 0.5), (14, 0.5)]
        else:
            hits = [(0, 0.55), (3, 0.4), (6, 0.55), (10, 0.4), (12, 0.6)]
        for s, d in hits:
            ch = stab_chord(root, d * beat * 2, rng)
            add_wrap(synth_bus, pan2(ch, -0.45, 0.55) + pan2(np.roll(ch, 600), 0.45, 0.55), at(bar * 4 + s / 4))

    # ---- siren lead over bars 12-15: semitone alarm (E5 -> F5), sliding ----
    siren = [(48, 76, 77, 2.0), (50, 77, 76, 2.0), (52, 76, 79, 2.0), (54, 79, 82, 1.5),
             (56, 76, 77, 2.0), (58, 77, 76, 2.0), (60, 82, 83, 2.0), (62, 83, 88, 2.0)]
    for b, n0, n1, d in siren:
        s = siren_note(n0, n1, d * beat)
        add_wrap(synth_bus, pan2(s, 0.15, 0.32), at(b))
        add_wrap(synth_bus, pan2(s, -0.5, 0.12), at(b + 0.75))   # dotted-8th echo
        add_wrap(verb_send, pan2(s, 0.0, 0.15), at(b))

    # ---- texture: machine grind + hum, and a riser into the loop point ----
    t = time_vec(n)
    grind = fft_filter(rng.uniform(-1, 1, n), lo=900, hi=4500, circular=True, peak=(2100, 3.0, 3.0))
    gate16 = (1.0 - ((t * SR / s16) % 1.0)) ** 3            # 16th-note saw gate
    grind *= gate16 * (0.10 + 0.06 * section_open)
    hum = drive(saw(55.0, n) * 0.5 + saw(55.0 * 1.5 * 1.003, n) * 0.3, 2.0)  # 55 Hz & 82.5 Hz: loop-periodic
    hum = fft_filter(hum, lo=40, hi=600, circular=True) * 0.07
    fx_bus += pan2(grind, -0.3) + pan2(np.roll(grind, 3000), 0.3) + pan2(hum, 0.0)
    # riser: last 4 bars, noise + upward saw sweep, swelling to the downbeat
    rl = 16 * spb
    rt = time_vec(rl)
    prog = rt / rt[-1]
    rnz = fft_filter(rng.uniform(-1, 1, rl), lo=1500, hi=12000)
    rsw = np.sin(sweep_phase(110 * 2 ** (4 * prog)))
    riser = (rnz * 0.6 + drive(rsw, 3.0) * 0.25) * prog ** 3
    riser *= np.clip((rl - np.arange(rl)) / 300.0, 0, 1)
    add_wrap(fx_bus, pan2(riser, 0.0, 0.5), n - rl)

    # ---- factory-hall reverb on the metal/snare send (circular) ----
    verb = circ_convolve(verb_send, hall_ir(rng)) * 0.18

    # ---- mix & master ----
    mix = (kick_bus * 1.0 + drum_bus * 0.9 + metal_bus * 1.0 + bass_st * 0.95
           + synth_bus * (0.7 * (1 - 0.35 * duck[:, None]) + 0.25) + fx_bus * 0.9 + verb)
    mix = fft_filter(mix, lo=30, order=2, circular=True)
    mix = drive(normalize(mix, 1.0) * 1.1, 1.6)             # glue / crunch
    mix -= mix.mean(axis=0)
    return normalize(mix, 0.86), int(bpm)


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
    # The battle has its own rng; advance the shared one exactly as the old
    # battle did so the sting (rendered after it) is unchanged.
    _legacy_battle_rng_advance(rng)
    battle, bpm = make_battle(np.random.default_rng(a.seed + 1000))
    write_wav(os.path.join(a.out, "MUS_Battle.wav"), battle)
    write_wav(os.path.join(a.out, "MUS_VictorySting.wav"), make_sting(rng))
    print(f"done (battle {bpm} BPM, seed {a.seed})")


if __name__ == "__main__":
    main()
