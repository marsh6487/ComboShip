#!/usr/bin/env python3
"""Compile production OoT collider queues, resets and cylinder contact rules as native C."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:static )?(?:void|s32|u8|f32) " + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(f"Missing production function {name}")
    opening = source.index("{", match.start())
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def typedef(source, name):
    end = source.index("} " + name + ";") + len("} " + name + ";")
    return source[source.rindex("typedef struct", 0, end):end]


def production():
    collision = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    math = (ROOT / "soh/src/code/sys_math3d.c").read_text()
    lib = (ROOT / "soh/src/code/z_lib.c").read_text()
    math_header = (ROOT / "soh/include/z64math.h").read_text()
    actor_header = (ROOT / "soh/include/z64actor.h").read_text()
    context = typedef((ROOT / "soh/include/z64.h").read_text(), "CollisionCheckContext")
    types = math_header[math_header.index("typedef struct {"):math_header.index("// Defines a point in the spherical")]
    parts = [(ROOT / "tests/actor_queue_growth/fixture_prefix.h").read_text(), types,
             '#include "z64collision_check.h"', typedef(actor_header, "DamageTable"),
             typedef(actor_header, "CollisionCheckInfo"), context,
             (ROOT / "tests/actor_queue_growth/fixture_runtime.h").read_text()]
    parts += [function(lib, "Math_Vec3s_ToVec3f")]
    resets = ["Collider_ResetATBase", "Collider_ResetACBase", "Collider_ResetOCBase",
              "Collider_ResetATInfo_Unk", "Collider_ResetATInfo", "Collider_ResetACInfo", "Collider_ResetOCInfo",
              "Collider_ResetQuadACDist", "Collider_ResetLineOC"]
    for shape in ("JntSphElement", "TrisElement", "JntSph", "Cylinder", "Tris", "Quad"):
        resets += [f"Collider_Reset{shape}{kind}" for kind in ("AT", "AC", "OC")]
    parts += [function(collision, name) for name in resets]
    start = collision.find("/* Actor collider queue ownership */")
    if start < 0:
        start = collision.index("void CollisionCheck_InitContext(")
    end = collision.index(function(collision, "CollisionCheck_DisableSAC")) + len(function(collision, "CollisionCheck_DisableSAC"))
    parts += [collision[start:end]]
    start = collision.index("static ColChkResetFunc sATResetFuncs[]")
    end = collision.index(function(collision, "CollisionCheck_SetOC_SAC")) + len(function(collision, "CollisionCheck_SetOC_SAC"))
    parts += [collision[start:end], function(collision, "CollisionCheck_SetOCLine")]
    parts += [function(math, "Math3D_CylOutsideCylDist"), function(math, "Math3D_CylOutsideCyl")]
    parts += [function(collision, name) for name in (
        "CollisionCheck_SkipTouch", "CollisionCheck_SkipBump", "CollisionCheck_NoSharedFlags",
        "CollisionCheck_SetBounce", "CollisionCheck_SetATvsAC", "CollisionCheck_AC_CylVsCyl",
        "CollisionCheck_GetMassType", "CollisionCheck_SetOCvsOC", "CollisionCheck_OC_CylVsCyl",
        "CollisionCheck_SkipOC", "CollisionCheck_Incompatible", "CollisionCheck_SetJntSphHitFX",
        "CollisionCheck_SetCylHitFX", "CollisionCheck_SetTrisHitFX", "CollisionCheck_SetQuadHitFX")]
    start = collision.index("static ColChkApplyFunc sColChkApplyFuncs[]")
    end = collision.index("};", start) + 2
    parts += [collision[start:end], function(collision, "CollisionCheck_SetHitEffects")]
    # Cylinder scenarios execute real dispatch iteration and contact/math, while
    # unused shape dispatch entries are null in this focused native fixture.
    parts += ["static ColChkVsFunc sACVsFuncs[4][4] = { [1][1] = CollisionCheck_AC_CylVsCyl };",
              "static ColChkVsFunc sOCVsFuncs[4][4] = { [1][1] = CollisionCheck_OC_CylVsCyl };"]
    parts += [function(collision, name) for name in ("CollisionCheck_AC", "CollisionCheck_AT", "CollisionCheck_OC")]
    parts += ["static ColChkLineFunc sOCLineCheckFuncs[4];", function(collision, "CollisionCheck_LineOC")]
    parts += [(ROOT / "tests/actor_queue_growth/queue_test.c").read_text()]
    return "\n\n".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--case", choices=("growth", "sac", "lifecycle", "failure", "guards", "contacts", "wide"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="actor-queue-growth-") as directory:
        path = Path(directory)
        (path / "native_test.c").write_text(production())
        flags = ["-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-unused-variable"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"]
        subprocess.run([os.environ.get("CC", "cc"), *flags, "-I", str(ROOT / "soh/include"),
                        str(path / "native_test.c"), "-lm", "-o", str(path / "native_test")], check=True)
        cases = [args.case] if args.case else ("growth", "sac", "lifecycle", "failure", "guards", "contacts", "wide")
        for case in cases:
            subprocess.run([str(path / "native_test"), case], check=True)
            print(f"PASS: production actor queue {case}", flush=True)


if __name__ == "__main__":
    main()
