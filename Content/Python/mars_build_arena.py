"""
Mars Hello World - bake the runtime-generated Mars into /Game/Maps/MarsArena.

Creates (or rebuilds) real level actors via the C++ helper UMarsArenaBuilder:
  * MarsLandscape      - Landscape whose heightmap is sampled from AMarsTerrain's height function (seed from
                         Project Settings > Mars Show), with MI_MarsGround
  * MarsRockField      - the boulders and slabs as instanced meshes
  * Mars/Environment    - standalone Mars_SkyAtmosphere, Mars_HeightFog, Mars_Sun (DirectionalLight), Mars_SkyLight,
                         Mars_Phobos, Mars_Deimos (MI_Phobos / MI_Deimos), with AMarsEnvironment's exact settings
  * MarsShowCamera, MarsMusic, MarsPlayerStart
  * SpawnMarker_<i>_*  - one AMarsSpawnMarker per CombatantClasses entry (arrival point + EntryStart widget)
Only actors tagged "MarsArenaGenerated" are replaced, so hand-placed actors survive a rebuild.
Re-running after you hand-edit the landscape or markers will overwrite those edits.

Pass --environment-only to rebuild just the Mars/Environment actors (replaces any AMarsEnvironment) and leave the
landscape, rocks, camera and markers alone:  py mars_build_arena.py --environment-only

Run it:
  * Editor: Tools > Execute Python Script... > Content/Python/mars_build_arena.py
  * Or launch: UnrealEditor.exe MarsHelloWorld.uproject -ExecutePythonScript="<project>/Content/Python/mars_build_arena.py"
"""

import sys

import unreal

MAP_PATH = "/Game/Maps/MarsArena"
SAVE_AFTER_TICKS = 90  # let the landscape edit layers merge (GPU) before saving

_handle = None
_ticks = 0


def log(msg):
    unreal.log("[MarsArena] " + msg)


def _save_tick(_delta):
    global _handle, _ticks
    _ticks += 1
    if _ticks < SAVE_AFTER_TICKS:
        return
    unreal.unregister_slate_post_tick_callback(_handle)
    _handle = None
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    ok = level_subsystem.save_current_level()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("saved %s: %s" % (MAP_PATH, ok))


def main():
    global _handle, _ticks
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor_subsystem.get_editor_world()
    if world is None or world.get_path_name().split(".")[0] != MAP_PATH:
        if not level_subsystem.load_level(MAP_PATH):
            unreal.log_error("[MarsArena] could not load " + MAP_PATH)
            return
        world = editor_subsystem.get_editor_world()
    if "--environment-only" in sys.argv[1:]:
        result = unreal.MarsArenaBuilder.build_mars_environment(world)
    else:
        result = unreal.MarsArenaBuilder.build_mars_arena(world, True)
    if result.startswith("ERROR"):
        unreal.log_error("[MarsArena] " + result)
        return
    log(result)
    _ticks = 0
    _handle = unreal.register_slate_post_tick_callback(_save_tick)


main()