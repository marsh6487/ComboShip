#!/usr/bin/env python3
"""Exercise the production OoT form-instrument controller and animation queue.

Player, PlayState, animation buffers, queue entries, and display commands use
the real engine headers. The only services replaced are resource loading,
audio output, event dispatch, and matrix allocation. Animation changes, frame
loads, masked copies, interpolation, and the instrument controller are the
production function bodies, compiled in standalone and Combo configurations.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mask_transformation_tests import function


def block(source, start):
    begin = source.index("{", start)
    end = begin + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def build_source():
    form = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    animation = (ROOT / "soh/src/code/z_skelanime.c").read_text()
    player_lib = (ROOT / "soh/src/code/z_player_lib.c").read_text()
    fixture = (ROOT / "tests/form_instruments/controller_test.cpp").read_text()
    start = form.index("    // Gakki (instrument) state")
    end = form.index("    // Deku cheek inflation", start)
    fixture = fixture.replace("/* PRODUCTION_GAKKI_FIELDS */", form[start:end])
    hands = "\n".join(
        block(player_lib, player_lib.index("Gfx* " + name + "[]")) + ";"
        for name in ("sPlayerRightHandOcarinaDLs", "sPlayerRightHandOotDLs"))
    # Native C stores OTR path pointers in these Gfx* tables.
    hands = re.sub(r"^(\s+)(g\w+),", r"\1(Gfx*)\2,", hands, flags=re.M)
    fixture = fixture.replace("/* PRODUCTION_OCARINA_HAND_TABLES */", hands)
    start = form.index("typedef struct {", form.index("// Gakki (Instrument) Keyframe System"))
    end = form.index("// Slot-Based Restriction Helpers", start)
    fixture = fixture.replace("/* PRODUCTION_KEYFRAMES */", form[start:end])

    names = re.findall(r"^static\s+[^\n;{]*\b(MmForm_(?:Gakki\w+|EnterGakki|UpdateGakki|ExitGakki|RegisterOcarinaNoteHook))\s*\(", form, re.M)
    # Forward declarations are also matched; retain each actual body only once.
    names = list(dict.fromkeys(names))
    bodies = [function(form, name) for name in names]
    declarations = [body[:body.index("{")].rstrip() + ";" for body in bodies]
    fixture = fixture.replace("/* PRODUCTION_GAKKI_DECLARATIONS */", "\n".join(declarations))
    fixture = fixture.replace("/* PRODUCTION_GAKKI_FUNCTIONS */", "\n".join(bodies))
    active = function(form, "MmForm_UpdateActive")
    start = active.index("    // ── Gakki (instrument)")
    end = active.index("    // ── Unified per-form owner dispatch", start)
    fixture = fixture.replace("/* PRODUCTION_GAKKI_SESSION */", active[start:end])
    for label, suffix, ending in (
        ("GORON", "Goron Drums", "        } else if"),
        ("ZORA", "Zora Guitar", "            // Initialize shield"),
        ("DEKU", "Deku Pipes", "        } else if"),
    ):
        start = form.index("            // Gakki (instrument) animations — " + suffix)
        end = form.index(ending, start)
        fixture = fixture.replace("/* PRODUCTION_LOAD_" + label + " */", form[start:end])

    override = function(form, "MmForm_OverrideLimbDraw")
    start = override.index("    if (gFormState.currentForm == MM_PLAYER_FORM_FIERCE_DEITY)")
    fixture = fixture.replace("/* PRODUCTION_FD_HAND_OVERRIDE */", block(override, start))
    # This callback boundary is shared with the full form skeleton traversal.
    zora_calls = re.findall(r"^\s*MmForm_GakkiOverrideZoraHand\([^;]+;", override, re.M)
    fixture = fixture.replace("/* PRODUCTION_ZORA_HAND_OVERRIDE */", "\n".join(zora_calls))
    draw = function(form, "MmForm_PostLimbDraw")
    start = draw.index("    if (gFormState.gakkiActive)", draw.index("// === 10. Gakki"))
    fixture = fixture.replace("/* PRODUCTION_INSTRUMENT_DRAW */", block(draw, start))
    for label, name in (("RESTORE", "MmForm_RestoreOotState"), ("DAMAGE", "MmForm_CheckDamage")):
        body = function(form, name)
        start = body.index("    // Clean" + ("up" if label == "RESTORE" else " up") + " gakki")
        fixture = fixture.replace("/* PRODUCTION_" + label + "_CLEANUP */", body[start:body.index("\n\n", start)])
    for label, name in (("INIT", "MmForm_Init"), ("RESET", "MmForm_Reset"), ("DEATH", "MmForm_OnDeath")):
        body = function(form, name).split("{", 1)[1].split("    MmForm_StopGoronRollSfx();", 1)[0]
        fixture = fixture.replace("/* PRODUCTION_" + label + "_CLEANUP */", body)
    reload = function(form, "MmForm_SoftReload")
    cleanup = reload.split("{", 1)[1].split("    // 2. Reset stale fields", 1)[0]
    start = reload.index("    gFormState.gakkiActive = 0;")
    end = reload.index("    gFormState.formDLsPinned = 0;", start)
    fixture = fixture.replace("/* PRODUCTION_RELOAD_CLEANUP */", cleanup + reload[start:end])
    native_transform = (ROOT / "soh/mods/transformation_masks/mm_native_transform.inc.h").read_text()
    fixture = fixture.replace("/* PRODUCTION_DESTROY_CLEANUP */", function(native_transform, "MmForm_OnPlayDestroy"))

    native_names = (
        "Animation_GetLength", "Animation_GetLastFrame", "SkelAnime_InterpFrameTable",
        "AnimationContext_AddEntry", "AnimationContext_SetLoadFrame", "AnimationContext_SetCopyAll",
        "AnimationContext_SetInterp", "AnimationContext_SetCopyTrue", "AnimationContext_SetCopyFalse",
        "AnimationContext_LoadFrame", "AnimationContext_CopyAll", "AnimationContext_Interp",
        "AnimationContext_CopyTrue", "AnimationContext_CopyFalse", "AnimationContext_Update",
        "LinkAnimation_SetUpdateFunction", "LinkAnimation_Update", "LinkAnimation_Morph",
        "LinkAnimation_AnimateFrame", "LinkAnimation_Loop", "LinkAnimation_Once",
        "LinkAnimation_Change", "SkelAnime_CopyFrameTable",
    )
    native = "\n".join(function(animation, name) for name in native_names)
    # C permits this comparison between two header pointer types. Make only
    # the pointer conversion explicit when hosting the unchanged logic in C++.
    native = native.replace("animation != currentAnimation", "(void*)animation != (void*)currentAnimation")
    native += "\n" + function((ROOT / "soh/src/code/z_lib.c").read_text(), "Math_StepToF")
    fixture = fixture.replace("/* PRODUCTION_ANIMATION_QUEUE */", native)
    return fixture


CASES = (
    "opening-note", "goron-wait", "goron-five-buttons", "goron-alternating-arms",
    "goron-drum-scales", "deku-selected-pipe", "deku-release", "pitch-modifier-button",
    "missing-wait", "missing-strike", "exit", "custom-form", "ordinary-voice", "native-voice-cleanup",
    "replay-repeat", "held-bend", "late-note-off", "muted-replay", "mute-held",
    "zora-hand", "zora-intro", "fd-ocarina",
    "cleanup-damage", "cleanup-restore", "cleanup-reload", "cleanup-init", "cleanup-reset", "cleanup-death", "cleanup-destroy",
    "performance-loop", "performance-result-dialogue", "performance-callbacks", "performance-scales",
    "performance-decision", "performance-close", "performance-unrelated-text", "performance-cleanup", "opening-after-song",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--keep-generated", type=Path)
    args = parser.parse_args()
    source_text = build_source()
    if args.keep_generated:
        args.keep_generated.write_text(source_text)
    include = (".", "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh", "libultraship/include", "combo")
    failed = []
    with tempfile.TemporaryDirectory(prefix="form-instruments-controller-") as temporary:
        source = Path(temporary) / "controller.cpp"
        source.write_text(source_text)
        for combined in (False, True):
            mode = "combo" if combined else "standalone"
            binary = Path(temporary) / mode
            command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-w", "-fpermissive",
                       "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
                       "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-no-pie",
                       "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                       *(["-DCOMBO_BUILD", "-DSOH_BUILD_DLL"] if combined else []),
                       *["-I" + str(ROOT / path) for path in include], str(source), "-o", str(binary)]
            subprocess.run(command, check=True)
            for case in args.case or CASES:
                result = subprocess.run([str(binary), case], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                if result.returncode:
                    failed.append(mode + ":" + case)
            print("Completed " + mode + " controller cases", flush=True)
    if failed:
        print("FAILED: " + ", ".join(failed), file=sys.stderr)
        return 1
    print("PASS production form-instrument controller and native animation queue (standalone + Combo)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
