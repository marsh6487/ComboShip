#!/usr/bin/env python3
"""Execute OoT's production pause equip completion and NEI input boundaries."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/oot_pause_inventory"))
from run_tests import function


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-ref")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    def read(path):
        return subprocess.check_output(["git", "show", args.source_ref + ":" + path], cwd=ROOT, text=True) if args.source_ref else (ROOT / path).read_text()
    buttons = read("soh/mods/ext_buttons/ext_buttons.cpp")
    ui = read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
    registry = read("soh/mods/extended_player.c")
    registry = registry[registry.index("static const NeiItem sNeiItems[]"):registry.index("#define NEI_ITEMS_COUNT")]
    items = list(dict.fromkeys(re.findall(r"^\s*\{\s*((?:EXT_)?ITEM_\w+)\s*,", registry, re.M)))
    items += ["EXT_ITEM_SHEIKAH_SLATE", "EXT_ITEM_PHANTOM_HOURGLASS", "EXT_ITEM_SHADOW_CRYSTAL", "EXT_ITEM_ROD_OF_SEASONS",
              "ITEM_MM_REMAINS_ODOLWA", "ITEM_MM_REMAINS_GOHT", "ITEM_MM_REMAINS_GYORG", "ITEM_MM_REMAINS_TWINMOLD", "ITEM_RITO_MASK",
              "ITEM_MM_LETTER_KAFEI", "ITEM_MM_SPECIAL_DELIVERY"]
    finish = ui[ui.index("            uint16_t targetButtonIndex ="):ui.index("            pauseCtx->unk_1E4 = 0;", ui.index("            uint16_t targetButtonIndex ="))]
    prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
SaveContext gSaveContext{};
#define HOURGLASS_BUTTON_ITEM(btn) ExtButton_GetItem(btn)
#define HOURGLASS_LAST_BUTTON 7
static const u16 sHourglassButtonMasks[8]={BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DUP,BTN_DDOWN,BTN_DLEFT,BTN_DRIGHT};
static int dpad = 1, claimed = 0;
static u8 pictoOwned, pictoMode, kegOwned, kegMode;
static int crystalUses;
extern "C" void MmForm_HandleMaskUse(PlayState*,Player*,s32 item) {assert(item==EXT_ITEM_SHADOW_CRYSTAL);++crystalUses;}
extern "C" u8 Picto_IsOwned() {return pictoOwned;}
extern "C" u8 Picto_IsOnLensActive() {return pictoMode;}
extern "C" u8 PowerKeg_IsOwned() {return kegOwned;}
extern "C" u8 PowerKeg_IsOnBombActive() {return kegMode;}
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return !strcmp(key,"gEnhancements.DpadEquips") ? dpad : fallback;
}
extern "C" u8 ItemInput_ButtonIsClaimed(u16 mask) { return claimed && (mask & (BTN_DUP|BTN_DDOWN|BTN_DLEFT|BTN_DRIGHT)); }
extern "C" void Interface_LoadItemIcon1(PlayState*, u16) {}
extern "C" void Interface_LoadItemIcon2(PlayState*, u16) {}
static void reset() {
    gSaveContext = {};
    memset(gSaveContext.equips.buttonItems, ITEM_NONE, sizeof(gSaveContext.equips.buttonItems));
    memset(gSaveContext.equips.cButtonSlots, SLOT_NONE, sizeof(gSaveContext.equips.cButtonSlots));
    dpad=1; claimed=0;pictoOwned=pictoMode=kegOwned=kegMode=0;crystalUses=0;
}
'''
    names = ["ExtButton_GetItem", "ExtButton_SetItem", "ExtButton_ClearItem"]
    for name in ("ExtButton_GetInputMask", "ExtButton_WriteMirrorEquip", "ExtButton_WriteEquip", "ExtButton_EquipItem", "ExtButton_EquipMagicMirror"):
        if re.search(r"\b" + name + r"\([^;{}]*\)\s*\{", buttons):
            names.append(name)
    production = "\n".join(function(buttons, name) for name in names)
    production += "\nstatic void Assign(PlayState* play, int button, u16 item, u16 slot) {\nPauseContext* pauseCtx=&play->pauseCtx;\npauseCtx->equipTargetCBtn=button-1; pauseCtx->equipTargetItem=item; pauseCtx->equipTargetSlot=slot;\n" + finish + "\n}\n"
    for path, name in (("item_rod_of_seasons.c", "Seasons_EquippedButtonMask"), ("item_sheikah_slate.c", "Slate_EquippedButtonMask")):
        production += "\n" + function(read("soh/mods/items/logic/" + path), name)
    production += "\n" + function(read("soh/mods/items/logic/item_phantom_hourglass.c"), "Hourglass_EquippedButtonMask")
    production += "\n" + function(read("soh/mods/items/logic/item_shadow_crystal.c"), "ShadowCrystal_TickInput")
    checks = r'''
static const u16 masks[8] = {BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DUP,BTN_DDOWN,BTN_DLEFT,BTN_DRIGHT};
int main(int argc, char** argv) {
    PlayState play{};
    const bool input = argc>1 && !strcmp(argv[1],"input");
    if (input) {
        for (int button=1;button<8;button++) {
            reset(); ExtButton_SetItem(button,EXT_ITEM_ROD_OF_SEASONS);
            assert(Seasons_EquippedButtonMask()==masks[button] && "Rod ignores assigned D-pad button");
            reset(); ExtButton_SetItem(button,EXT_ITEM_SHEIKAH_SLATE);
            assert(Slate_EquippedButtonMask()==masks[button] && "Slate ignores assigned D-pad button");
            if (button>=4) {
                dpad=0; assert(Slate_EquippedButtonMask()==0);
                dpad=1; claimed=1; assert(Slate_EquippedButtonMask()==0);
            }
            reset();ExtButton_SetItem(button,EXT_ITEM_PHANTOM_HOURGLASS);
            assert(Hourglass_EquippedButtonMask()==masks[button]);
            const int status=button<4?button:button+1;
            gSaveContext.buttonStatus[status]=BTN_DISABLED;
            assert(Hourglass_EquippedButtonMask()==0 && "disabled button leaked full-ID item input");
            reset();ExtButton_SetItem(button,EXT_ITEM_SHADOW_CRYSTAL);
            play.state.input[0].press.button=masks[button];
            ShadowCrystal_TickInput(&play,nullptr);assert(crystalUses==1);
            if(button>=4) {
                dpad=0;ShadowCrystal_TickInput(&play,nullptr);assert(crystalUses==1);
                dpad=1;claimed=1;ShadowCrystal_TickInput(&play,nullptr);assert(crystalUses==1);
            }
        }
        puts("PASS OoT runtime masks: C/D-pad Rod, Slate, Hourglass and Crystal; disabled/claimed controls");
        return 0;
    }
    if (argc>1 && !strcmp(argv[1],"shared")) {
        for (u16 item: {u16(ITEM_LENS),u16(ITEM_BOMB)}) {
            reset();
            pictoOwned=pictoMode=kegOwned=kegMode=1;
            const u8 slot=item==ITEM_LENS?SLOT_LENS:SLOT_BOMB;
            Assign(&play,1,item,slot);Assign(&play,4,item,slot);
            assert(ExtButton_GetItem(1)==item && ExtButton_GetItem(4)==item && "active NEI shared-cell assignment moved C binding");
            pictoMode=kegMode=0;
            assert(ExtButton_GetItem(1)==item && ExtButton_GetItem(4)==item && "shared mode change lost independent bindings");
            reset();Assign(&play,1,item,slot);Assign(&play,4,item,slot);
            assert(ExtButton_GetItem(1)==ITEM_NONE && ExtButton_GetItem(4)==item && "true native Lens/Bomb lost swap semantics");
        }
        puts("PASS OoT shared cells: active Pictograph/Power Keg independent bindings; native Lens/Bomb swaps");return 0;
    }
    if (argc>1 && !strcmp(argv[1],"relocation")) {
        for(u16 item:{u16(ITEM_DEKU_LEAF),u16(ITEM_WHIP),u16(EXT_ITEM_ROD_OF_SEASONS),u16(EXT_ITEM_MAGIC_MIRROR)})
        for(int d=4;d<8;++d)for(int c=1;c<4;++c)for(bool occupied:{false,true}) {
            reset();
            if(occupied) Assign(&play,c,EXT_ITEM_PHANTOM_HOURGLASS,47);
            Assign(&play,d,item,31);
            gSaveContext.equips.buttonItems[0]=ITEM_SWORD_MASTER;
            Assign(&play,c,item,31);
            assert(ExtButton_GetItem(c)==item);
            assert(ExtButton_GetItem(d)==(occupied?u16(EXT_ITEM_PHANTOM_HOURGLASS):u16(ITEM_NONE)) &&
                   "moving an NEI to C must remove its old Dpad binding and preserve the displaced item once");
            assert(ExtButton_GetItem(0)==ITEM_SWORD_MASTER);
            if(!occupied)assert(EXT_BUTTON_ITEM(d)==0 && gSaveContext.equips.cButtonSlots[d-1]==SLOT_NONE);
            for(int other=1;other<8;++other)if(other!=c && other!=d)assert(ExtButton_GetItem(other)==ITEM_NONE);
        }
        for(u16 item:{u16(ITEM_DEKU_LEAF),u16(EXT_ITEM_SHEIKAH_SLATE)})for(int target=1;target<4;++target) {
            reset();
            for(int source=1;source<8;++source) {
                if(source==target)continue;
                if(item>=0x200) ExtButton_SetItem(source,item);
                else gSaveContext.equips.buttonItems[source]=item;
                gSaveContext.equips.cButtonSlots[source-1]=item>=0x200?SLOT_NONE:31;
            }
            Assign(&play,target,EXT_ITEM_PHANTOM_HOURGLASS,47);
            Assign(&play,target,item,31);
            int count=0,displaced=0;
            for(int button=1;button<8;++button) {
                count+=ExtButton_GetItem(button)==item;
                displaced+=ExtButton_GetItem(button)==EXT_ITEM_PHANTOM_HOURGLASS;
            }
            assert(count==1 && displaced==1 && "legacy duplicates must not duplicate the displaced item");
            Assign(&play,target,item,31);
            assert(ExtButton_GetItem(target)==item);
        }
        // Two different items may share a custom cell number; only the moved identity follows.
        reset();Assign(&play,1,ITEM_ROCS_FEATHER_SKIJER,31);Assign(&play,4,ITEM_DEKU_LEAF,31);
        Assign(&play,2,ITEM_DEKU_LEAF,31);
        assert(ExtButton_GetItem(1)==ITEM_ROCS_FEATHER_SKIJER && ExtButton_GetItem(4)==ITEM_NONE);
        puts("PASS OoT NEI Dpad-to-C relocation: all directions, occupied targets, full IDs and duplicate cleanup");
        return 0;
    }
    for (u16 item: {/* REGISTRY_ITEMS */}) {
        for (int button=4;button<8;button++) {
            reset();
            Assign(&play,1,item,31);
            const auto equips = gSaveContext.equips;
            const auto ext = gSaveContext.ship.extButtons;
            Assign(&play,button,item,31);
            for (int c=1;c<4;c++) {
                assert(gSaveContext.equips.buttonItems[c]==equips.buttonItems[c] && "D-pad assignment wrote a C slot");
                assert(EXT_BUTTON_ITEM(c)==ext.items[c] && "D-pad assignment changed C full id");
                assert(gSaveContext.equips.cButtonSlots[c-1]==equips.cButtonSlots[c-1]);
            }
            assert(ExtButton_GetItem(button)==item);
        }
    }
    reset(); Assign(&play,1,ITEM_BOW,SLOT_BOW); Assign(&play,4,ITEM_BOMB,SLOT_BOMB);
    Assign(&play,4,ITEM_BOW,SLOT_BOW);
    assert(ExtButton_GetItem(1)==ITEM_BOMB && ExtButton_GetItem(4)==ITEM_BOW && "native swap changed");
    reset(); Assign(&play,1,ITEM_BOW,SLOT_BOW); Assign(&play,4,EXT_ITEM_ROD_OF_SEASONS,47);
    Assign(&play,4,ITEM_BOW,SLOT_BOW);
    assert(ExtButton_GetItem(1)==EXT_ITEM_ROD_OF_SEASONS && ExtButton_GetItem(4)==ITEM_BOW && "native swap lost displaced full id");
    reset();Assign(&play,1,ITEM_ROCS_FEATHER_SKIJER,24);
    Assign(&play,4,ITEM_TUNIC_KOKIRI,SLOT_TUNIC_KOKIRI);
    assert(ExtButton_GetItem(1)==ITEM_ROCS_FEATHER_SKIJER && "native equipment virtual slot collided with NEI inventory cell");
    reset();
    ExtButton_SetItem(1,EXT_ITEM_SHEIKAH_SLATE);
    const auto equips=gSaveContext.equips; const auto ext=gSaveContext.ship.extButtons;
    Assign(&play,4,ITEM_NONE,SLOT_NONE); Assign(&play,4,PAUSE_ITEM_NONE,SLOT_NONE);
    assert(!memcmp(&equips,&gSaveContext.equips,sizeof(equips)) && !memcmp(&ext,&gSaveContext.ship.extButtons,sizeof(ext)));
    puts("PASS OoT assignment: independent NEI C/D-pad bindings, full ids and native swaps");
}
'''
    checks = checks.replace("/* REGISTRY_ITEMS */", ",".join("u16("+item+")" for item in items))
    flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t"]
    flags += ['-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"']
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-g"]
    includes = ["-I" + str(ROOT / p) for p in ("soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh", "libultraship/include", "combo", "combo/menu")]
    with tempfile.TemporaryDirectory(prefix="nei-controls-oot-") as td:
        source = Path(td)/"test.cpp"; source.write_text(prefix+production+checks)
        failed=False
        for combo in (False,True):
            binary=Path(td)/("combo" if combo else "solo")
            subprocess.run([os.environ.get("CXX","c++"),*flags,*( ["-DCOMBO_BUILD"] if combo else []),*includes,str(source),"-o",str(binary)],check=True)
            for mode in ("assignment","input","shared","relocation"):
                result=subprocess.run([str(binary),mode],env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0","UBSAN_OPTIONS":"halt_on_error=1"})
                failed |= result.returncode != 0
        return int(failed)

if __name__ == "__main__":
    sys.exit(main())
