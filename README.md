# Mars Hello World (Unreal Engine 5.8)

A "hello world" for Unreal Engine 5 with a bit more drama than usual.

You play a first-person **Earth astronaut** on a red Mars plain under a butterscotch sky, with a
regenerating shield (you cannot die). Three visitors arrive:

| Who | How they arrive | Voice | How they fight |
|---|---|---|---|
| **The Martian** (green, three eyes, antennae) | Sprints in across the dunes | Deep, slow, warbly | Claw slam, lobbed acid spit, acid volley |
| **X-Bot** (chrome robot, cyan visor) | Flies in on a jetpack and lands hard | Ring-modulated, bit-crushed robot | Eye lasers, laser burst, rocket punch |
| **The Venusian** (gold jellyfish, magenta core) | Hovers in and keeps floating | High, shimmering, echoing | Homing plasma orbs, sonic pulse, tentacle lash |
| **You, the Earth astronaut** (white/orange suit) | Already in the arena (first person) | Natural human voice (Piper speaker 199) | Rifle; regenerating shield; cannot die |

Each visitor arrives looking for Kevin. They say **"Hello world, I'm looking for Kevin."** / **"Have you
seen Kevin?"** (and a few variants) in their own voice, with HUD subtitles. As the others show up they
ask **"Are you Kevin?"** One answers **"I'm Phil."**, another **"I'm Tyler."**, and the third
**"I'm Danyon."** (spelled D-A-N-Y-O-N, pronounced DAN-yun). Someone also asks you; you answer
**"No!"**. They keep looking. Then, at a moment picked from the show seed, one of them decides
another **is** Kevin and starts shooting. The others jump in on the accuser, then each decides
that **all the others** — including you — are Kevin, and they fight you and each other. You can
kill them. Whenever an NPC knocks another out, they yell **"I killed Kevin!"** Losers fall apart
(their body parts become physics objects). When all three visitors are down, you say
**"I hate it when things call me Kevin. I go by Kev."** and the victory HUD offers **Respawn**
(restarts the round) and **Quit**. Calmer ambient music plays during the arrivals and Kevin
search; an upbeat 132 BPM battle loop crossfades in on the first shot; a short sting plays on
victory. A full show runs about **80 to 120 seconds**.
Gravity is Mars gravity (3.71 m/s²), so everything hops and floats a bit.

The project is **C++-first, with no hand-made binary assets**. The terrain, sky, characters, effects
and HUD are all built by code when the game starts. The WAV voice lines in `RawAudio/` and the original procedural soundtrack in `RawAudio/Music/` are
the only media (retired takes stay under `RawAudio/Archive/`: `HelloWorld/`, `AstronautRadio/`,
`DenyOther/`).

## Run it on Windows

1. Install **Unreal Engine 5.8** from the Epic Games Launcher (Library → Engine Versions → +). Also install
   **Visual Studio 2022** (or newer) with the *Game development with C++* workload and a Windows SDK.
2. Install **Git** and **Git LFS**, then clone:

   ```bat
   git lfs install
   git clone https://github.com/Phil801/mars-hello-world-ue5.git C:\Dev\mars-hello-world-ue5
   ```

3. Right-click `C:\Dev\mars-hello-world-ue5\MarsHelloWorld.uproject` and choose **Generate Visual Studio project files**.
4. Double-click `MarsHelloWorld.uproject`. When it asks to rebuild the missing `MarsHelloWorld` module, click **Yes**.
   The first compile and shader build can take a while.
5. On first editor launch, `Content/Python/init_unreal.py` runs `mars_setup.py` automatically. It imports the voices to
   `/Game/Audio` and creates the materials in `/Game/Materials`. You can also run it yourself from **Tools → Execute Python Script...**:

   ```text
   C:\Dev\mars-hello-world-ue5\Content\Python\mars_setup.py
   ```

6. Press **Play** (Alt+P). You spawn as the astronaut in first person.

   Controls: **WASD** move, **mouse** look, **LMB** fire, **Space** jump, **Left Shift** sprint,
   **C** cycle camera (first person, then the old director / wide-orbit / low-action attract views),
   **P** slow motion, **R** restart, **H** hide help, **Esc** quit. On victory, click **Respawn**
   (or press **R**) to restart the round, or click **Quit**.
7. To package a standalone `.exe`:

   ```bat
   C:\Dev\mars-hello-world-ue5\Scripts\BuildWin.bat "C:\Program Files\Epic Games\UE_5.8"
   ```

   The game ends up in `C:\Dev\mars-hello-world-ue5\Build\Windows\Windows\MarsHelloWorld.exe`.

If you skip step 5 the game still runs. The ground is tinted a single rust colour instead of vertex-painted,
and the voices play straight from `RawAudio/` via a procedural sound wave.

Linux: `Scripts/build_linux.sh /path/to/UnrealEngine`. Container build (needs Epic-linked GitHub access): see
[`Scripts/docker/README.md`](Scripts/docker/README.md).

## How it works

```mermaid
flowchart TD
    A[Engine loads /Engine/Maps/Entry] --> B[AMarsGameMode::StartPlay]
    B --> T[AMarsTerrain: procedural dunes, craters, flat arena, 260 rocks]
    B --> E[AMarsEnvironment: Mars SkyAtmosphere, dusty fog, sun, Phobos and Deimos]
    B --> CAM[AMarsShowCamera: director camera]
    B --> S[UMarsShowSubsystem: the referee]
    B --> P[Player astronaut already in the arena, first-person camera]
    B -->|timed entrances| M[Martian runs in]
    B -->|timed entrances| X[X-Bot flies in]
    B -->|timed entrances| V[Venusian hovers in]
    M & X & V -->|NotifyArrived| D[UMarsConversationDirector]
    D -->|cue table| G[Conversation: looking for Kevin, Are you Kevin?, I'm Phil / I'm Tyler / I'm Danyon, astronaut says No!]
    G --> ACC[Accusation: one decides another is Kevin and opens fire]
    ACC --> J[Others fight the accuser, then "you're all Kevin!"]
    J --> F[Free-for-all: NPCs attack the astronaut and each other]
    F -->|after 50 s| SD[Sudden death: damage ramps up]
    F -->|NPC KO| KO[NPC killer: "I killed Kevin!" + loser collapses]
    KO -->|all NPCs down| W[Astronaut: "I hate it when things call me Kevin. I go by Kev." + Respawn / Quit]
    MUS[AMarsMusicActor: ambient, then 132 BPM battle on first shot, victory sting]
    MUS -.BeginShow / first shot / victory / duck.-> S
    H[AMarsHUD: banners, subtitles, health bars, damage numbers, kill feed] -.reads.-> S
```

Code map (`Source/MarsHelloWorld/`):

- `Core/`: `MarsGameMode` builds the world and spawns the cast. `MarsShowSubsystem` is the referee and runs the phases
  Arrivals → Conversation → Accusation → Fight → Victory. `MarsShowSettings` holds the tunables (Project Settings → Game → Mars Show).
  `MarsGameplayTags` has the state, damage and species tags. `MarsPlayerController` is first-person
  input plus attract-camera controls, built at runtime with Enhanced Input.
- `Dialogue/`: `MarsDialogueTable` is the data-driven line set (C++ only, no `.uasset` data table). `MarsConversationDirector`
  is the arrival / conversation / accusation state machine: it assigns "I'm Phil" / "I'm Tyler" / "I'm Danyon" uniquely
  at random from the show seed, picks who accuses whom first, asks the astronaut "Are you Kevin?",
  and queues a kill line on each NPC KO.
- `Combat/CombatantAstronaut`: playable first-person Earth astronaut (primitive arms and rifle,
  regenerating shield, immortal). The show camera is still there as an attract / debug view (C).
- `World/`: `MarsTerrain` is a procedural mesh with fractal noise, ridges, seeded craters and a flattened arena, plus instanced rocks.
  `MarsEnvironment` holds the sky, fog, light and moons. `MarsAssets` builds bodies from `/Engine/BasicShapes` and handles material fallbacks.
- `Combat/`: `CombatantCharacter` is the shared state machine (Entering → Greeting → Ready → Fighting → Dead/Victorious), AI and attacks.
  Greeting means "holding for a conversation line". `CombatantMartian`, `CombatantXBot` and `CombatantVenusian` hold each character's
  body, animation and moves (data in `FCombatantProfile`). `MarsProjectile`, `MarsHitFlash` (hit effects with no Niagara needed)
  and `MarsHealthComponent` support them.
- `Audio/MarsVoiceLibrary`: uses the imported SoundWave if there is one. Otherwise it parses `RawAudio/*.wav` into a `USoundWaveProcedural`.
  Otherwise there are subtitles only. Every spoken line shows a HUD subtitle either way.
- `Audio/MarsMusicActor`: 2D soundtrack (imported `/Game/Audio/Music` or a procedural pump of `RawAudio/Music/`).
  Ambient on `BeginShow`, battle loop crossfaded in on the first shot, victory sting, ducking under voice lines.
  `Respawn` / `R` tears the world down, so the next round starts on ambient again.
- `UI/MarsHUD`: a Canvas HUD, so no UMG assets are needed. Captions shrink to fit long lines. Victory shows **Respawn**
  and **Quit**. `Camera/MarsShowCamera`: director, wide-orbit and low-action camera modes, plus trauma shake.

Changing the show: edit `Config/DefaultGame.ini` (`[/Script/MarsHelloWorld.MarsShowSettings]`). Change `RandomSeed` for a different
landscape, a different Phil / Tyler / Danyon assignment, a different first accusation, and different fight choices.
Music volumes and the crossfade live in the same settings (`MusicMasterVolume`, and so on).
Set `AutoRestartSeconds=8` to loop the show forever.

## How this was built

The whole project was written by **Grok Bot**, an AI agent working for Phil Burns. It started from no game-dev
knowledge, as a test of learning a field from zero and shipping a real product:

1. Researched Unreal Engine 5 and studied the source of 20 open-source UE5 games, such as
   [tomlooman/ActionRoguelike](https://github.com/tomlooman/ActionRoguelike). Those patterns carried over:
   native gameplay tags, a world subsystem as the game director, `UDeveloperSettings`, data-driven profiles, and target and Build.cs layout.
2. Designed a project that runs **with no binary assets in git**. The world, characters and HUD are generated at runtime,
   and a Python editor script makes the few `.uasset`s that need an editor.
3. Generated the voices and the procedural soundtrack offline (see below). Wrote the C++ against the public UE API docs.
4. It was written on a Linux box **without an Unreal Engine install**, so it has not been compiled yet. Everything that
   must be checked on the first compile and run is listed in [`CHECKS.md`](CHECKS.md).

## Voices

`RawAudio/` holds 44.1 kHz, 16-bit mono WAVs, loudness-normalised to −16 LUFS: **14 lines × 3 visitors**,
plus **3 lines** for the astronaut. They were made by `Tools/voice/generate_voices.py` (re-runnable).
Each character keeps a single Piper speaker and the same effects on every line. Keep the spoken texts
in sync with `Source/MarsHelloWorld/Dialogue/MarsDialogueTable.cpp`.

| Character | Base voice | Effects |
|---|---|---|
| Martian | Piper `en_US-libritts_r-medium`, speaker 700, slowed | pitched down about 4 semitones, vibrato, cavern echo |
| X-Bot | same model, speaker 333 | 55 Hz ring modulation, 6-bit crush, sample-and-hold, metallic comb |
| Venusian | same model, speaker 850 | pitched up about 5 semitones, chorus, long airy echoes |
| Astronaut | same model, speaker 199 | natural human (loudness-normalised only; no radio FX) |

File names are `VO_<Martian|XBot|Venusian>_<LineId>.wav`. The full script:

| Line id | Martian | X-Bot | Venusian |
|---|---|---|---|
| ArrivalHello | Hello world, I'm looking for Kevin. | Hello, world. I'm looking for Kevin. | Hello world, I'm looking for Kevin. |
| ArrivalSeenKevin | Have you seen Kevin? | Have you seen Kevin? | Have you seen Kevin? |
| ArrivalVariant | Hello, world! Kevin was supposed to meet me here. | Hello, world. Scanning for Kevin. | Hello, world! I'm looking for someone called Kevin. |
| AskAreYouKevin | Are you Kevin? | Are you Kevin? | Are you Kevin? |
| AskAreYouKevinVariant | Wait. Are you Kevin? | Identify. Are you Kevin? | You there... are you Kevin? |
| DenyPhil | I'm Phil. | I'm Phil. | I'm Phil. |
| DenyTyler | I'm Tyler. | I'm Tyler. | I'm Tyler. |
| DenyDanyon | I'm Danyon. | I'm Danyon. | I'm Danyon. |
| KeepLooking | Still looking for Kevin. | Kevin is not in this scan. | Has anyone seen Kevin? |
| Accuse | Wait... you're Kevin! | Target identified. You are Kevin. | I knew it! You're Kevin! |
| AccuseVariant | That's Kevin! Get him! | Kevin confirmed. Engaging. | Kevin! I found Kevin! |
| AllAreKevin | You're all Kevin! | Recalculating. All units are Kevin. | Wait... you're ALL Kevin! |
| KillKevin | I killed Kevin! | I killed Kevin! | I killed Kevin! |
| KillKevinVariant | Got him! I killed Kevin! | Kevin eliminated. | I killed Kevin! Finally! |

Astronaut lines (natural voice, speaker 199): **DenyKevin** "No!"; **DenyKevinVariant** "No! I'm not Kevin.";
**VictoryHateKevin** "I hate it when things call me Kevin. I go by Kev."

Which arrival line each character uses, who claims to be Phil, Tyler, or Danyon (no duplicates),
and who accuses whom first, are all rolled from `RandomSeed`. Piper is given the phonetic text
"I'm Dan-yun." for **DenyDanyon** so it is pronounced DAN-yun; the subtitle stays "I'm Danyon."
Retired takes stay under `RawAudio/Archive/` (`HelloWorld/`, `AstronautRadio/`, `DenyOther/`).

## Music

Original, copyright-free soundtrack, synthesized with Python and numpy (`Tools/music/generate_music.py`).
No samples, no downloads, nothing copyrighted. Output is 44.1 kHz, 16-bit stereo PCM, loudness-normalised,
written to `RawAudio/Music/`. Re-run:

```bash
python3 Tools/music/generate_music.py --out RawAudio/Music --seed 801
```

| File | Role | Length | Notes |
|---|---|---|---|
| `MUS_AmbientMars.wav` | Arrivals and the Kevin search | 40.00 s loop | Calm D-dorian bed: soft pads, sparse winds, a slow pulse. Overlap-add seam so it repeats without a click. |
| `MUS_Battle.wav` | First shot through the fight | 29.08 s loop (16 bars) | 132 BPM, four-on-the-floor kick, snare, hats, E-minor bass riff, synth lead hook. Integer bars, seamless. |
| `MUS_VictorySting.wav` | Victory (one-shot) | 4.20 s | Short G-major sting; not looped. |

`AMarsMusicActor` starts ambient on `BeginShow`, crossfades to the battle loop on the first real shot
(accusation or the player's first rifle shot in the fight), fades the loops and plays the sting on victory,
and ducks under spoken lines. **Respawn** / **R** restarts the level, so the next round is ambient again.
Volumes and the crossfade length are in `MarsShowSettings`. `mars_setup.py` imports the tracks into
`/Game/Audio/Music` and marks the two loops as looping; before that step the actor pumps the raw WAVs
through `USoundWaveProcedural`.

## Credits and licenses

- **Code**: MIT, © 2026 Phil Burns (see `LICENSE`). **Unreal Engine** is © Epic Games, under the Unreal Engine EULA.
  Engine content used at runtime (`/Engine/BasicShapes`, `/Engine/Maps/Entry`) is covered by that EULA.
- **Piper TTS** (tool used to generate the voices, not shipped): https://github.com/rhasspy/piper. piper-tts 1.8.0 is GPL-3.0.
  Only its *output audio* is in this repo.
- **Voice model** `en_US-libritts_r-medium` (https://huggingface.co/rhasspy/piper-voices). Trained on
  **LibriTTS-R** (https://www.openslr.org/141/, **CC BY 4.0**, from LibriVox public-domain recordings; attribution: Koizumi et al., 2023).
  Its model card says it was *fine-tuned from the Piper "lessac" voice*. The lessac base uses the Blizzard 2013
  Lessac data, which has its own terms. **Check those terms before any commercial release**, or regenerate the voices
  with a voice whose whole lineage is permissive.
- Design patterns are inspired by Tom Looman's ActionRoguelike (MIT). No code was copied.
- No Mixamo, Adobe, Quixel or Marketplace assets are used.
