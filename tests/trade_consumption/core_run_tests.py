#!/usr/bin/env python3
"""Compile the real trade-state helpers against each game's native save headers.

Only the unrelated OoT presentation renderer is excluded. Temporary generated
translation units/binaries live outside the checkout. --red-probe isolates the
pre-existing ownership reader so the missing consumption filter fails at an
assertion rather than because the new public entry points do not exist yet.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--game", choices=("oot", "mm", "both"), default="both")
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--red-probe", action="store_true")
parser.add_argument("--source-ref", help="Read trade helpers from a git ref to reproduce a historical failure")
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
fixture = (root / "tests/trade_consumption/core_test.cpp").read_text()
games = ("oot", "mm") if args.game == "both" else (args.game,)

with tempfile.TemporaryDirectory(prefix="trade-consumption-core-") as temporary:
    out = Path(temporary)
    for game in games:
        tree = "soh" if game == "oot" else "mm"
        relative = tree + "/mods/items/logic/trade_items.c"
        source = subprocess.check_output(["git", "show", args.source_ref + ":" + relative],
                                         cwd=root, text=True) if args.source_ref else (root / relative).read_text()
        production = source[source.index("#define TRADE_ADULT_COUNT"):]
        if game == "oot":
            production = production[:production.index("extern void* MmAssets_LoadResource")]
        unit = out / (game + ".cpp")
        definitions = ""
        if game == "oot":
            equipment = (root / "soh/mods/extended_equipment.h").read_text()
            definitions = re.search(r"^#define ITEM_EXT_BOOTS_2[^\n]*", equipment, re.M).group()
        unit.write_text(fixture.replace("// PRODUCTION_CORE", production).replace("// CORE_ITEM_DEFINES", definitions))
        includes = [tree, tree + "/include", tree + "/include/PR", tree + "/src",
                    tree + "/assets", tree + "/mods", tree + ("/soh" if game == "oot" else "/2s2h"),
                    "libultraship/include"]
        for combined in (False, True):
            flags = ["-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX",
                     "-DSOH_BUILD_DLL" if game == "oot" else "-DMM_BUILD_DLL"]
            if game == "mm":
                flags.append("-DCORE_MM")
            if combined:
                flags.append("-DCOMBO_BUILD")
            if args.red_probe:
                flags.append("-DCORE_RED_PROBE")
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
            binary = out / (game + ("-combo" if combined else "-standalone"))
            command = [os.environ.get("CXX", "g++-14"), "-std=c++20", "-Wall",
                       "-Wno-unused-function", "-Wno-unused-variable", "-Wno-unused-parameter",
                       "-Wno-enum-compare", "-Wno-parentheses", "-Wno-sign-compare",
                       *flags, *["-I" + str(root / path) for path in includes],
                       str(unit), "-o", str(binary)]
            print("BUILD", game, "combined" if combined else "standalone", flush=True)
            subprocess.run(command, check=True)
            subprocess.run([str(binary)], check=True)
