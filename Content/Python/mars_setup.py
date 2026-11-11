"""
Mars Hello World - one-shot editor setup (UE 5.8 Python, PythonScriptPlugin + EditorScriptingUtilities).

Creates the few binary assets a C++-only repo cannot ship as text:
  * /Game/Audio/VO_<Species>_<LineId>      - SoundWaves imported from RawAudio/*.wav
  * /Game/Audio/Music/MUS_*                - soundtrack imported from RawAudio/Music/*.wav
  * /Game/Materials/M_MarsGround          - vertex-colour ground (the procedural terrain paints rust colours)
  * /Game/Materials/M_MarsSurface         - "Color" param base colour, "Glow" param emissive
  * /Game/Materials/M_MarsEmissive        - same but glowing by default (eyes, flames, projectiles)
  * /Game/Maps/MarsArena                  - optional empty level; the game mode builds Mars at runtime

The game runs WITHOUT this step (it falls back to engine materials and plays the raw WAVs procedurally),
but running it gives correct ground colours and lets the cooker package the voices.

Run it:
  * Editor: Tools > Execute Python Script... > Content/Python/mars_setup.py   (or Output Log "Cmd" -> py mars_setup.py)
  * Headless: UnrealEditor-Cmd.exe <path>/MarsHelloWorld.uproject -run=pythonscript -script="mars_setup.py"
It is idempotent: existing assets are re-imported / rebuilt in place, never deleted.
"""

import os
import unreal

AUDIO_DIR = "/Game/Audio"
MUSIC_DIR = "/Game/Audio/Music"
MAT_DIR = "/Game/Materials"
MAP_PATH = "/Game/Maps/MarsArena"
LOOPING_MUSIC = {"MUS_AmbientMars", "MUS_Battle"}

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def log(msg):
    unreal.log("[MarsSetup] " + msg)


def list_voice_wavs(raw_dir):
    """Top-level RawAudio/*.wav only. Archived hello-world lines stay in RawAudio/Archive/."""
    if not os.path.isdir(raw_dir):
        return []
    names = []
    for file_name in sorted(os.listdir(raw_dir)):
        if file_name.lower().endswith(".wav"):
            names.append(file_name)
    return names


def import_voices():
    raw_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "RawAudio")
    wavs = list_voice_wavs(raw_dir)
    if not wavs:
        unreal.log_warning("[MarsSetup] no WAV files in " + raw_dir)
        return
    tasks = []
    asset_names = []
    for file_name in wavs:
        path = os.path.join(raw_dir, file_name)
        asset_name = os.path.splitext(file_name)[0]
        asset_names.append(asset_name)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", AUDIO_DIR)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    if tasks:
        asset_tools.import_asset_tasks(tasks)
    for asset_name in asset_names:
        wave = eal.load_asset(AUDIO_DIR + "/" + asset_name)
        if wave:
            # Voice sound group = no reverb send surprises; keep it uncompressed-ish for clarity.
            try:
                wave.set_editor_property("sound_group", unreal.SoundGroup.SOUNDGROUP_VOICE)
            except Exception:  # property names can move between engine versions
                pass
            eal.save_loaded_asset(wave)
            log("voice ok: " + asset_name)


def import_music():
    """Import RawAudio/Music/*.wav into /Game/Audio/Music. Archive folders are skipped."""
    raw_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "RawAudio", "Music")
    if not os.path.isdir(raw_dir):
        unreal.log_warning("[MarsSetup] no music folder at " + raw_dir)
        return
    wavs = [n for n in sorted(os.listdir(raw_dir)) if n.lower().endswith(".wav")]
    if not wavs:
        unreal.log_warning("[MarsSetup] no WAV files in " + raw_dir)
        return
    if not eal.does_directory_exist(MUSIC_DIR):
        eal.make_directory(MUSIC_DIR)
    tasks = []
    asset_names = []
    for file_name in wavs:
        path = os.path.join(raw_dir, file_name)
        asset_name = os.path.splitext(file_name)[0]
        asset_names.append(asset_name)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", MUSIC_DIR)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    if tasks:
        asset_tools.import_asset_tasks(tasks)
    for asset_name in asset_names:
        wave = eal.load_asset(MUSIC_DIR + "/" + asset_name)
        if not wave:
            continue
        try:
            wave.set_editor_property("sound_group", unreal.SoundGroup.SOUNDGROUP_MUSIC)
        except Exception:
            pass
        if asset_name in LOOPING_MUSIC:
            try:
                wave.set_editor_property("looping", True)
            except Exception:
                pass
        eal.save_loaded_asset(wave)
        log("music ok: " + asset_name)


def get_or_create_material(name):
    full = MAT_DIR + "/" + name
    if eal.does_asset_exist(full):
        mat = eal.load_asset(full)
        mel.delete_all_material_expressions(mat)  # rebuild graph in place (expressions only, never the asset)
        return mat
    return asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def scalar(mat, value, x, y):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", value)
    return e


def color_glow_material(name, default_color, default_glow, roughness, metallic):
    mat = get_or_create_material(name)
    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", default_color)
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 250)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", default_glow)
    mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 200)
    mel.connect_material_expressions(color, "", mult, "A")
    mel.connect_material_expressions(glow, "", mult, "B")

    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(scalar(mat, roughness, -350, 400), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(mat, metallic, -350, 500), "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    log("material ok: " + name)


def ground_material():
    mat = get_or_create_material("M_MarsGround")
    vcol = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -700, 0)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 200)
    tint.set_editor_property("parameter_name", "Color")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 100)
    mel.connect_material_expressions(vcol, "", mult, "A")
    mel.connect_material_expressions(tint, "", mult, "B")
    mel.connect_material_property(mult, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(mat, 0.92, -350, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(mat, 0.25, -350, 400), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    log("material ok: M_MarsGround")


def create_map():
    if eal.does_asset_exist(MAP_PATH):
        log("map exists: " + MAP_PATH)
        return
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if les.new_level(MAP_PATH):
        les.save_current_level()
        log("map created: " + MAP_PATH + " (empty on purpose; AMarsGameMode builds Mars at runtime)")


def main():
    for d in (AUDIO_DIR, MUSIC_DIR, MAT_DIR, "/Game/Maps"):
        if not eal.does_directory_exist(d):
            eal.make_directory(d)
    import_voices()
    import_music()
    ground_material()
    color_glow_material("M_MarsSurface", unreal.LinearColor(0.5, 0.5, 0.5, 1.0), 0.0, 0.6, 0.0)
    color_glow_material("M_MarsEmissive", unreal.LinearColor(1.0, 0.6, 0.2, 1.0), 6.0, 0.4, 0.0)
    create_map()
    log("done. Press Play (Alt+P).")


if __name__ == "__main__":
    main()
