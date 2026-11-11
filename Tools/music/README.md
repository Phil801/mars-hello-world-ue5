# Procedural Mars soundtrack

`generate_music.py` synthesizes the three original, copyright-free tracks used by `AMarsMusicActor`.
It uses only numpy (no samples, no downloads, no copyrighted material).

```bash
# from the repo root; needs numpy (the project voice venv is fine)
python3 Tools/music/generate_music.py --out RawAudio/Music --seed 801
```

| Output | Role |
|---|---|
| `RawAudio/Music/MUS_AmbientMars.wav` | 40 s calm D-dorian loop (arrivals / Kevin search) |
| `RawAudio/Music/MUS_Battle.wav` | 16-bar 132 BPM loop (kick, snare, hats, bass, lead); starts on the first shot |
| `RawAudio/Music/MUS_VictorySting.wav` | 4.2 s G-major sting (not looped) |

Same seed (default 801) reproduces the same files. See the Music section of the project README for how they play in-game.
