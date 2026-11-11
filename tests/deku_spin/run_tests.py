#!/usr/bin/env python3
"""Compile the production Deku spin action and its skeleton draw boundary.

No engine checkout or assets are needed. The fixture uses narrow engine service
doubles; it does not prove engine ABI compatibility or actual camera/rendering.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CASES = (
    "stationary", "steering", "release-stick", "render-trail", "draw-gates",
    "end-idle", "end-run", "water-void", "anti-reversal",
)


def masked(source):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda match: " " * len(match[0]), source)


def block(source, start):
    code = masked(source)
    opening = code.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (code[end] == "{") - (code[end] == "}")
        end += 1
    return source[start:end]


def function(source, name):
    match = re.search(r"(?m)^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", masked(source))
    if match is None:
        raise ValueError("Missing production function: " + name)
    return block(source, match.start())


def enumeration(source, name):
    start = source.index("typedef enum " + name)
    return block(source, start) + " " + name + ";"


def build_source():
    form = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    header = (ROOT / "soh/mods/transformation_masks/transformation_masks.h").read_text()
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    fixture = (ROOT / "tests/deku_spin/spin_test.cpp").read_text()
    fixture = fixture.replace("/* PRODUCTION_ENUMS */", "\n".join((
        enumeration(form, "MmFormStateId"), enumeration(form, "GoronActionId"),
        enumeration(header, "MmPlayerTransformation"))))
    start = form.index("    // Deku spin attack state")
    fixture = fixture.replace("/* PRODUCTION_SPIN_FIELDS */", form[start:form.index("    // === Deku bubble", start)])
    fixture = fixture.replace("/* PRODUCTION_ACTION */", function(form, "MmForm_Action_DekuSpin"))
    fixture = fixture.replace("/* PRODUCTION_SPIN_GATE */", function(form, "MmForm_IsDekuSpinning"))
    fixture = fixture.replace("/* PRODUCTION_START */", function(form, "MmForm_StartDekuSpinFromOot"))
    movement = function(player, "Player_GetMovementSpeedAndYaw")
    fixture = fixture.replace("/* PRODUCTION_MOVEMENT_INPUT */", re.sub(r"\bthis\b", "self", movement))

    # This is the real neutral-idle tail that otherwise copies visual yaw back
    # into travel yaw. The surrounding idle animation/interaction dispatch is
    # unrelated to the regression and remains outside the fixture boundary.
    idle = function(player, "Player_Action_Idle")
    start = idle.index("            yawDiff = yawTarget - this->actor.shape.rot.y;")
    end = idle.index("            if (Player_GetIdleAnim", start)
    fixture = fixture.replace("/* PRODUCTION_IDLE_YAW_TAIL */", re.sub(r"\bthis\b", "self", idle[start:end]))

    draw = function(form, "MmForm_Draw")
    start = draw.index("            // Draw the MM form skeleton (with OOT or form-specific joints)")
    end = draw.index("\n        }\n    }\n\n    // === Underground flower petals", start)
    fixture = fixture.replace("/* PRODUCTION_SKELETON_DRAW */", draw[start:end])
    post = function(form, "MmForm_PostLimbDraw")
    start = post.index("    if (gFormState.currentForm == MM_PLAYER_FORM_DEKU", post.index("// === 8a. Deku spin"))
    fixture = fixture.replace("/* PRODUCTION_TRAIL_DRAW */", block(post, start))
    return fixture


def mutations(source):
    yield "actor-yaw-spin", "stationary", source.replace(
        "player->actor.shape.rot.y = player->yaw;",
        "player->actor.shape.rot.y = player->yaw + (s16)gFormState.dekuSpinRotAccum;", 1)
    steered = source.replace("static void MmForm_Action_DekuSpin(Player* player, PlayState* play) {",
                             "static void MmForm_Action_DekuSpin(Player* player, PlayState* play) {\n"
                             "    s16 oldTravelYaw = player->yaw;", 1)
    yield "double-steering", "steering", steered.replace(
        "gFormState.dekuSpinRotAccum += (s16)(gFormState.dekuSpinSpeed);",
        "gFormState.dekuSpinRotAccum += (s16)(gFormState.dekuSpinSpeed) + (s16)(player->yaw - oldTravelYaw);", 1)
    yield "missing-visual-spin", "render-trail", source.replace(
        "Matrix_RotateY(BINANG_TO_RAD((s16)gFormState.dekuSpinRotAccum), MTXMODE_APPLY);", "", 1)
    yield "inactive-spin", "draw-gates", source.replace(
        "gFormState.state == MMFORM_STATE_ACTIVE && gFormState.currentForm == MM_PLAYER_FORM_DEKU",
        "gFormState.currentForm == MM_PLAYER_FORM_DEKU", 1)
    yield "matrix-leak", "render-trail", source.replace("                Matrix_Pop();", "", 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--mutation-check", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="deku-spin-") as temporary:
        source = Path(temporary) / "spin.cpp"
        binary = Path(temporary) / "spin-test"
        production = build_source()
        source.write_text(production)
        command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                   "-Wno-unused-function", "-Wno-unused-parameter", str(source), "-o", str(binary)]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-no-pie"]
        subprocess.run(command, check=True)
        failed = []
        for case in args.case or CASES:
            result = subprocess.run([str(binary), case], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            if result.returncode:
                failed.append(case)
        if failed:
            raise SystemExit("FAILED: " + ", ".join(failed))
        print(f"PASS {len(args.case or CASES)} production Deku spin cases" + (" (ASan/UBSan)" if args.sanitize else ""))
        if args.mutation_check:
            for name, case, mutant in mutations(production):
                if mutant == production:
                    raise SystemExit("Mutation did not alter production body: " + name)
                source.write_text(mutant)
                subprocess.run(command, check=True)
                result = subprocess.run([str(binary), case], capture_output=True, text=True,
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                if result.returncode != 1 or "FAIL:" not in result.stderr:
                    raise SystemExit("Mutation escaped its behavioral assertion: " + name)
                print("REJECTED " + name + ": " + result.stderr.strip())


if __name__ == "__main__":
    main()
