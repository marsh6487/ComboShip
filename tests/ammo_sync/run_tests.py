#!/usr/bin/env python3
"""Exercise production ammo transfer helpers against both native save layouts.

The two hosts compile separately with their own native headers and inventory
tables. Only logging and the frozen OoT seed-option read are controlled engine
boundaries; inventory ownership, capacity and transfer code are production.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tests/shuffle_parity/boss_keys"))
from run_tests import function

parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix="combo-ammo-sync-") as directory:
    build = Path(directory)
    objects = []
    for game, host in (("soh", "oot"), ("mm", "mm")):
        sync = (ROOT / game / ("soh" if game == "soh" else "2s2h") /
                "FleetShipCombo/FleetSync.cpp").read_text()
        inv = (ROOT / game / "src/code/z_inventory.c").read_text()
        names = ("gUpgradeMasks", "gUpgradeNegMasks", "gUpgradeShifts",
                 "gUpgradeCapacities", "gItemSlots")
        tables = "\n".join(re.search(
            r"^(?:u32|u16|u8) " + name + r"\[[^]]*\](?:\[4\])? = \{.*?\};",
            inv, re.M | re.S)[0] for name in names)
        helpers = []
        for name in (("SyncAmmoCapacity", "RepairBombchuCapacity", "ApplyBombchuCapacity")
                     if game == "mm" else ("NativeBombchuCapacity", "ApplyBombchuCapacity", "SyncAmmoCapacity", "RecordBombchuCapacity")):
            if re.search(r"^void " + name + r"\(|^int " + name + r"\(", sync, re.M):
                helpers.append(function(sync, name))
        helpers += [function(sync, name) for name in ("PutInvItem", "ApplyInvItem", "ApplyFcRegistryToNatives")]
        # Verify the real snapshot entrypoint establishes this dependency before
        # the exact inventory helper below, not only in the test fixture.
        apply = function(sync, "ApplyShared")
        assert apply.index("ApplyBombchuCapacity(sh)") < apply.index('if (sh.contains("inv"))')
        declarations = "\n".join("extern " + line.split(" = ", 1)[0] + ";"
                                 for line in tables.splitlines() if " = {" in line)
        source = (HERE / (host + ".cpp")).read_text().replace("// TABLES", declarations)
        upgrade = function(inv, "Inventory_ChangeUpgrade")
        source = source.replace("// UPGRADE", upgrade[:upgrade.index("{")] + ";")
        source = source.replace("// PRODUCTION", "\n".join(helpers))
        path = build / (host + ".cpp")
        path.write_text(source)
        flags = ["-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX",
                 "-DGBI_S32_VTX", "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(ROOT)]
        for p in (game + "/include", game + "/include/PR", game,
                  game + ("/soh" if game == "soh" else "/2s2h"), "libultraship/include"):
            flags += ["-isystem", str(ROOT / p)]
        if game == "soh":
            flags += ["-DSOH_BUILD_DLL"]
            flags += ["-D" + name + "=Oot_" + name for name in
                      (*names, "gSaveContext", "Inventory_ChangeUpgrade")]
        if args.sanitize:
            flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-fno-omit-frame-pointer", "-fno-pie"]
        native = build / (host + "_inventory.c")
        ammo = ""
        if game == "mm":
            ammo = ('\n#include <stdbool.h>\nenum { VB_POWDER_KEG_CAP_AMMO };\n'
                    'static int GameInteractor_Should(int behavior, int initial) { return initial; }\n' +
                    function((ROOT / "mm/src/code/z_parameter.c").read_text(), "Inventory_ChangeAmmo"))
        native.write_text('#include "' + ("z64.h" if game == "soh" else "z64save.h") +
                          '"\n#include "macros.h"\nextern SaveContext gSaveContext;\n' + tables + "\n" + upgrade + ammo)
        native_obj = build / (host + "_inventory.o")
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", *flags[1:], "-c",
                        str(native), "-o", str(native_obj)], check=True)
        objects.append(str(native_obj))
        obj = build / (host + ".o")
        subprocess.run([os.environ.get("CXX", "c++"), *flags, "-c", str(path), "-o", str(obj)], check=True)
        objects.append(str(obj))
    flags = ["-std=c++20"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
    binary = build / "ammo_sync"
    subprocess.run([os.environ.get("CXX", "c++"), *flags, str(HERE / "test.cpp"),
                    *objects, "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
