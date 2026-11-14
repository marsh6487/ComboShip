#!/usr/bin/env python3
"""Exercise exact MM handoff functions and randomizer callbacks with real trade state.

This is a source-function fixture, not a native engine or runtime test. Production
enums, slot table, macros and function bodies remain unchanged. Native Item_GiveImpl
is limited to its trade/mask arm; the cycle fixture stops before check-flag reset.
Only animation, text input, offer scheduling and hook dispatch are controlled.
--source-ref reads semantic call sites from a git ref, retaining the current core,
to reproduce the pre-fix failures independently of the core's own regression test.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--source-ref")
args = parser.parse_args()
HOOK_PATHS = {
    "mm/src/code/z_parameter.c",
    "mm/src/overlays/actors/ovl_En_Sellnuts/z_en_sellnuts.c",
    "mm/src/overlays/actors/ovl_En_Akindonuts/z_en_akindonuts.c",
    "mm/2s2h/Rando/ActorBehavior/ActorBehavior.cpp",
    "mm/2s2h/Rando/MiscBehavior/OnCycleSave.cpp",
    "mm/mods/extended_equipment.c",
}


def source(path):
    if args.source_ref and path in HOOK_PATHS:
        return subprocess.check_output(["git", "show", args.source_ref + ":" + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def block(text, opening):
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return end


def located(path, text, start, end):
    return f'\n#line {text.count(chr(10), 0, start) + 1} "{path}"\n' + text[start:end] + "\n"


def function(path, name):
    text = source(path)
    match = re.search(r"^(?:static\s+)?(?:s32|u8|u32|void|bool) " + re.escape(name) +
                      r"\([^;{]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError(f"Missing definition {name} in {path}")
    opening = text.index("{", match.start())
    return located(path, text, match.start(), block(text, opening))


def declaration(path, name):
    text = source(path)
    end = text.index("} " + name + ";") + len("} " + name + ";")
    start = text.rfind("typedef enum", 0, end)
    if start < 0:
        raise ValueError(f"Missing enum {name}")
    return text[start:end] + "\n"


def macro(path, name):
    text = source(path)
    match = re.search(r"^#define " + re.escape(name) + r"(?:\(|\s).*$", text, re.M)
    if not match:
        raise ValueError(f"Missing macro {name} in {path}")
    end = match.end()
    while text[:end].endswith("\\"):
        end = text.index("\n", end + 1)
    return text[match.start():end] + "\n"


def array(path, name):
    text = source(path)
    match = re.search(r"^(?:u8|u16) " + name + r"\[[^;]*?=\s*\{", text, re.M)
    opening = text.index("{", match.start())
    return located(path, text, match.start(), block(text, opening) + 1)


item, interface, player, save = ["mm/include/" + p for p in
                                 ("z64item.h", "z64interface.h", "z64player.h", "z64save.h")]
constants = "/* Exact production enums and macros. */\n"
for path, names in [(item, ("InventorySlot", "ItemId", "QuestItem", "GetItemId", "EquipmentType")),
                    (interface, ("EquipSlot", "DpadEquipSlot")),
                    (player, ("PlayerTransformation", "PlayerItemAction"))]:
    for name in names:
        constants += declaration(path, name)
for path, names in [
    (item, ("ITEM_MM_MOONS_TEAR", "ITEM_MM_DEED_LAND", "ITEM_MM_DEED_SWAMP", "ITEM_MM_DEED_MOUNTAIN",
            "ITEM_MM_DEED_OCEAN", "ITEM_MM_ROOM_KEY", "ITEM_MM_LETTER_KAFEI", "ITEM_MM_SPECIAL_DELIVERY",
            "ITEM_TRADE_PLACEHOLDER", "ITEM_TRADE_PREV", "ITEM_TRADE_NEXT")),
    ("mm/mods/nei_oot_compat.h", ("ITEM_POCKET_EGG", "ITEM_POCKET_CUCCO", "ITEM_COJIRO", "ITEM_ODD_MUSHROOM",
            "ITEM_ODD_POTION", "ITEM_SAW", "ITEM_SWORD_BROKEN", "ITEM_PRESCRIPTION", "ITEM_FROG", "ITEM_EYEDROPS",
            "ITEM_CLAIM_CHECK", "ITEM_WEIRD_EGG", "ITEM_CHICKEN", "ITEM_LETTER_ZELDA", "EQUIP_TYPE_BOOTS")),
    ("mm/mods/extended_equipment.h", ("EXT_EQUIP_OWNED_SHIFT",)),
    ("mm/include/z64pause_menu.h", ("ITEM_GRID_ROWS", "ITEM_GRID_COLS", "ITEM_NUM_SLOTS")),
    (save, ("SLOT", "INV_CONTENT", "CUR_FORM", "BUTTON_ITEM_EQUIP", "C_SLOT_EQUIP", "GET_CUR_FORM_BTN_ITEM",
            "GET_CUR_FORM_BTN_SLOT", "SET_CUR_FORM_BTN_ITEM", "SET_CUR_FORM_BTN_SLOT", "DPAD_BUTTON",
            "DPAD_BUTTON_ITEM_EQUIP", "DPAD_SLOT_EQUIP", "DPAD_GET_CUR_FORM_BTN_ITEM", "DPAD_GET_CUR_FORM_BTN_SLOT",
            "DPAD_SET_CUR_FORM_BTN_ITEM", "DPAD_SET_CUR_FORM_BTN_SLOT", "PACK_WEEKEVENTREG_FLAG", "WEEKEVENTREG",
            "GET_WEEKEVENTREG", "CHECK_WEEKEVENTREG", "SET_WEEKEVENTREG", "WEEKEVENTREG_57_04",
            "WEEKEVENTREG_86_01", "WEEKEVENTREG_08_80", "WEEKEVENTREG_32_40", "WEEKEVENTREG_32_80",
            "WEEKEVENTREG_33_01", "WEEKEVENTREG_33_02", "WEEKEVENTREG_RECEIVED_LAND_TITLE_DEED")),
]:
    for name in names:
        constants += macro(path, name)

param = "mm/src/code/z_parameter.c"
native = array("mm/src/code/z_inventory.c", "gItemSlots")
text = source(param)
start = text.index("} else if ((item >= ITEM_MOONS_TEAR) && (item <= ITEM_MASK_GIANT))") + len("} else ")
native += "\nstatic u8 Item_GiveImpl(PlayState* play, u8 item) {\n    s16 temp, i;\n"
native += located(param, text, start, block(text, text.index("{", start))) + "return ITEM_NONE;\n}\n"
native += "static u8 ItemGrantAudit_ItemGive(PlayState*, u8);\n"
for path, names in [(param, ("Item_Give", "ItemGrantAudit_ItemGive", "Inventory_DeleteItem", "Inventory_ReplaceItem")),
                    ("mm/src/code/z_msgevent.c", ("MsgEvent_DeleteItem",)),
                    ("mm/src/overlays/actors/ovl_En_Bjt/z_en_bjt.c", ("EnBjt_TakeItem",))]:
    for name in names:
        native += function(path, name)
path = "mm/src/code/z_sram_NES.c"
text = source(path)
start = text.index("    // NEI: the item argument only unbinds C buttons")
end = text.index("    // #endregion", start) + len("    // #endregion")
native += "\nvoid Probe_NativeTradeCycleReset(PlayState* play) { s32 j;\n"
native += located(path, text, start, end) + "}\n"

# Execute the unchanged boots migration condition and body. Other equipment
# initialization (scene actors, tunic migration and graphics) is outside this fixture.
path = "mm/mods/extended_equipment.c"
native += function(path, "ExtEquip_GetBit") + function(path, "ExtEquip_HasItem")
text = source(path)
start = text.index("    if (Nei_Save()->extBootsLayoutVersion < 1)")
native += "\nvoid Probe_OldBootsMigration(void) {\n"
native += located(path, text, start, block(text, text.index("{", start))) + "}\n"

sell = "mm/src/overlays/actors/ovl_En_Sellnuts/z_en_sellnuts.c"
akind = "mm/src/overlays/actors/ovl_En_Akindonuts/z_en_akindonuts.c"
actor_head = ('#include "mm_fixture.h"\nstatic AnimationHeader animation;\n'
              'static AnimationInfo sAnimationInfo[32] = { [0 ... 31] = { &animation } };\n')
sell_unit = actor_head + function(sell, "func_80ADBAB8") + function(sell, "func_80ADBBEC")
akind_unit = actor_head + function(akind, "func_80BECFBC") + function(akind, "func_80BEF20C") + function(akind, "func_80BEF360")

rando = '#include "mm_fixture.h"\n'
rando += function("mm/2s2h/Rando/ActorBehavior/ActorBehavior.cpp", "MiscVanillaBehaviorHandler")
path = "mm/2s2h/Enhancements/Cycle/KeepExpressMail.cpp"
text = source(path)
opening = text.index("{", text.index("COND_VB_SHOULD"))
rando += "\nstatic void ProbeKeepExpress(bool* should, va_list args)\n" + located(path, text, opening, block(text, opening))
for file, name in [("EnSellnuts", "Sell"), ("EnAkindonuts", "Akind")]:
    path = "mm/2s2h/Rando/ActorBehavior/" + file + ".cpp"
    text = source(path)
    opening = text.index("{", text.index("[](Actor* actor)"))
    if name == "Sell":
        rando += array(path, "D_80ADD930_copy")
    rando += f"\nstatic void ProbeRando{name}(Actor* actor)\n" + located(path, text, opening, block(text, opening))
path = "mm/2s2h/Rando/MiscBehavior/OnCycleSave.cpp"
text = source(path)
opening = text.index("{", text.index("void Rando::MiscBehavior::AfterEndOfCycleSave()"))
end = text.index("    // Unset any flags used for checks", opening)
rando += '\nSaveContext saveContextCopy;\nvoid Probe_RandoTradeCycleRestore(void)\n'
rando += located(path, text, opening, end) + "}\n"
rando += r'''
int GameInteractor_Should(int id, int initial, ...) {
    bool should = initial;
    va_list args;
    va_start(args, initial);
    if (fixtureRando) {
        va_list copy; va_copy(copy, args);
        MiscVanillaBehaviorHandler(id, &should, copy);
        va_end(copy);
    }
    if (fixtureKeepExpress && id == VB_MSG_SCRIPT_DEL_ITEM) {
        va_list copy; va_copy(copy, args);
        ProbeKeepExpress(&should, copy);
        va_end(copy);
    }
    if (fixtureVeto) should = false;
    va_end(args);
    return should;
}
void Probe_RandoActorUpdate(Actor* actor) {
    if (!fixtureRando) return;
    if (actor->id == ACTOR_EN_SELLNUTS) ProbeRandoSell(actor);
    if (actor->id == ACTOR_EN_AKINDONUTS) ProbeRandoAkind(actor);
}
'''

with tempfile.TemporaryDirectory(prefix="trade-consumption-mm-") as temporary:
    build = Path(temporary)
    (build / "mm_constants.inc").write_text(constants)
    trade = source("mm/mods/items/logic/trade_items.c")
    (build / "mm_trade.inc").write_text(trade[trade.index("#define TRADE_ADULT_COUNT"):])
    (build / "mm_native_functions.inc").write_text(native)
    (build / "mm_sell.c").write_text(sell_unit)
    (build / "mm_akind.c").write_text(akind_unit)
    (build / "mm_rando.cpp").write_text(rando)
    failures = 0
    for mode, defines in [("standalone", []), ("combo", ["-DCOMBO_BUILD"])]:
        flags = ["-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-Wno-parentheses", "-O0", *defines,
                 "-I" + str(build), "-I" + str(HERE)]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
        objects = []
        for unit in [HERE / "mm_native.c", build / "mm_sell.c", build / "mm_akind.c",
                     build / "mm_rando.cpp", HERE / "mm_hooks_test.cpp"]:
            is_cpp = unit.suffix == ".cpp"
            obj = build / (mode + "-" + unit.stem + ".o")
            compiler = os.environ.get("CXX", "g++-14") if is_cpp else os.environ.get("CC", "cc")
            subprocess.run([compiler, "-std=c++20" if is_cpp else "-std=gnu11", *flags,
                            "-c", str(unit), "-o", str(obj)], check=True)
            objects.append(str(obj))
        binary = build / ("mm-hooks-" + mode)
        subprocess.run([os.environ.get("CXX", "g++-14"), *flags, *objects, "-o", str(binary)], check=True)
        print("MM semantic trade hooks:", mode, flush=True)
        failures += subprocess.run([str(binary)]).returncode != 0
    raise SystemExit(1 if failures else 0)
