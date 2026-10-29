#!/usr/bin/env python3
"""Execute all changed production OoT light call expressions on native types."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

from run_tests import ROOT, flags, function


def main():
    cases = (
        ("soh/mods/transformation_masks/mm_player_form.cpp", "MmForm_CheckBarrierInput", 1),
        ("soh/mods/transformation_masks/mm_player_form.cpp", "MmForm_UpdateBarrier", 1),
        ("soh/mods/actors/cane_pacci.c", "Pacci_UhLightAt", 1),
        ("soh/mods/items/logic/item_lantern.c", "Lantern_UpdateLight", 1),
        ("soh/mods/items/logic/item_dominionrod.c", "DomRod_CreateLight", 1),
        ("soh/mods/items/logic/item_dominionrod.c", "DomRod_UpdateLight", 1),
        ("soh/mods/transformation_masks/rito_flight.inc.c", "MmForm_RitoSpawnFlame", 1),
        ("soh/soh/Enhancements/Graphics/RemoveSpinAttackDarkness.cpp", "Custom_EnMThunder_Update", 1),
        ("soh/expansions/sw97/actors/spells/z_magic_light.inc.c", "MagicLight_Update", 2),
    )
    calls, labels = [], []
    for path, name, count in cases:
        body = function((ROOT / path).read_text(), name)
        found = re.findall(r"\bLights_PointNoGlowSetInfo\s*\([^;]*\);", body, re.S)
        assert len(found) == count, (path, name, len(found))
        for index, call in enumerate(found):
            calls.append(re.sub(r"\bthis\b", "self", call))
            labels.append(path + ":" + name + ("#" + str(index + 1) if count > 1 else ""))
    sw97 = (ROOT / cases[-1][0]).read_text()
    begin = sw97.index("struct MagicLight;")
    end = sw97.index("} MagicLight;", begin) + len("} MagicLight;")
    constants = "\n".join(re.findall(r"^#define PACCI_UH_LIGHT_(?:R|G|B|RADIUS) [^\n]+",
                                     (ROOT / "soh/mods/actors/cane_pacci.c").read_text(), re.M))
    constants += "\n" + "\n".join(re.findall(r"^#define DOMROD_ORB_(?:ENV_R|ENV_G|ENV_B|LIGHT_RADIUS) [^\n]+",
                                             (ROOT / "soh/mods/items/logic/item_dominionrod.h").read_text(), re.M))
    with tempfile.TemporaryDirectory(prefix="oot-light-boundary-") as directory:
        work = Path(directory)
        (work / "light_source_types.inc").write_text(
            sw97[begin:end] + "\n" + constants + "\nstatic const char* kLightCallNames[] = {\n" +
            ",\n".join('"' + label + '"' for label in labels) + "\n};\n")
        (work / "light_calls.inc").write_text("switch (index) {\n" + "\n".join(
            "case " + str(index) + ": " + call + " break;" for index, call in enumerate(calls)) +
            "\ndefault: assert(false);\n}\n")
        binary = work / "light-calls"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags("soh"), "-I" + directory,
                        "-no-pie", str(Path(__file__).with_name("light_boundary_test.cpp")),
                        "-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
