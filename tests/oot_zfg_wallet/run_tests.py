#!/usr/bin/env python3
"""Exercise OoT's production wallet resolution, grant, and display capacities."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def block(source, start):
    start = source.index(start)
    opening = source.index("{", start)
    end = opening + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]

inventory = (ROOT / "soh/src/code/z_inventory.c").read_text()
randomizer = (ROOT / "soh/soh/Enhancements/randomizer/randomizer.cpp").read_text()
parameter = (ROOT / "soh/src/code/z_parameter.c").read_text()
tables = "\n".join(re.search(r"^(?:u32|u16|u8) " + name + r"\[[^;]*?=\s*\{.*?\};",
                             inventory, re.M | re.S)[0]
                   for name in ("gUpgradeMasks", "gUpgradeNegMasks", "gUpgradeShifts", "gUpgradeCapacities"))
production = "\n".join((tables, block(inventory, "void Inventory_ChangeUpgrade("),
                        block(parameter, "void Rupees_ChangeBy(")))

with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    (temp / "wallet_production.inc").write_text(production)
    (temp / "zfg_grant.inc").write_text(block(randomizer, "        case RG_ZFG_WALLET: {"))
    includes = ["", "soh", "soh/include", "soh/include/PR", "soh/soh", "soh/src", "soh/assets",
                "libultraship/include", "combo", "combo/menu"]
    cxx = shlex.split(os.environ.get("CXX", "c++"))
    for combo in (False, True):
        binary = temp / "wallet"
        subprocess.run([*cxx, "-std=gnu++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
                        *(["-DCOMBO_BUILD"] if combo else []),
                        *["-I" + str(ROOT / name) for name in includes], "-I" + str(temp),
                        str(ROOT / "tests/oot_zfg_wallet/wallet_test.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print(f"PASS OoT ZFG wallet chain, direct receipt, capacity, and four-digit formatting"
              f" ({'Combo' if combo else 'standalone'})")
