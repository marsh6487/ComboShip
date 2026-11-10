#!/usr/bin/env python3
"""Run production renderer and native limb traversal in both receiving hosts."""
from pathlib import Path
import sys
from catalog_fixture import write_catalog_fixture

def write_native_lerp(build):
    native = (ROOT / "mm/src/code/z_scene_proc.c").read_text()
    bodies = []
    for signature in ["s32 AnimatedMat_Lerp(", "void AnimatedMat_DrawColorLerp("]:
        start = native.index(signature)
        brace = native.index("{", start)
        depth, end = 1, brace + 1
        while depth:
            depth += (native[end] == "{") - (native[end] == "}")
            end += 1
        bodies.append(native[start:end])
    (build / "renderer_native_lerp.inc").write_text("\n".join(bodies))

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
source = (ROOT / 'scripts/diagnostics/run_foreign_soul_skeleton_tests.py').read_text()
source = source.replace("ROOT = Path(__file__).resolve().parents[2]", "ROOT = Path(" + repr(str(ROOT)) + ")")
source = source.replace("(build / 'foreign_anim.h').write_text(header)", "(build / 'foreign_anim.h').write_text(header)\n    write_catalog_fixture(build)\n    write_native_lerp(build)\n    (build / 'legacy_skeleton_fixture.cpp').write_text((ROOT / 'tests/foreign_soul/skeleton_test.cpp').read_text().rstrip()[:-1] + 'return 0; }\\n')")
source = source.replace("str(ROOT/'tests/foreign_soul/skeleton_test.cpp')", "str(ROOT/'tests/soul_mini_models/renderer_test.cpp')")
exec(compile(source, str(ROOT / 'scripts/diagnostics/run_foreign_soul_skeleton_tests.py'), 'exec'))
