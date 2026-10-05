#!/usr/bin/env python3
"""Run production pause wheels and button accessors with native game structures."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function


def main():
    for host in ("mm", "soh"):
        overlay = ("src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c" if host == "mm"
                   else "src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
        ui = (ROOT / host / overlay).read_text()
        inv = (ROOT / host / "mods/extended_inventory.c").read_text()
        buttons = (ROOT / host / "mods/ext_buttons/ext_buttons.cpp").read_text()
        prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
SaveContext gSaveContext{};
static NeiSaveData state{};
static int icons[8]{};
static int page = 1;
static s16 gCurrentItemCyclingSlot = -1;
extern "C" NeiSaveData* Nei_Save() { return &state; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return state.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { state.ownedItems[slot - 24] = item; }
int ExtInv_GetCurrentPage() { return page; }
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
extern "C" s32 TradeAdult_OwnedCount() { return 0; }
extern "C" u8 TradeAdult_CellItem() { return ITEM_NONE; }
'''
        if host == "mm":
            prefix += r'''
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#define GRACE_HOURGLASS_KALEIDO_CELL (SLOT_PHANTOM_HOURGLASS - 24)
#define SLATE_KALEIDO_CELL (SLOT_SHEIKAH_SLATE - 24)
typedef void (*KaleidoWheelCycleFunc)(PlayState*, s32);
extern "C" void Audio_PlaySfx(u16) {}
extern "C" void Interface_LoadItemIconImpl(PlayState*, u8 btn) { ++icons[btn]; }
extern "C" void Interface_Dpad_LoadItemIconImpl(PlayState*, u8 btn) { ++icons[btn + 4]; }
void ExtInv_RefreshButtonIconsForItem(PlayState*, uint16_t) {}
uint8_t ExtInv_GetOotSlotItem(int) { return ITEM_NONE; }
void ExtInv_SetOotSlotItem(int, uint8_t) {}
'''
            button_names = ("ExtButton_GetItem", "ExtButton_SetItem", "ExtButton_ClearItem",
                            "ExtButton_GetDpadItem", "ExtButton_SetDpadItem")
            slate_names = ("Slate_KaleidoHandle", "KaleidoScope_ResetItemCycling")
        else:
            prefix += r'''
static u8 sGraceHourglassSelectorActive = 0;
static u8 sSlateSelectorActive = 0;
extern "C" uint8_t Picto_IsOwned() { return 0; }
extern "C" uint8_t PowerKeg_IsOwned() { return 0; }
Vec3f gSfxDefaultPos{};
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;
extern "C" void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
extern "C" void Interface_LoadItemIcon1(PlayState*, u16 btn) { ++icons[btn]; }
'''
            button_names = ("ExtButton_GetItem", "ExtButton_SetItem", "ExtButton_ClearItem")
            slate_names = ("Slate_KaleidoCycle", "Slate_HandleKaleidoSelector", "KaleidoScope_ResetItemCycling")
        production = "\n".join(function(inv, n) for n in (
            "GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant",
            "Slate_RuneOwned", "Slate_GrantRune", "Slate_RuneCount", "Slate_RuneAt", "Slate_GetRune", "Slate_SetRune",
            "Slate_RuneNeighbor"))
        production += "\n" + "\n".join(function(buttons, n) for n in button_names)
        production += "\n" + function(ui, "KaleidoScope_ResetItemCycling")
        production += "\n" + function(ui, "Slate_KaleidoSyncTitle")
        production += "\n" + "\n".join(function(ui, n) for n in (
            "KaleidoWheel_Run", "GraceHourglass_KaleidoCycle", "GraceHourglass_KaleidoHandle",
            *(n for n in slate_names if n != "KaleidoScope_ResetItemCycling")))
        checks = r'''
int main() {
    PlayState play{};
    for (u16 first : {u16(ITEM_HYLIAS_GRACE), u16(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        state = {};
        for (auto& item : state.ownedItems) item = ITEM_NONE;
        gSaveContext = {};
        std::memset(icons, 0, sizeof(icons));
        play = {};
        gCurrentItemCyclingSlot = -1;
        GraceHourglass_Grant(first);
        GraceHourglass_Grant(first == ITEM_HYLIAS_GRACE ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_HYLIAS_GRACE);
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 17;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = first;
        play.pauseCtx.namedItem = first;
        /* SET_BUTTONS */
        play.state.input[0].press.button = BTN_A;
        GraceHourglass_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == 17);
        play.state.input[0].press.button = 0;
        /* STICK_RIGHT */
        GraceHourglass_KaleidoHandle(&play);
        const u16 next = first == ITEM_HYLIAS_GRACE ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_HYLIAS_GRACE;
        assert(ExtInv_GetSlotItem(41) == next);
        assert(play.pauseCtx.cursorItem[PAUSE_ITEM] == next);
        assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
        assert(state.hyliasGraceOwned && state.phantomHourglassOwned);
        /* CHECK_BUTTONS */
        /* STICK_LEFT */
        GraceHourglass_KaleidoHandle(&play);
        assert(ExtInv_GetSlotItem(41) == first);
        /* STICK_ZERO */
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 16;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = ITEM_NONE;
        GraceHourglass_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == -1);
        play.pauseCtx.cursorSlot[PAUSE_ITEM] = 17;
        play.pauseCtx.cursorItem[PAUSE_ITEM] = first;
        GraceHourglass_KaleidoHandle(&play);
        assert(gCurrentItemCyclingSlot == -1);
    }
    state.slateRunesOwned = 0x1F;
    state.slateMode = 0;
    ExtInv_SetSlotItem(SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE);
    play = {};
    play.pauseCtx.cursorSlot[PAUSE_ITEM] = SLOT_SHEIKAH_SLATE - 24;
    play.pauseCtx.cursorItem[PAUSE_ITEM] = EXT_ITEM_SHEIKAH_SLATE;
    play.state.input[0].press.button = BTN_A;
    /* SLATE_HANDLE */
    play.state.input[0].press.button = 0;
    /* STICK_RIGHT */
    for (int rune = 1; rune <= 5; ++rune) {
        play.pauseCtx.namedItem = EXT_ITEM_SHEIKAH_SLATE;
        /* SLATE_HANDLE */
        assert(Slate_GetRune() == rune % 5);
        assert(play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
    }
    /* STICK_ZERO */
    play.pauseCtx.namedItem = EXT_ITEM_SHEIKAH_SLATE;
    Slate_GrantRune(3);
    /* SLATE_HANDLE */
    assert(Slate_GetRune() == 3 && play.pauseCtx.namedItem == PAUSE_ITEM_NONE);
    /* RESET_CHECK */
    puts("PASS native pause wheels: A, both directions, u16 C/Dpad items, slot metadata and all Slate title refreshes");
}
'''
        if host == "mm":
            checks = checks.replace("/* SET_BUTTONS */", r'''
        ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, first);
        C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) = first >= 0x200 ? SLOT_NONE : 89;
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_DOWN) = ITEM_BOW;
        ExtButton_SetDpadItem(0, EQUIP_SLOT_D_RIGHT, first);
        DPAD_SLOT_EQUIP(0, EQUIP_SLOT_D_RIGHT) = 89;
        ExtButton_SetDpadItem(0, EQUIP_SLOT_D_LEFT, EXT_ITEM_SHEIKAH_SLATE);
''').replace("/* CHECK_BUTTONS */", r'''
        assert(ExtButton_GetItem(0, EQUIP_SLOT_C_LEFT) == next);
        assert(C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) == (next >= 0x200 ? SLOT_NONE : 89));
        assert(ExtButton_GetItem(0, EQUIP_SLOT_C_DOWN) == ITEM_BOW);
        assert(ExtButton_GetDpadItem(0, EQUIP_SLOT_D_RIGHT) == next);
        assert(DPAD_SLOT_EQUIP(0, EQUIP_SLOT_D_RIGHT) == 89);
        assert(ExtButton_GetDpadItem(0, EQUIP_SLOT_D_LEFT) == EXT_ITEM_SHEIKAH_SLATE);
        assert(icons[EQUIP_SLOT_C_LEFT] == 1 && icons[EQUIP_SLOT_D_RIGHT + 4] == 1);
''')
            checks = checks.replace("/* STICK_RIGHT */", "play.pauseCtx.stickAdjX = 40;")
            checks = checks.replace("/* STICK_LEFT */", "play.pauseCtx.stickAdjX = -40;")
            checks = checks.replace("/* STICK_ZERO */", "play.pauseCtx.stickAdjX = 0;")
            checks = checks.replace("/* SLATE_HANDLE */", "Slate_KaleidoHandle(&play);")
            checks = checks.replace("/* RESET_CHECK */", r'''
    KaleidoScope_ResetItemCycling();
    assert(gCurrentItemCyclingSlot == -1);
''')
        else:
            checks = checks.replace("/* SET_BUTTONS */", r'''
        sGraceHourglassSelectorActive = 0;
        ExtButton_SetItem(1, first);
        gSaveContext.equips.cButtonSlots[0] = first >= 0x200 ? SLOT_NONE : 41;
        gSaveContext.equips.buttonItems[2] = ITEM_BOW;
        ExtButton_SetItem(4, first);
        gSaveContext.equips.cButtonSlots[3] = first >= 0x200 ? SLOT_NONE : 41;
        ExtButton_SetItem(5, EXT_ITEM_SHEIKAH_SLATE);
''').replace("/* CHECK_BUTTONS */", r'''
        assert(ExtButton_GetItem(1) == next && ExtButton_GetItem(4) == next);
        assert(gSaveContext.equips.cButtonSlots[0] == (next >= 0x200 ? SLOT_NONE : 41));
        assert(gSaveContext.equips.cButtonSlots[3] == (next >= 0x200 ? SLOT_NONE : 41));
        assert(ExtButton_GetItem(2) == ITEM_BOW && ExtButton_GetItem(5) == EXT_ITEM_SHEIKAH_SLATE);
        assert(icons[1] == 1 && icons[4] == 1);
''')
            checks = checks.replace("/* STICK_RIGHT */", "play.pauseCtx.stickRelX = 40;")
            checks = checks.replace("/* STICK_LEFT */", "play.pauseCtx.stickRelX = -40;")
            checks = checks.replace("/* STICK_ZERO */", "play.pauseCtx.stickRelX = 0;")
            checks = checks.replace("/* SLATE_HANDLE */", "Slate_HandleKaleidoSelector(&play);")
            checks = checks.replace("/* RESET_CHECK */", r'''
    play.pauseCtx.cursorItem[PAUSE_ITEM] = ExtInv_GetSlotItem(41);
    play.pauseCtx.cursorSlot[PAUSE_ITEM] = 17;
    play.state.input[0].press.button = BTN_A;
    GraceHourglass_KaleidoHandle(&play);
    assert(gCurrentItemCyclingSlot == 17);
    KaleidoScope_ResetItemCycling();
    play.state.input[0].press.button = 0;
    GraceHourglass_KaleidoHandle(&play);
    assert(gCurrentItemCyclingSlot == -1);
''')
        flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t"]
        includes = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets", host + "/2s2h",
                    "libultraship/include", "combo", "combo/menu"]
        flags += ["-I" + str(ROOT / p) for p in includes]
        for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
            for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
                flags.append(f'-D{key}="{value}"')
        with tempfile.TemporaryDirectory(prefix="nei-wheel-") as td:
            source = Path(td) / "test.cpp"
            source.write_text(prefix + production + checks)
            binary = Path(td) / "test"
            subprocess.run([os.environ.get("CXX", "c++"), *flags, str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
            print("PASS pause host:", host, flush=True)


if __name__ == "__main__":
    main()
