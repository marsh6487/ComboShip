#!/usr/bin/env python3
"""Exercise the native wallet draw and cross-game render metadata."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    end = opening + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]

randomizer = ROOT / "soh/soh/Enhancements/randomizer"
draw = (randomizer / "draw.cpp").read_text()
producer = (randomizer / "NeiGiPresentation.cpp").read_text()
receiver = (ROOT / "combo/menu/ComboForeignDrawOOT.h").read_text()
item_table = (randomizer / "item_list.cpp").read_text()

bindings = set(re.findall(r"itemTable\[(RG_\w+)\].SetCustomDrawFunc\(Randomizer_DrawWallet\);", item_table))
assert bindings == {"RG_CHILD_WALLET", "RG_ADULT_WALLET", "RG_GIANT_WALLET", "RG_TYCOON_WALLET",
                    "RG_ZFG_WALLET", "RG_WALLET_INF"}
animated_gate = receiver[receiver.index("    if (rcStatic == 0 ||"):].split("{", 1)[0]
validation = re.search(r"    if \(\(raw.drawKind == CW_DRAW_KIND_MAGIC_MIRROR[^;]+;", receiver)[0]
foreign_case = re.search(r"        case CW_DRAW_KIND_ZFG_WALLET:\n[^;]+;\n            break;", receiver)[0]

with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    (temp / "wallet_gi_binding.inc").write_text(
        function(draw, "static void OOT_DrawZfgWalletModel(")
        + "\n" + function(draw, "void Randomizer_DrawWallet(")
        + "\n" + function(producer, "static bool NeiGi_DescribeWallet(")
        + "\n" + function(receiver, "inline void OOT_DrawForeignZfgWallet("))
    (temp / "wallet_gi_receive.inc").write_text(animated_gate
        + "{ return ComboForeignResolve::Unknown; }\n" + validation)
    (temp / "wallet_gi_receive_draw.inc").write_text(foreign_case)
    includes = ["", "soh", "soh/include", "soh/include/PR", "soh/soh", "soh/src", "soh/assets",
                "libultraship/include", "combo", "combo/menu"]
    cxx = shlex.split(os.environ.get("CXX", "c++"))
    binary = temp / "wallet_gi"
    subprocess.run([*cxx, "-std=gnu++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0", "-DCOMBO_BUILD",
                    *["-I" + str(ROOT / name) for name in includes], "-I" + str(temp),
                    str(ROOT / "tests/oot_zfg_wallet/wallet_gi_test.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("PASS OoT native and foreign wallet models, optional tint and zero-resource recipe validation")
