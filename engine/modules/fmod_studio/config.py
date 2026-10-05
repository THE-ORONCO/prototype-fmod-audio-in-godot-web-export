# FMOD Studio as a Godot engine module, linked statically from the FMOD HTML5 SDK (web templates only).
import os


def can_build(env, platform):
    return platform == "web"


def get_opts(platform):
    from SCons.Variables import PathVariable

    return [
        PathVariable(
            "fmod_sdk_path",
            "Extracted FMOD Studio API HTML5 SDK (the folder that contains api/)",
            "",
            PathVariable.PathAccept,
        ),
    ]


def configure(env):
    # Runs before any SConscript. platform/web/SCsub freezes link settings before modules/SCsub runs, so they go here.
    sdk = env["fmod_sdk_path"]
    if not os.path.isdir(os.path.join(sdk, "api")):
        raise RuntimeError("fmod_studio: set fmod_sdk_path to the extracted FMOD HTML5 SDK (got '%s')" % sdk)
    # Studio libs already contain the core. "L" = logging build, used for debug templates.
    lib = "fmodstudio_wasm.a" if env["target"] == "template_release" else "fmodstudioL_wasm.a"
    env.Append(LIBS=[env.File(os.path.join(sdk, "api", "studio", "lib", "w32", lib))])
    # Flags the FMOD HTML5 docs require for linking the W32 libraries (ALLOW_MEMORY_GROWTH is already set by Godot).
    env["EXPORTED_RUNTIME_METHODS"] += ["ccall", "cwrap", "setValue", "getValue"]
