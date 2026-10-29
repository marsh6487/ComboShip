#!/usr/bin/env python3
"""Run real Hammergeist effect calls through the native float effect helpers."""
from pathlib import Path
import os
import subprocess
import tempfile

from run_tests import ROOT, flags, function


def main():
    actor = (ROOT / "soh/mods/actors/trutefel/actors/z_en_hammergeist.c").read_text()
    effects = (ROOT / "soh/src/code/z_effect_soft_sprite_old_init.c").read_text()
    lib = (ROOT / "soh/src/code/z_lib.c").read_text()
    enum_end = actor.index("} EnHammergeistFace;") + len("} EnHammergeistFace;")
    enum_begin = actor.rindex("typedef enum", 0, enum_end)
    bodies = [function(lib, "Math_Vec3f_Copy"), actor[enum_begin:enum_end],
              function(actor, "EnHammergeist_ChangeFace")]
    bodies += [function(effects, name) for name in ("EffectSsEnIce_SpawnFlyingVec3f", "EffectSsEnFire_SpawnVec3f")]
    bodies += [function(actor, name) for name in ("EnHammergeist_DefuseLeftHammer", "EnHammergeist_DefuseRightHammer",
                                                "EnHammergeist_Infuse")]
    with tempfile.TemporaryDirectory(prefix="hammergeist-effects-") as directory:
        work = Path(directory)
        (work / "hammergeist_adapters.inc").write_text("\n".join(bodies))
        binary = work / "effects"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags("soh"), "-I" + directory,
                        "-no-pie", str(Path(__file__).with_name("hammergeist_effects_test.cpp")),
                        "-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
