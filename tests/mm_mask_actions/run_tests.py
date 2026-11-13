#!/usr/bin/env python3
"""Compile MM's real mask-action gate, B getters, and native command branches.

This is a source-function regression fixture, not an engine build or game test.
Only external hooks, labels, and actor/action boundaries are substituted.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, default=HERE.parents[1])
args = parser.parse_args()
root = args.source_root.resolve()


def source(path):
    return (root / path).read_text()


def declaration(path, name, kind="enum"):
    text = source(path)
    end = text.index("} " + name + ";") + len("} " + name + ";")
    start = text.rfind("typedef " + kind, 0, end)
    if start < 0:
        raise ValueError(f"Missing {kind} {name} in {path}")
    return text[start:end] + "\n"


def macro(path, name):
    match = re.search(r"^#define " + re.escape(name) + r"(?:\(|\s).*$", source(path), re.M)
    if not match:
        raise ValueError(f"Missing macro {name} in {path}")
    return match.group(0) + "\n"


def function(path, name, optional=False):
    text = source(path)
    match = re.search(r"^(?:static\s+)?(?:bool|void|u16|ItemId|s32) " + name + r"\(", text, re.M)
    if not match:
        if optional:
            return ""
        raise ValueError(f"Missing function {name} in {path}")
    opening = text.index("{", match.end())
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    line = text.count("\n", 0, match.start()) + 1
    return f'\n#line {line} "{path}"\n' + text[match.start():end] + "\n"


item = "mm/include/z64item.h"
interface = "mm/include/z64interface.h"
player = "mm/include/z64player.h"
save = "mm/include/z64save.h"
equipment = "mm/mods/extended_equipment.h"
constants = "/* Extracted from the current production headers. */\n"
for path, name in [
    (item, "EquipmentType"), (item, "ItemId"), (interface, "EquipSlot"),
    (interface, "DpadEquipSlot"), (interface, "DoAction"),
    (interface, "DoActionSegmentIndex"), (player, "PlayerTransformation"),
    (player, "PlayerItemAction"),
]:
    constants += declaration(path, name)
constants += macro(player, "GET_MASK_FROM_IA")
constants += declaration(player, "PlayerMask")
constants += declaration(save, "ButtonStatus")
constants += declaration(interface, "ActionLabel", "struct")
for path, names in [
    (item, ["ITEM_SWORD_MASTER", "ITEM_SWORD_BGS", "ITEM_SWORD_KNIFE", "ITEM_EXT_BUTTON",
            "ITEM_NET", "ITEM_ROD_FIRE", "ITEM_CANE_OF_SOMARIA"]),
    (equipment, ["ITEM_EXT_SWORD_1", "ITEM_EXT_SWORD_2", "ITEM_EXT_SWORD_3", "ITEM_EXT_SHIELD_1"]),
    (save, ["CUR_FORM", "BUTTON_ITEM_EQUIP", "CUR_FORM_EQUIP"]),
]:
    constants += "".join(macro(path, name) for name in names)

functions = "/* Exact production function bodies, with original source locations. */\n"
functions += function("mm/mods/extended_equipment.c", "ExtEquip_GetItemId")
functions += function("mm/src/code/z_parameter.c", "Interface_IsExtraMaskActionSwordItem", optional=True)
for path, name in [
    ("mm/src/code/z_parameter.c", "Interface_SetBButtonPlayerDoAction"),
    ("mm/src/code/z_inventory.c", "Inventory_GetBtnBItem"),
    ("mm/src/code/z_player_lib.c", "Player_GetItemOnButton"),
]:
    functions += function(path, name)

# Keep the native ITEM_F0/F1/F2 branches intact, including spawn/cooldown guards.
# Other item-use states belong to the engine and are outside this gate fixture.
path = "mm/src/overlays/actors/ovl_player_actor/z_player.c"
text = source(path)
start = text.index("} else if (item == ITEM_F0) {") + len("} else ")
end = text.index("} else if ((Player_BButtonSwordFromIA", start) + 1
line = text.count("\n", 0, start) + 1
functions += "\nstatic void RunNativeMaskCommand(PlayState* play, Player* this, ItemId item) {\n"
functions += f'#line {line} "{path}"\n' + text[start:end] + "\n}\n"

with tempfile.TemporaryDirectory(prefix="mm-mask-actions-") as folder:
    build = Path(folder)
    (build / "production_constants.inc").write_text(constants)
    (build / "production_functions.inc").write_text(functions)
    for mode, defines in [("standalone", []), ("combo", ["-DCOMBO_BUILD"])]:
        binary = build / ("mask-actions-" + mode)
        subprocess.run([
            os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-Wno-enum-compare", "-O0", *defines, "-I", str(build),
            str(HERE / "mask_actions_test.c"), "-o", str(binary),
        ], check=True)
        print(f"MM mask actions: {mode} source-function fixture", flush=True)
        subprocess.run([str(binary)], check=True)
