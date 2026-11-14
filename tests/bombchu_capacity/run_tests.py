#!/usr/bin/env python3
"""Compile native MM ammo policies and bounded production grant/display paths."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/shuffle_parity/boss_keys"))
from run_tests import function
sys.path.insert(0, str(ROOT / "scripts/ci"))
from native_test_abi import ABI_FLAGS


def table(source, declaration):
    return re.search(r"^" + declaration + r" = \{.*?^\};", source, re.M | re.S)[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--json-include", type=Path,
                        default=Path(os.environ.get("NLOHMANN_JSON_INCLUDE", "/usr/include")))
    args = parser.parse_args()
    inventory = (ROOT / "mm/src/code/z_inventory.c").read_text()
    parameter = (ROOT / "mm/src/code/z_parameter.c").read_text()
    give = function(parameter, "Item_GiveImpl")
    services = "\n".join(table(inventory, r"(?:u32|u16|u8) " + name + r"\[[^]]*\](?:\[4\])?")
                          for name in ("gBitFlags", "gUpgradeMasks", "gUpgradeNegMasks", "gUpgradeShifts",
                                       "gUpgradeCapacities", "gItemSlots"))
    services += "\n" + table(parameter, r"s16 sExtraItemBases\[\]")
    services += "\n" + re.search(r"^s16 sBombchuRefillCounts\[\] = \{[^}]*\};", parameter, re.M)[0]
    services += "\n" + function(inventory, "Inventory_ChangeUpgrade")
    services += "\n" + function(parameter, "Inventory_ChangeAmmo")
    services += "\n" + function(parameter, "Inventory_DeleteItem")
    services += "\n" + function(parameter, "Item_CheckObtainabilityImpl")
    services += "\n" + function(parameter, "Item_CheckObtainability")
    shop = (ROOT / "mm/src/overlays/actors/ovl_En_GirlA/z_en_girla.c").read_text()
    services += "\n" + function(shop, "EnGirlA_CanBuyBombchus")
    for name, first, last in (("NativeBombchuRefill", "item == ITEM_BOMBCHU", "(item >= ITEM_ARROWS_10)"),
                              ("NativeBombBagGrant", "item == ITEM_BOMB_BAG_20", "item == ITEM_WALLET_ADULT")):
        start = give.index("    } else if (" + first)
        end = give.index("    } else if (" + last, start)
        services += ("\nu8 " + name + "(PlayState* play, u8 item) {\n    if" +
                     give[start + len("    } else if"):end] + "    }\n    return ITEM_NONE;\n}\n")
    # Execute the exact Bombchu full-capacity predicates from C, D-pad and
    # pause drawing. Geometry/render submission is outside this fixture.
    pause = (ROOT / "mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c").read_text()
    predicates = re.findall(r"\(\(i == ITEM_BOMBCHU\) && [^\n]+?(?= \|\|)", parameter)
    predicates += re.findall(r"\(\(item == ITEM_BOMBCHU\) && [^\n]+?(?= \|\|)", pause)
    assert len(predicates) == 3
    for i, predicate in enumerate(predicates):
        services += (f"\nint NativeBombchuFull{i}(void) {{ const int i = ITEM_BOMBCHU; "
                     "const int item = ITEM_BOMBCHU; return " + predicate + "; }\n")
    services = '''#include "global.h"
#include <stdlib.h>
#include "functions.h"
#include "variables.h"
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
enum { VB_POWDER_KEG_CAP_AMMO };
static int GameInteractor_Should(int behavior, int initial) { return initial; }
s32 TradeAdult_IndexOfItem(u8 item) { return -1; }
void TradeAdult_ConsumeItem(u8 item) { abort(); }
''' + services

    grants = (ROOT / "mm/2s2h/Rando/GiveItem.cpp").read_text()
    bag = grants[grants.index("        case RI_BOMB_BAG_20:"):grants.index("        case RI_WALLET_ADULT:")]
    foreign = grants[grants.index("        case RI_OOT_BOMBCHU_BAG:"):
                     grants.index("        // OoT child-trade masks", grants.index("        case RI_OOT_BOMBCHU_BAG:"))]
    grant_inc = "void NativeGive(RandoItemId randoItemId) { switch(randoItemId) {\n" + bag + foreign + "default: abort(); } }\n"
    ben = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    get_start = ben.index('extern "C" COMBO_EXPORT int MM_GetSharedTier(')
    get_body = ben.index("    if (!IS_RANDO", get_start)
    get_switch = ben.index("    switch (", get_body)
    case_start = ben.index("        case ComboRando::SF_BOMBCHU_BAG:", get_switch)
    case_end = ben.index("        case ComboRando::SF_MAGIC:", case_start)
    grant_inc += ("\nint MM_GetSharedTier(int family) {\n" + ben[get_body:get_switch] +
                  "switch(static_cast<ComboRando::SharedFamily>(family)) {\n" + ben[case_start:case_end] +
                  "default: return 0; } }\n")
    raise_start = ben.index('extern "C" COMBO_EXPORT void MM_RaiseSharedTier(')
    raise_body = ben.index("    if (!IS_RANDO", raise_start)
    raise_end = ben.index("        RandoItemId base = RI_JUNK;", raise_body)
    grant_inc += "\nvoid NativeRaiseSharedTier(int family, int tier) {\n" + ben[raise_body:raise_end] + "abort();\n} }\n"
    convert = (ROOT / "mm/2s2h/Rando/ConvertItem.cpp").read_text()
    obtain = function(convert[convert.index("bool Rando::IsItemObtainable("):], "Rando::IsItemObtainable")
    start = obtain.index("        case RI_BOMBCHU:")
    end = obtain.index("        case RI_SHIELD_HERO:", start)
    grant_inc += ("\nbool NativeRandoBombchuObtainable(RandoItemId item) { switch(item) {\n" +
                  obtain[start:end] + "default: abort(); } return true; }\n")
    editor = (ROOT / "mm/2s2h/DeveloperTools/SaveEditor.cpp").read_text()
    editor_inc = function(editor, "NextItemInSlot")
    cheat = (ROOT / "mm/2s2h/Enhancements/Cheats/Infinite.cpp").read_text()
    editor_inc += "\nvoid NativeCheatRefill() {\n" + re.search(
        r"if \(INV_CONTENT\(ITEM_BOMBCHU\) == ITEM_BOMBCHU\) \{.*?\}", cheat, re.S)[0] + "\n}\n"

    flags = [*ABI_FLAGS, "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-ffunction-sections", "-fdata-sections", "-I" + str(ROOT)]
    for path in ("mm/include", "mm/include/PR", "mm/2s2h", "mm", "mm/src", "mm/assets",
                 "libultraship/include", "combo"):
        flags += ["-isystem", str(ROOT / path)]
    # The compiler already searches /usr/include. Adding it explicitly moves
    # libc ahead of libstdc++ and breaks cstdlib's include_next on GCC.
    if args.json_include.resolve() != Path("/usr/include"):
        flags += ["-isystem", str(args.json_include)]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    failures = 0
    with tempfile.TemporaryDirectory(prefix="bombchu-capacity-") as temporary:
        build = Path(temporary)
        (build / "services.c").write_text(services)
        (build / "grant.inc").write_text(grant_inc)
        (build / "editor.inc").write_text(editor_inc)
        flags += ["-I" + str(build)]
        for mode in ("original", "combo"):
            definitions = ["-DCOMBO_BUILD"] if mode == "combo" else []
            obj = build / (mode + ".o")
            subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=c11", *flags, *definitions,
                            "-c", str(build / "services.c"), "-o", str(obj)], check=True)
            for case in ("native", "grant", "editor"):
                binary = build / (mode + "-" + case)
                source = ROOT / ("tests/bombchu_capacity/" + case + "_test.cpp")
                subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", *flags,
                                *definitions, str(source), str(obj), "-Wl,--gc-sections", "-o", str(binary)],
                               check=True)
                result = subprocess.run([str(binary)], timeout=30,
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                print(f"{mode} {case}: {'PASS' if result.returncode == 0 else 'FAIL'}", flush=True)
                failures += result.returncode != 0
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
