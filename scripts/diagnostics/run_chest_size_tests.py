#!/usr/bin/env python3
"""Run production chest sizing/update logic without a ROM, then compile the actor."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def main():
    source_path = ROOT / "soh/src/overlays/actors/ovl_En_Box/z_en_box.c"
    source = source_path.read_text()
    chest = functions(source)
    actor = functions((ROOT / "soh/src/code/z_actor.c").read_text())
    category = functions((ROOT / "soh/soh/Enhancements/randomizer/item_category_adj.cpp").read_text())
    hooks = (ROOT / "soh/soh/Enhancements/randomizer/hook_handlers.cpp").read_text()
    fairy = functions(hooks.replace('extern "C" bool ', "bool "))["Randomizer_IsChestStrayFairy"]
    foreign = (ROOT / "combo/rando/CrossForeign.h").read_text()
    foreign_types = re.search(r"enum GameId[^;]+;", foreign)[0]
    foreign_types += "\n" + re.search(r"struct ForeignItem \{.*?\n\};", foreign, re.S)[0]
    selected = [actor[name] for name in ("Actor_SetScale", "Actor_SetFocus")]
    selected += [category["Randomizer_AdjustItemCategory"]]
    selected += [body for name, body in chest.items() if name in {
        "EnBox_IsTimeGateChest", "EnBox_SetupAction", "EnBox_WaitOpen", "EnBox_SpawnIceSmoke",
        "EnBox_LoadChestDL", "EnBox_LoadChestPair", "EnBox_UpdateSizePosition", "EnBox_UpdateTexture", "EnBox_Update"}]
    prototypes = "\n".join(body[:body.index("{")] + ";" for body in selected)
    preamble = "\n".join(re.findall(r"^#define ENBOX_MOVE_.*$", source, re.M))
    preamble += "\n" + re.search(r"static AnimationHeader\* sAnimations\[4\] = \{.*?\};", source, re.S)[0]
    inventory = (ROOT / "soh/src/code/z_inventory.c").read_text()
    for name in ("gItemSlots", "gBitFlags"):
        preamble += "\n" + re.search(r"u\d+ " + name + r"\[\] = \{.*?\};", inventory, re.S)[0]
    flags = ["-std=gnu11", "-O1", "-g", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
             "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types",
             "-Wno-discarded-array-qualifiers", "-Wno-int-conversion",
             "-Ilibultraship/include", "-Isoh/include", "-Isoh/src", "-Isoh/assets", "-Isoh"]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            flags.append(f'-D{key}="{value}"')
    with tempfile.TemporaryDirectory(prefix="chest-size-") as directory:
        folder = Path(directory)
        (folder / "chest_size_production.inc").write_text(
            preamble + "\n" + prototypes + "\n" + "\n".join(selected))
        (folder / "chest_fairy_production.inc").write_text(fairy)
        (folder / "chest_foreign_types_production.inc").write_text("namespace ComboRando {\n" + foreign_types + "\n}")
        compiler = os.environ.get("CC", "cc")
        cxx = os.environ.get("CXX", "c++")
        cpp_flags = [flag for flag in flags if flag not in {
            "-std=gnu11", "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types",
            "-Wno-discarded-array-qualifiers", "-Wno-int-conversion"}]
        sanitizers = ["-fsanitize=undefined", "-fno-sanitize-recover=undefined"]
        for mode, defines in (("oot", []), ("combo", ["-DCOMBO_BUILD"])):
            native = folder / (mode + "_chest.o")
            bridge = folder / (mode + "_fairy.o")
            binary = folder / (mode + "_chest_size_test")
            subprocess.run([compiler, *flags, *defines, "-I" + directory, *sanitizers, "-c",
                            "soh/tests/chest_size_test.c", "-o", str(native)], cwd=ROOT, check=True)
            subprocess.run([cxx, "-std=c++20", *cpp_flags, *defines, "-I" + directory, *sanitizers, "-c",
                            "soh/tests/chest_foreign_fixture.cpp", "-o", str(bridge)], cwd=ROOT, check=True)
            subprocess.run([cxx, *sanitizers, str(native), str(bridge), "-lm", "-o", str(binary)],
                           cwd=ROOT, check=True)
            subprocess.run([str(binary)], cwd=ROOT, check=True)
            subprocess.run([compiler, *flags, *defines, "-fsyntax-only", str(source_path)], cwd=ROOT, check=True)
            print("PASS: complete chest actor and C/C++ fairy bridge compile for " + mode)


if __name__ == "__main__":
    main()
