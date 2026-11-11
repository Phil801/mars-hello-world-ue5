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
- [ ] **Voices**: the imported SoundWaves play. Before setup, the `USoundWaveProcedural` fallback plays from `RawAudio/`.
- [ ] **Packaged build voices**: `RawAudio/` is not staged. Packaged builds rely on `/Game/Audio`, which is cooked via
      `DirectoriesToAlwaysCook`, so make sure the build scripts' `mars_setup.py` step succeeded.
- [ ] **Cooking engine content**: `/Engine/BasicShapes` is cooked (DirectoriesToAlwaysCook) so bodies appear in packaged builds.
- [ ] **Entrances**: the Martian runs in, X-Bot flies in and lands, the Venusian hovers in and stays flying (knockback uses
      `KickVelocity` so it never drops into Falling). The 25 s straggler timer teleports anyone stuck.
- [ ] **Greetings** happen one at a time with subtitles. The camera frames each speaker's face (check the forward axis of each body).
- [ ] **Projectile collision**: projectiles hit pawns (overlap) and terrain/rocks (block) and don't hit their owner.
- [ ] **Melee range and knockback** feel right under Mars gravity (fighters may float off; the arena leash at 1500 uu pulls them back).
- [ ] **Death**: detached body parts simulate physics. Parts use `PhysicsActor` collision; check they don't explode through the ground.
- [ ] **Show length**: about 60 to 90 s. Tune damage/HP in the combatant constructors, or `SuddenDeathAfterSeconds`.
- [ ] **HUD**: the Canvas font sizes are readable at 1080p and 4K.
- [ ] **Python setup**: material expression names, `MaterialProperty.MP_*` enums, `LevelEditorSubsystem.new_level`, and the
      `sound_group` property in UE 5.8's Python API.
- [ ] **Docker stub**: the `ghcr.io/epicgames/unreal-engine:dev-5.8` tag exists (needs Epic-linked GitHub access). Not run.
