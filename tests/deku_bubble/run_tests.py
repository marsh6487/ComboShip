#!/usr/bin/env python3
"""Run the actual B resolver, native upper actions, and Deku projectile bodies.

The partial checkout has no game headers/build. Only engine services (collision
queries/queue submission, audio, resources, graphics) are fixture boundaries.
Production control flow, charge, magic, flight, impacts and cleanup are compiled
unchanged; queue submission deliberately clears returned hits as SoH does.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(text, start):
    opening = text.index("{", start)
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(text, opening):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if not depth:
                return text[start:token.end()]
    raise ValueError("Unterminated block")


def function(text, name):
    pattern = re.compile(r"^[^\n;{}]*\b" + name + r"\([^;{}]*\)\s*\{", re.M)
    found = pattern.search(text)
    if not found:
        raise ValueError("Missing production function: " + name)
    return block(text, found.start())


def source():
    form = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    hooks = (ROOT / "soh/soh/Enhancements/PlayerHooks.cpp").read_text()
    fixture = (ROOT / "tests/deku_bubble/controller_test.cpp").read_text()
    start = form.index("    // === Deku bubble projectile")
    end = form.index("    // Deku water hop", start)
    fixture = fixture.replace("/* STATE */", form[start:end])
    fixture = fixture.replace("/* RESOLVER */", function(hooks, "ResolveItemOnButton"))
    native_names = ("Player_GetItemOnButton", "Player_ProcessItemButtons", "Player_StartDekuBubble",
                    "Player_DekuBubbleCleanup", "func_8083442C", "func_808351D4", "func_808353D8")
    bodies = [re.sub(r"\bthis\b", "self", function(player, name)) for name in native_names]
    fixture = fixture.replace("/* NATIVE_DECLARATIONS */", "\n".join(
        body[:body.index("{")].rstrip() + ";" for body in bodies))
    fixture = fixture.replace("/* NATIVE_PLAYER */", "\n".join(bodies))
    start = form.index("    // Deku bubble: charge tracking while OOT")
    charge = block(form, form.index("    if (gFormState.currentForm == MM_PLAYER_FORM_DEKU)", start))
    fixture = fixture.replace("/* CHARGE */", charge)
    active = function(form, "MmForm_UpdateActive")
    # Run exactly the production prefix before player ownership can return.
    prefix = active[active.index("{") + 1:active.index("    // ── Gakki (instrument)")]
    fixture = fixture.replace("/* FRAME_PREFIX */", prefix)
    effect = ROOT / "soh/mods/transformation_masks/deku_bubble_effect.inc.h"
    fixture = fixture.replace("/* EFFECT */", effect.read_text() if effect.exists() else "")
    names = ("MmForm_FireBubble", "MmForm_KillBubble", "MmForm_UpdateBubbleProjectile", "MmForm_FireDekuBubble")
    bodies = [function(form, name) for name in names]
    fixture = fixture.replace("/* BUBBLE_DECLARATIONS */", "\n".join(
        body[:body.index("{")].rstrip() + ";" for body in bodies))
    fixture = fixture.replace("/* BUBBLE_FUNCTIONS */", "\n".join(bodies))
    fixture = fixture.replace("/* RESET_AVAILABLE */", "#define HAS_BUBBLE_EFFECT 1" if effect.exists() else "")
    return fixture


def native_syntax(path):
    """Compile the complete effect and projectile against real SoH headers."""
    form = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    start = form.index("    // === Deku bubble projectile")
    end = form.index("    // Deku water hop", start)
    text = '''#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include "mods/items/helpers/camera_helper.h"
#include "mods/sound_translator/mm_sfx_ids.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
struct { int currentForm; f32 dekuCheekScale;\n''' + form[start:end] + '''} gFormState;
#define MMFORM_LOG(...) ((void)0)
static void MmForm_PlaySfx(Player*, u16, u16);
'''
    collider = form.index("static ColliderCylinderInit sBubbleColliderInit =")
    text += block(form, collider) + ";\n" + function(form, "MmForm_InitBubbleCollider") + "\n"
    text += (ROOT / "soh/mods/transformation_masks/deku_bubble_effect.inc.h").read_text() + "\n"
    for name in ("MmForm_FireBubble", "MmForm_KillBubble", "MmForm_UpdateBubbleProjectile", "MmForm_FireDekuBubble"):
        text += function(form, name) + "\n"
    (path / "native.cpp").write_text(text)
    flags = ["-std=c++20", "-fsyntax-only", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-Wno-unused-function"]
    includes = (".", "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh",
                "libultraship/include", "combo")
    for combined in (False, True):
        subprocess.run([os.environ.get("CXX", "c++"), *flags,
                        *["-I" + str(ROOT / part) for part in includes],
                        *(["-DCOMBO_BUILD", "-DSOH_BUILD_DLL"] if combined else []),
                        str(path / "native.cpp")], check=True)
    print("PASS complete native bubble effect/projectile syntax (SoH + Combo headers)")


CASES = ("button-alias", "hold-release", "quick-release", "rapid-fire", "charge-sound", "mute-charge",
         "close-wall", "flight", "frame-owner",
         "actor-hit", "actor-bounce", "actor-unk4", "torch-hit", "world-hit", "expiry", "no-magic",
         "splat-lifetime", "splat-draw", "sliding-splat", "surface-dimensions", "surface-downhill",
         "dynapoly-hit", "missing-assets",
         "paused-draw", "projected-audio", "impact-sound-lifetime", "cancel", "cleanup", "null-cleanup")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=CASES, action="append")
    parser.add_argument("--keep-generated", type=Path)
    parser.add_argument("--native-headers", action="store_true")
    args = parser.parse_args()
    generated = source()
    if args.keep_generated:
        args.keep_generated.write_text(generated)
    failed = []
    with tempfile.TemporaryDirectory(prefix="deku-bubble-") as temporary:
        path = Path(temporary)
        (path / "controller.cpp").write_text(generated)
        binary = path / "controller"
        if args.native_headers:
            native_syntax(path)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra",
                        "-Wno-unused-function", "-Wno-unused-parameter", "-Wno-unused-variable",
                        "-Wno-sign-compare",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                        "-no-pie", str(path / "controller.cpp"), "-o", str(binary)], check=True)
        for case in args.case or CASES:
            result = subprocess.run([str(binary), case], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            if result.returncode:
                failed.append(case)
    if failed:
        print("FAIL: " + ", ".join(failed))
        return 1
    print("PASS: " + str(len(args.case or CASES)) + " production Deku bubble cases (ASan + UBSan)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
