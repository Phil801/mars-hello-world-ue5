# CHECKS: unverified until the first compile and run

This project was written without an Unreal Engine install. Work through this list on the first build and tick items off.
"Fix" says what to change if the check fails.

## Compile
- [ ] **Builds cleanly on UE 5.8** with `BuildSettingsVersion.Latest` and `EngineIncludeOrderVersion.Latest`. Expect some
      implicit double→float conversion warnings (UE5 uses double-precision `FVector`). Fix: add `static_cast<float>` where flagged.
- [ ] `UKismetProceduralMeshLibrary::CreateGridMeshWelded` signature (MarsTerrain.cpp).
- [ ] `UGameplayStatics::SuggestProjectileVelocity_CustomArc` signature (CombatantCharacter.cpp, lobbed acid).
- [ ] `UInputMappingContext::MapKey` callable at runtime on a transient object (MarsPlayerController.cpp).
- [ ] `USkyAtmosphereComponent`, `UExponentialHeightFogComponent` and `USkyLightComponent` property and setter names (MarsEnvironment.cpp).
- [ ] `FPostProcessSettings` color-grading fields take `FVector4` (MarsShowCamera.cpp).
- [ ] Native gameplay tags convert implicitly to `FGameplayTag` inside class constructors (combatant profiles).

## Run (Play in editor, then a packaged build)
- [ ] **Entry map + GlobalDefaultGameMode**: `/Engine/Maps/Entry` loads and `AMarsGameMode` is used. Fix: run `mars_setup.py`
      and point `GameDefaultMap` to `/Game/Maps/MarsArena`.
- [ ] **Terrain faces up**: if the ground is invisible from above or lit from below, set `bFlipWinding=true` on `AMarsTerrain`.
- [ ] **Gravity**: things fall at about 3.71 m/s² (HUD shows `Gravity 3.71`). This comes from both `DefaultGravityZ` and the WorldSettings override.
- [ ] **Sky looks like Mars**: butterscotch by day, bluish around the sun. Tune `MarsEnvironment.cpp` (Rayleigh/Mie values) and the sun's angle.
- [ ] **Exposure**: not blown out or too dark. Tune `AutoExposureBias` in MarsShowCamera and the sun intensity (4.6).
- [ ] **BasicShapeMaterial has a "Color" vector parameter** (fallback tinting before `mars_setup.py` runs). If not, everything
      is grey until the setup script runs.
- [ ] **Voices**: each table line's imported SoundWave plays in that character's voice. Before setup, the
      `USoundWaveProcedural` fallback plays from `RawAudio/VO_<Species>_<LineId>.wav`. Missing audio must not stall
      the show (subtitle only). The astronaut should sound like a natural human (no radio band-limit). **DenyDanyon**
      should be heard as DAN-yun (Piper text "I'm Dan-yun.") while the subtitle reads "I'm Danyon."
- [ ] **Packaged build voices**: `RawAudio/` is not staged. Packaged builds rely on `/Game/Audio`, which is cooked via
      `DirectoriesToAlwaysCook`, so make sure the build scripts' `mars_setup.py` step imported every new line
      (it scans `RawAudio/*.wav` and skips `RawAudio/Archive/`) and imported `RawAudio/Music/` into `/Game/Audio/Music`.
- [ ] **Cooking engine content**: `/Engine/BasicShapes` is cooked (DirectoriesToAlwaysCook) so bodies appear in packaged builds.
- [ ] **Player spawn**: you start in first person as the Earth astronaut in the arena (white/orange suit arms and a rifle
      visible). WASD, mouse look, Space jump, Left Shift sprint, LMB fire all work. Gravity hops feel right.
- [ ] **Enhanced Input**: runtime `MapKey` with negate/swizzle modifiers for WASD and gamepad (MarsPlayerController.cpp).
      If a modifier property or `EKeys::Mouse2D` name moved in 5.8, rebind there.
- [ ] **Entrances**: the Martian runs in, X-Bot flies in and lands, the Venusian hovers in and stays flying (knockback uses
      `KickVelocity` so it never drops into Falling). The 25 s straggler timer teleports anyone stuck. The astronaut is
      already in the arena and is not launched as a fourth arriver.
- [ ] **Conversation**: each NPC arriver says an arrival line with a subtitle. The first NPC to land asks the
      astronaut "Are you Kevin?" and the astronaut answers **"No!"** (natural voice). As the second and third
      NPCs show up, someone already present asks them "Are you Kevin?" and they answer with their seeded identity
      (I'm Phil / I'm Tyler / I'm Danyon, one each, no duplicates). The first NPC arriver is asked after everyone
      has landed so all three identities are heard. Then one or two "still looking" lines. Lines play one at a time.
- [ ] **Accusation is seeded and not a 3-2-1 countdown**: after the search, one combatant accuses another
      ("you're Kevin!") and opens fire on them. The others then fight the accuser. Then the others say
      "you're all Kevin!" (or their variant) and it becomes a free-for-all. Changing `RandomSeed` changes
      who is Phil/Tyler/Danyon and who accuses whom. Same seed should repeat the same assignment.
- [ ] **Fight targeting**: once the fight starts, NPCs attack the astronaut and each other. They can kill each
      other. The player can kill them with the rifle (only during the fight).
- [ ] **Shield and immortality**: hits flash a shield overlay and drain the shield first; the shield regenerates
      after a short delay. Health never reaches zero (clamp). The astronaut never plays a death / ragdoll.
- [ ] **Kill lines**: when an NPC gets a KO they yell "I killed Kevin!" (or a variant) with a subtitle. Player
      kills are silent besides the kill feed.
- [ ] **Victory**: when all three NPCs are down, the astronaut says **"I hate it when things call me Kevin. I go by Kev."**
      with a subtitle, the YOU WIN banner stays up, and **Respawn** (that label, not "Play Again") plus **Quit**
      sit side by side. **Respawn** or **R** restarts the round; **Quit** or **Esc** exits. Click the buttons
      with the visible cursor.
- [ ] **Music**: ambient Mars loop starts at the beginning of the show (calm, no drums). On the first real shot
      (the accuser opening fire, or the player's first rifle shot once the fight has started) it crossfades to the
      132 BPM battle loop without a click or a gap. On victory the loops fade and a short sting plays. Voice lines
      duck the music; after a line finishes it swells back. **Respawn** / **R** returns to ambient. Before
      `mars_setup.py`, tracks should still play from `RawAudio/Music/` via `USoundWaveProcedural`. After setup,
      `/Game/Audio/Music` assets play and the two loops have `bLooping` set. If `GetAvailableAudioByteCount` or
      `UAudioComponent::SetUISound` / `bIsUISound` moved in 5.8, adjust `MarsMusicActor.cpp`.
- [ ] **Attract / debug camera**: **C** cycles first person → director → wide orbit → low action → first person.
      In attract views the full astronaut body is visible and movement/fire are disabled.
- [ ] **Caption fit**: longer lines still fit at 1080p (the HUD shrinks the subtitle to 90% of the screen width).
- [ ] **Projectile collision**: projectiles hit pawns (overlap) and terrain/rocks (block) and don't hit their owner.
- [ ] **Melee range and knockback** feel right under Mars gravity (fighters may float off; the arena leash at 1500 uu pulls them back).
- [ ] **Death**: detached body parts simulate physics. Parts use `PhysicsActor` collision; check they don't explode through the ground.
- [ ] **Show length**: about 80 to 120 s (conversation is longer than the old one-line hello). Tune
      `ConversationGapSeconds` / `OthersJoinAfterSeconds` / `FreeForAllAfterSeconds`, damage/HP, or `SuddenDeathAfterSeconds`.
- [ ] **HUD**: the Canvas font sizes are readable at 1080p and 4K.
- [ ] **Python setup**: material expression names, `MaterialProperty.MP_*` enums, `LevelEditorSubsystem.new_level`, and the
      `sound_group` property in UE 5.8's Python API.
- [ ] **Docker stub**: the `ghcr.io/epicgames/unreal-engine:dev-5.8` tag exists (needs Epic-linked GitHub access). Not run.
