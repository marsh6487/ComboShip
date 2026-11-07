"""Compile production cane transforms, native swept quads and vanilla endpoints.

Renderer matrix operations and collision registration are fixture boundaries;
the endpoint generation and sweep/state rules come verbatim from production.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags


def function(source, name):
    match = re.search(r"(?:static )?(?:void|u8|f32) " + name + r"\([^)]*\) \{", source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


with tempfile.TemporaryDirectory(prefix="byrna-collision-") as td:
    directory = Path(td)
    ext = (ROOT / "soh/mods/extended_equipment.c").read_text()
    native = (ROOT / "soh/src/code/z_player_lib.c").read_text()
    pieces = [line for line in ext.splitlines() if line.startswith("#define BYRNA_")]
    pieces += [function(ext, name) for name in ("ExtEquip_ApplySwordDLMatrix", "ExtEquip_ByrnaTrailBegin", "ExtEquip_ByrnaTrailLength")]
    start = native.index("Vec3f D_80126080 =")
    end = native.index("// ── Net:", start)
    pieces += [native[start:end], function(native, "func_80090480"), function(native, "func_80090A28")]
    if "static void Player_ByrnaMeleeWeaponPostLimb(" in native:
        pieces += [function(native, "Player_ByrnaMeleeWeaponPostLimb")]
    else:
        pieces += [function(native, "func_800906D4")]
    pieces += [function((ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text(), "func_80832318")]
    branch = native[native.index("} else if (ExtEquip_ByrnaTrailBegin())") + 2:]
    branch = branch[:branch.index("} else if (this->meleeWeaponState != 0)") + 1]
    # Graphics-only trail material selection is outside this collision fixture.
    branch = re.sub(r"EffectBlureShip_ChangeType\(.*?;", "", branch, flags=re.S)
    pieces += ["static void Invoke(PlayState* play, Player* this) { if (0) {} " + branch + "}"]
    (directory / "production.inc").write_text("\n".join(pieces))
    # Check both actual shipped geometry paths. Arbitrary user mod replacements
    # can have unrelated extents and are deliberately not inferred at runtime.
    sys.path[:0] = [str(ROOT / "tools/nei_held/SOURCE"), str(ROOT / "tools/nei_gi/SOURCE")]
    import numpy as np
    from equipment import BUILDERS
    model = BUILDERS["cane_of_byrna"]()
    authored = np.concatenate([part["p"] for part in model.parts]) * model.native_scale * 16
    legacy = np.array([[float(v.attrib[axis]) for axis in "XYZ"]
                       for part in (0, 1)
                       for v in ET.parse(ROOT / f"soh/assets/custom/objects/object_somaria/v_somaria_cane_vtx_{part}").getroot()])
    points = np.concatenate([authored, legacy])
    (directory / "mesh.inc").write_text("static Vec3f meshPoints[] = {\n" +
        ",\n".join("{%sf,%sf,%sf}" % tuple(format(float(n), ".8f") for n in p) for p in points) + "\n};")
    if "Player_ByrnaMeleeWeaponPostLimb(play, this);" in native:
        branch = native[native.index("} else if (ExtEquip_ByrnaTrailBegin())"):]
        branch = branch[:branch.index("} else if (this->meleeWeaponState != 0)")]
        assert "Player_ByrnaMeleeWeaponPostLimb(play, this);" in branch
        assert "func_80090A28" not in branch and "D_80126080" not in branch
    binary = str(directory / "test")
    extra = shlex.split(os.environ.get("BYRNA_TEST_CFLAGS", ""))
    if "--sanitize" in sys.argv:
        extra += ["-fsanitize=address,undefined,bounds", "-fno-omit-frame-pointer", "-g"]
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags()[1:], *extra, "-DCOMBO_BUILD", "-I" + td,
                    str(ROOT / "tests/byrna_collision/collision_test.c"), "-lm", "-o", binary], check=True)
    subprocess.run([binary], check=True)
