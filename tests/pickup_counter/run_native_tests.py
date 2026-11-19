#!/usr/bin/env python3
"""Compile the real pickup adapters; execute their actual native inventory reads."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--syntax", action="store_true", help="Also compile the complete GUI/hook translation units")
parser.add_argument("--deps", type=Path, help="Local dependency sources for full GUI syntax compilation")
args = parser.parse_args()
compiler = shlex.split(os.environ.get("CXX", "c++"))
environment = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}

with tempfile.TemporaryDirectory(prefix="pickup-native-") as temp:
    directory = Path(temp)
    for host in ("soh", "mm"):
        source_path = ROOT / host / ("soh/Enhancements/Graphics/PickupCounter.cpp" if host == "soh"
                                     else "2s2h/Enhancements/Graphics/PickupCounter.cpp")
        source = source_path.read_text()
        inventory = (ROOT / host / "src/code/z_inventory.c").read_text()
        tables = []
        for name in ("gUpgradeMasks", "gUpgradeShifts", "gUpgradeCapacities", "gItemSlots"):
            tables.append(re.search(r"^(?:u32|u16|u8) " + name + r"\[[^;]*?=\s*\{.*?\};",
                                    inventory, re.M | re.S)[0])
        (directory / "native_inventory.inc").write_text('extern "C" {\n' + "\n".join(tables) + "\n}\n")
        (directory / "native_adapters.inc").write_text("\n".join(
            function(source, name) for name in ("ReadSnapshot", "ReceiptKind", "IconItem")))
        includes = ["", host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + "/mods", host + ("/soh" if host == "soh" else "/2s2h"),
                    "libultraship/include", "libultraship/src", "combo", "combo/menu", "combo/gui"]
        flags = ["-std=gnu++20", "-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX",
                 "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0",
                 *["-I" + str(ROOT / path) for path in includes], "-I" + str(directory)]
        flags += ["-I" + path for name in ("IMGUI_DIR", "NLOHMANN_JSON_INCLUDE", "COMBO_NEI_FIXTURE_CPATH")
                  for path in os.environ.get(name, "").split(os.pathsep) if path]
        if args.deps:
            flags += ["-I" + str(args.deps / path) for path in
                      ("include", "imgui", "spdlog/include", "json/single_include", "thread-pool/include", "SDL2/include")]
        # Use the same build-owned CVar names as the host, including libultraship.
        variables = {}
        for path in ("libultraship/cmake/cvars.cmake", "CMake/soh-cvars.cmake" if host == "soh"
                     else "CMake/2ship-cvars.cmake", "CMake/lus-cvars.cmake"):
            for key, value in re.findall(r'set\((CVAR_\w+) "([^"\n]+)"', (ROOT / path).read_text()):
                for previous, replacement in variables.items():
                    value = value.replace("${" + previous + "}", replacement)
                variables[key] = value
        flags += [f'-D{key}="{value}"' for key, value in variables.items() if "${" not in value]
        for combo in (False, True):
            mode = ["-DCOMBO_BUILD"] if combo else []
            native_flags = flags + (["-DPICKUP_TEST_MM"] if host == "mm" else [])
            if args.sanitize:
                native_flags += ["-g", "-O1", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                                 "-fno-pie", "-no-pie"]
            binary = directory / "test"
            subprocess.run([*compiler, *native_flags, *mode, str(ROOT / "tests/pickup_counter/native_test.cpp"),
                            "-o", str(binary)], check=True, cwd=ROOT)
            subprocess.run([str(binary)], check=True, env=environment)
            if args.syntax:
                subprocess.run([*compiler, *flags, *mode, "-DFMT_HEADER_ONLY", "-fsyntax-only", str(source_path)],
                               check=True, cwd=ROOT)
                print(f"PASS full {host} pickup GUI/hook compilation ({'Combo' if combo else 'standalone'})", flush=True)
