#!/usr/bin/env python3
"""Exercise native pool generation and saved-item-compatible MM cane grants."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from, functions


def function(source, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError("Missing native function " + name)
    return block_from(source, match.start())


def arm(source, identity, next_identity):
    start = source.index("        case " + identity + ":")
    end = source.index("        case " + next_identity + ":", start)
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("all", "pool", "grant"), default="all")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    with tempfile.TemporaryDirectory(prefix="item-availability-") as temporary:
        build = Path(temporary)
        fleet = functions((ROOT / "mm/2s2h/FleetShipCombo/FleetShipCombo.cpp").read_text())
        (build / "unified_pool.inc").write_text(fleet["FleetCombo_UnifiedPoolActive"])
        placement = functions((ROOT / "mm/2s2h/Rando/Logic/PlacementConstraints.cpp").read_text())
        (build / "placement.cpp").write_text(
            '#include "Rando/Logic/Logic.h"\nnamespace Rando::Logic {\n' + "\n".join(
                placement[name] for name in ("DungeonItemToDungeon", "DungeonItemPlacementOption",
                                             "StaysAtVanillaCheck", "IsSongLocationItem")) + "\n}\n")
        native = (ROOT / "mm/mods/nei_save.cpp").read_text()
        cane = (ROOT / "mm/mods/items/logic/item_cane_of_somaria.c").read_text()
        conversion = (ROOT / "mm/2s2h/Rando/ConvertItem.cpp").read_text()
        header = (ROOT / "mm/mods/items/logic/item_cane_of_somaria.h").read_text()
        definitions = "\n".join(line for line in header.splitlines()
                                if re.match(r"#define CANE_(?:TYPE_\w+|SKILL_\w+)(?:\([^)]*\))? ", line)
                                and not line.endswith("\\"))
        definitions += "\n" + "\n".join(re.findall(r"^#define NEI_CANE_.*$", native, re.M))
        grants = (ROOT / "mm/2s2h/Rando/GiveItem.cpp").read_text()
        production = definitions + "\n" + "\n".join(function(native, name) for name in (
            "Nei_CaneHasSkill", "Nei_CaneGrantSkill", "Nei_CaneOwned", "Nei_CaneSetType",
            "Nei_SomariaSummonOwned", "Nei_CaneSetSkillSlot")) + "\n" + "\n".join(
                function(cane, name) for name in ("Cane_HasSkill", "Cane_GiveSkill"))
        production += "\n" + function(conversion, "ResolveCanePresentation")
        convert_body = conversion[conversion.index("RandoItemId Rando::ConvertItem("):]
        convert_arm = arm(convert_body, "RI_OOT_NEI_CANE_OF_SOMARIA", "RI_OOT_PROGRESSIVE_ROC")
        native_arms = arm(grants, "RI_OOT_NEI_CANE_OF_SOMARIA", "RI_OOT_NEI_SHOVEL")
        native_arms += arm(grants, "RI_OOT_SONG_FUGUE_OF_HOME", "RI_SONG_LULLABY")
        (build / "grant.cpp").write_text((ROOT / "tests/item_availability/grant_test.cpp").read_text().replace(
            "/* PRODUCTION */", production).replace("/* CONVERT_ARM */", convert_arm).replace(
                "/* GRANT_ARMS */", native_arms))
        flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(build),
                 "-include", str(ROOT / "tests/dungeon_rewards/headless.h")]
        for path in (".", "mm/include", "mm/include/PR", "mm/2s2h", "mm", "mm/src", "mm/assets",
                     "libultraship/include", "combo"):
            flags += ["-isystem", str(ROOT / path)]
        json_include = Path(os.environ.get("NLOHMANN_JSON_INCLUDE", "/usr/include"))
        if json_include.resolve() != Path("/usr/include"):
            flags += ["-isystem", str(json_include)]
        if args.sanitize:
            flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
        failures = 0
        for mode in ("combo", "standalone"):
            mode_flags = ["-DCOMBO_BUILD"] if mode == "combo" else []
            for case in (("pool", "grant") if args.case == "all" else (args.case,)):
                binary = build / (case + "-" + mode)
                sources = ([ROOT / "tests/item_availability/pool_test.cpp",
                            ROOT / "mm/2s2h/Rando/Logic/GeneratePools.cpp", build / "placement.cpp"]
                           if case == "pool" else [build / "grant.cpp"])
                subprocess.run([*compiler, *flags, *mode_flags, *map(str, sources), "-o", str(binary)], check=True)
                failures += subprocess.run([str(binary)], env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"},
                                           timeout=30).returncode != 0
        market = ROOT / "tests/item_availability/market_run_tests.py"
        if args.case == "all" and market.exists():
            failures += subprocess.run([sys.executable, "-B", str(market)], check=False).returncode != 0
        return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
