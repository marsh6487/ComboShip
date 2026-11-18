#!/usr/bin/env python3
"""Execute native MM/OoT popout registrations with their real option types."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_back_equipment_tests import function
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags as soh_flags


def registration(source_path, cvar):
    source = (ROOT / source_path).read_text()
    route = f'.CVar("{cvar}")'
    if source.count(route) != 1:
        raise AssertionError(f"expected one native popout registration for {cvar}")
    anchor = source.index(route)
    builder = source.rfind("AddWidget(", 0, anchor)
    if builder < 0:
        raise AssertionError(f"missing native popout builder for {cvar}")
    start = source.rfind("\n", 0, builder) + 1
    end = source.index(";\n", anchor) + 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    mm_fixture = Path(__file__).with_name("mm_options_test.cpp").read_text()
    mm_fixture = mm_fixture.replace("/* PRODUCTION_POPOUT_REGISTRATION */", registration(
        "mm/2s2h/BenGui/BenMenu.cpp", "gWindows.SpinAttackMM"))
    # SaveEditor.h initializes readable labels through this native helper.
    mm_fixture = mm_fixture.replace("/* PRODUCTION_ENUM_LABEL_HELPER */", function(
        (ROOT / "mm/2s2h/ShipUtils.cpp").read_text(), "convertEnumToReadableName"))
    soh_fixture = Path(__file__).with_name("soh_options_test.cpp").read_text()
    soh_fixture = soh_fixture.replace("/* SOH_PAK_POPOUT_REGISTRATION */", registration(
        "soh/soh/SohGui/SohMenuNEI.cpp", "gOpenWindows.PakLoader"))
    soh_fixture = soh_fixture.replace("/* SOH_SPIN_POPOUT_REGISTRATION */", registration(
        "soh/soh/SohGui/SohMenuEnhancements.cpp", "gOpenWindows.SpinAttack"))
    includes = [ROOT / path for path in (
        "mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/mods",
        "libultraship/include", "combo", "combo/menu", "")]
    # Like other gate fixtures, consume the workflow's pinned headers explicitly.
    pinned_includes = [Path(path) for path in os.environ.get(
        "COMBO_NEI_FIXTURE_CPATH", "").split(os.pathsep) if path]
    common_flags = ["-O1", "-g", "-DFMT_HEADER_ONLY", "-ffunction-sections", "-fdata-sections",
                    *["-I" + str(path) for path in pinned_includes]]
    if args.sanitize:
        common_flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                         "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    mm_flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
                # MM's GNU target uses this for inherited UI enum/member names.
                "-fpermissive", "-w", "-include", str(ROOT / "mm/include/z64.h"),
                *["-I" + str(path) for path in includes]]
    hosts = (("MM", mm_fixture, mm_flags),
             ("OoT", soh_fixture, [*soh_flags(), "-DSPDLOG_HEADER_ONLY"]))
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    with tempfile.TemporaryDirectory(prefix="native-menu-popouts-") as temporary:
        build = Path(temporary)
        for host, fixture, flags in hosts:
            source = build / f"{host}_options_test.cpp"
            source.write_text(fixture)
            for mode, definitions in (("standalone", []), ("combo", ["-DCOMBO_BUILD"])):
                print(f"Building {host} popout options: {mode}, sanitize={args.sanitize}", flush=True)
                binary = build / f"{host}-{mode}"
                subprocess.run([*compiler, *common_flags, *flags, *definitions, str(source),
                                "-Wl,--gc-sections", "-o", str(binary)], check=True)
                subprocess.run([str(binary)], check=True)
                print(f"PASS {host} popout native options: {mode}, sanitize={args.sanitize}", flush=True)


if __name__ == "__main__":
    main()
