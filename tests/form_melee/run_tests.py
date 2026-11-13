#!/usr/bin/env python3
"""Run form melee against native collision/math bodies and controlled limb matrices.

The limb matrices are explicit fixtures, not sampled game animation. Collider
and vector layouts, intersection routines, hit flags and the production action /
draw helpers are real. Asset loading, animation advancement and effects are test
boundaries. This provides CPU regression evidence, not an in-game acceptance.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^[A-Za-z_][\w \t*]+?\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError("Missing production function: " + name)
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def declaration(body):
    return body[:body.index("{")].strip() + ";"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("cases", nargs="*")
    args = parser.parse_args()
    root = args.root
    form = (root / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    combat = (root / "soh/mods/transformation_masks/mm_form_combat.c").read_text()
    collision = (root / "soh/src/code/z_collision_check.c").read_text()
    math = (root / "soh/src/code/sys_math3d.c").read_text()
    lib = (root / "soh/src/code/z_lib.c").read_text()
    player_lib = (root / "soh/src/code/z_player_lib.c").read_text()
    player = (root / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    # Matrix primitives and native MM curve references are unchanged controls
    # even when --root selects an older OoT form implementation for RED runs.
    matrix = (ROOT / "soh/src/code/sys_matrix.c").read_text()
    mm_player_lib = (ROOT / "mm/src/code/z_player_lib.c").read_text()

    # Keep native vector/collider structures; only their platform includes are
    # excluded. The test's Player and presentation services are narrow fixtures.
    headers = "\n".join(re.sub(r"^#include[^\n]*", "", (root / path).read_text(), flags=re.M)
                        for path in ("soh/include/z64math.h", "soh/include/z64collision_check.h"))
    headers += "\n" + re.search(r"typedef float MtxF_t.*?\} MtxF;",
                                  (ROOT / "libultraship/include/fast/types.h").read_text(), re.S).group(0)
    collision_names = [
        "Collider_ResetATBase", "Collider_ResetATInfo_Unk", "Collider_ResetATInfo", "Collider_ResetACBase", "Collider_ResetACInfo",
        "Collider_ResetQuadACDist", "Collider_ResetQuadAT", "Collider_ResetCylinderAT", "Collider_ResetCylinderAC",
        "Collider_QuadSetNearestAC", "Collider_SetQuadMidpoints", "Collider_SetQuadVertices",
        "CollisionCheck_SkipTouch", "CollisionCheck_SkipBump", "CollisionCheck_NoSharedFlags",
        "CollisionCheck_SetBounce", "CollisionCheck_SetATvsAC", "CollisionCheck_AC_QuadVsCyl",
        "CollisionCheck_AC_CylVsCyl",
    ]
    bodies = {name: function(collision, name) for name in collision_names}
    bodies["func_80090480"] = function(player_lib, "func_80090480")
    bodies.update((name, function(matrix, name)) for name in
                  ("Matrix_TranslateRotateZYX", "Matrix_Scale", "Matrix_MultVec3f"))
    # Follow actual function dependencies instead of rewriting cylinder/triangle
    # math in the test. Forward declarations preserve the production signatures.
    pending = list(bodies.values())
    while pending:
        body = pending.pop()
        for name in re.findall(r"\b(Math3D_\w+|Math_Vec\w+)\s*\(", body):
            if name in bodies:
                continue
            source = math if name.startswith("Math3D_") else lib
            bodies[name] = function(source, name)
            pending.append(bodies[name])

    new_path = bool(re.search(r"^static void MmForm_UpdateMelee\(", form, re.M))
    names = ["MmForm_SetAction", "MmForm_Action_Punch"]
    names += (["MmForm_MeleeLimb", "MmForm_UpdateMelee", "MmForm_DrawMelee"] if new_path else
              ["MmForm_ZoraMeleeLimb", "MmForm_UpdateZoraMelee", "MmForm_DrawZoraMelee"])
    if re.search(r"^static u8 MmForm_OwnsMeleePose\(", form, re.M):
        names.append("MmForm_OwnsMeleePose")
    if re.search(r"^static void MmForm_OverrideGoronPunchHand\(", form, re.M):
        names += ["GakkiKeyframe_Interp", "MmForm_OverrideGoronPunchHand"]
    form_bodies = [function(combat, name) for name in
                   ("MmForm_SetPunchQuadVertices", "MmForm_EnablePunchQuad", "MmForm_DisablePunchQuad",
                    "MmForm_DisableJumpKickQuads")]
    form_bodies += [function(form, name) for name in names]
    override = function(form, "MmForm_OverrideLimbDraw")
    hand_call = re.search(r"MmForm_OverrideGoronPunchHand\([^;]*;", override)
    form_bodies.append("static void production_hand_override(s32 limbIndex, Vec3f* pos, Vec3s* rot) { " +
                       (hand_call.group(0) if hand_call else "") + " }")
    pose = re.search(r"u8 forceOotCopy = .*?;", function(form, "MmForm_Draw"), re.S).group(0)
    form_bodies.append("static u8 production_force_pose(Player* player) { " + pose + " return forceOotCopy; }")
    # The shield/void/knockback entry branches are outside this fixture. Retain
    # the actual contact-priority guard and body-hit response dispatch verbatim.
    damage = function(player, "func_808382DC")
    contact_start = damage.index("            if ((this->unk_A87 != 0)")
    contact_end = damage.index("} else if (this->invincibilityTimer != 0)", contact_start) + 1
    form_bodies.append("static s32 production_contact_response(Player* this, PlayState* play) {\n" +
                       damage[contact_start:contact_end] + "\nreturn 0;\n}")
    enum = re.search(r"typedef enum GoronActionId \{.*?\} GoronActionId;", form, re.S).group(0)
    properties = re.search(r"typedef struct \{\n    const char\* skelPath;.*?\} MmFormProperties;", form, re.S).group(0)
    keyframe = re.search(r"typedef struct \{\n    s16 frame;\n    Vec3s scale;.*?\} GakkiKeyframe;", form, re.S).group(0)
    props = re.search(r"static const MmFormProperties sFormProps\[.*?\n\};", form, re.S).group(0)
    arrays = [re.search(r"static const f32 sGoronPunchQuadParams\[\]\[6\] = \{.*?\n\};", combat, re.S).group(0)]
    arrays += [re.search(r"static const u8 " + name + r"\[\]\[2\] = \{.*?\n\};", form, re.S).group(0)
               for name in ("sGoronPunchFrames", "sZoraPunchFrames", "sGerudoStationaryFrames")]
    aliases = ("#define FORM_NATIVE_PAIRED 1\n#define Form_Update MmForm_UpdateMelee\n"
               "#define Form_Draw MmForm_DrawMelee\n") if new_path else (
               "#define FORM_NATIVE_PAIRED 0\n#define Form_Update MmForm_UpdateZoraMelee\n"
               "#define Form_Draw MmForm_DrawZoraMelee\n")
    native_curves = "\n".join(re.search(r"struct_80124618 " + name + r"\[\] = \{.*?\n\};",
                                        mm_player_lib, re.S).group(0)
                                for name in ("D_801C0750", "D_801C0784"))
    native_curves += "\n" + function(mm_player_lib, "func_80124618")
    with tempfile.TemporaryDirectory(prefix="form-melee-") as temporary:
        build = Path(temporary)
        (build / "native_types.inc").write_text(headers)
        (build / "native_collision.inc").write_text("\n".join(declaration(body) for body in bodies.values()) +
                                                   "\n" + "\n".join(bodies.values()))
        (build / "native_scale.inc").write_text(native_curves)
        (build / "form_melee.inc").write_text("\n".join([enum, properties, keyframe, props, *arrays, aliases,
                                                       *(declaration(body) for body in form_bodies), *form_bodies]))
        binary = build / "melee"
        flags = ["-std=gnu11", "-I" + temporary, "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                 "-fno-pie", "-no-pie", "-Werror=implicit-function-declaration"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
        subprocess.run([os.environ.get("CC", "cc"), *flags, str(Path(__file__).with_name("melee_test.c")),
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
        failures = 0
        cases = args.cases or ["goron-body", "zora-body", "goron-left", "goron-right", "goron-butt",
                               "zora-left", "zora-right", "zora-kick", "windows", "pose", "lifecycle", "timing",
                               "hand-scales", "hand-reach", "contact-priority"]
        for case in cases:
            result = subprocess.run([str(binary), case], check=False, timeout=10,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            failures += result.returncode != 0
        print(f"{len(cases) - failures}/{len(cases)} form melee cases passed", flush=True)
        return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
