#!/usr/bin/env python3
"""Run the production OoT stepping event and MM form routing with audio sinks.

Only audio output, form identity, weather and types are fixtures. The actual
floor selection, cadence, step statistics, jump/land and routing bodies run as
C in standalone and Combo configurations. No archive/synth or audible claim.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CASES = ("curve", "surfaces", "cadence", "fallback", "controls", "legacy", "lifetime", "jump-land", "winter")


def function(source, signature):
    start = source.index(signature)
    begin = source.index("{", start)
    end, depth = begin + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def read(path, source_ref):
    if source_ref:
        return subprocess.check_output(["git", "show", source_ref + ":" + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text(encoding="utf-8-sig")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=CASES, action="append")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--source-ref", help="Run the current regression against committed production bodies")
    args = parser.parse_args()
    masks = read("soh/mods/transformation_masks/transformation_masks.c", args.source_ref)
    header = read("soh/mods/transformation_masks/transformation_masks.h", args.source_ref)
    player = read("soh/src/overlays/actors/ovl_player_actor/z_player.c", args.source_ref)
    bodies = [function(masks, "u8 TransformMasks_TryPlayMmStepSfx(")]
    speed_signature = "u8 TransformMasks_TryPlayMmStepSfxWithSpeed("
    if speed_signature in masks:
        bodies.append(function(masks, speed_signature))
    bodies.extend(function(player, signature) for signature in (
        "static u16 Player_GetStepSfxOffset(", "void Player_PlaySteppingSfx(",
        "u16 Player_ApplyFloorAndAgeSfxOffsets(", "void Player_PlayJumpingSfx(",
        "void Player_PlayLandingSfx(", "s32 func_8084021C(", "void func_8084029C("))
    fixture = (ROOT / "tests/deku_footsteps/step_test.c").read_text()
    enum_start = header.index("typedef enum MmPlayerTransformation {")
    enum_end = header.index("} MmPlayerTransformation;", enum_start) + len("} MmPlayerTransformation;")
    fixture = fixture.replace("/* PRODUCTION_FORM_ENUM */", header[enum_start:enum_end])
    declarations = re.findall(r"^u8 TransformMasks_TryPlayMmStepSfx(?:WithSpeed)?\([^;]+;", header, re.M)
    fixture = fixture.replace("/* PRODUCTION_STEP_DECLARATIONS */", "\n".join(declarations))
    fixture = fixture.replace("/* PRODUCTION_BODIES */", "\n\n".join(bodies))
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-O1", "-g"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                  "-fno-pie", "-no-pie"]
    failed = []
    with tempfile.TemporaryDirectory(prefix="deku-footsteps-") as temporary:
        source = Path(temporary) / "step_test.c"
        source.write_text(fixture)
        for combined in (False, True):
            mode = "combo" if combined else "standalone"
            binary = Path(temporary) / mode
            subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                            *(["-DCOMBO_BUILD"] if combined else []), str(source), "-lm", "-o", str(binary)], check=True)
            for case in args.case or CASES:
                if subprocess.run([str(binary), case]).returncode:
                    failed.append(mode + ":" + case)
    if failed:
        raise SystemExit("FAILED: " + ", ".join(failed))
    print("PASS production Deku step pitch, surfaces, cadence and controls (standalone + Combo)")


if __name__ == "__main__":
    main()
