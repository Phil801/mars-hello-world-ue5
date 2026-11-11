"""Runs automatically when the editor starts (PythonScriptPlugin convention).

If the generated assets are missing, run mars_setup.py once the editor has finished loading.
Set the environment variable MARS_SKIP_SETUP=1 to disable.
"""

import os
import unreal

_handle = None


def _tick(_delta):
    global _handle
    unreal.unregister_slate_post_tick_callback(_handle)
    if unreal.EditorAssetLibrary.does_asset_exist("/Game/Materials/M_MarsGround"):
        return
    unreal.log("[MarsSetup] First run: creating voices + materials (Content/Python/mars_setup.py)")
    import mars_setup
    mars_setup.main()


if os.environ.get("MARS_SKIP_SETUP") != "1":
    # Slate never ticks in -run=pythonscript commandlets, so this is a no-op there (the build script runs mars_setup.py directly).
    _handle = unreal.register_slate_post_tick_callback(_tick)
