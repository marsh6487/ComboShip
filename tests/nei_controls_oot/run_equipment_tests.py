#!/usr/bin/env python3
"""Run the production extended-equipment pause assignment branch for all C/D inputs."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/oot_equipment_pause"))
from run_access_tests import function

parser = argparse.ArgumentParser()
parser.add_argument("--source-ref")
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
def read(path):
    return subprocess.check_output(["git","show",args.source_ref+":"+path],cwd=ROOT,text=True) if args.source_ref else (ROOT/path).read_text()
ui = read("soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c")
equipment = read("soh/mods/extended_equipment.c")
buttons = read("soh/mods/ext_buttons/ext_buttons.cpp")
player = read("soh/src/overlays/actors/ovl_player_actor/z_player.c")
start = ui.index("                u8 extAgeOk =")
branch = ui[start:ui.index("            } else if (CHECK_AGE_REQ_EQUIP",start)]
start = ui.index("            if (extEquipPage) {\n                // Extended equipment page")
selection = ui[start:ui.index("            } else {",start)] + "            }\n"
prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/extended_player.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "mods/ext_buttons/ext_buttons.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
static int enabled=0, own=1, allow=1;
static u8 current[4]{};
extern "C" u8 ExtEquip_IsEnabled() {return enabled;}
extern "C" u8 ExtEquip_HasItem(s16,u8) {return own;}
extern "C" u8 ExtEquip_CheckAgeReq(s16,u8) {return allow;}
extern "C" u8 TransformMasks_IsTransformedAny() {return 0;}
extern "C" MmPlayerTransformation MmForm_GetCurrentForm() {return MM_PLAYER_FORM_HUMAN;}
extern "C" u8 ExtEquip_GetCurrent(s16 type) {return current[type];}
extern "C" void ExtEquip_SetSlot(s16 type,u8 index) {current[type]=index;}
extern "C" void ExtEquip_Equip(s16 type,u8 index) {if(own && allow) {current[type]=index;enabled=1;}}
extern "C" void Interface_LoadItemIcon2(PlayState*,u16) {}
extern "C" void Audio_PlaySoundGeneral(u16,Vec3f*,u8,f32*,f32*,s8*) {}
Vec3f gSfxDefaultPos{}; f32 gSfxDefaultFreqAndVolScale; s8 gSfxDefaultReverb;
static int sEquipTimer;
static int dpad=1,nativeUses;
static s32 nativeItem;
static Input* sControlInput;
static s32 sZoraBoomerangActive,sDekuBubbleActive,sHeldItemButtonIsHeldDown;
static u16 sItemButtons[]={BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DUP,BTN_DDOWN,BTN_DLEFT,BTN_DRIGHT};
extern "C" int32_t CVarGetInteger(const char*,int32_t) {return dpad;}
extern "C" bool GameInteractor_Should(GIVanillaBehavior,uint32_t result,...) {return result;}
s8 Player_ItemToItemAction(s32 item) {
    if(item>=ITEM_MASK_KEATON && item<=ITEM_MASK_TRUTH)return PLAYER_IA_MASK_KEATON+item-ITEM_MASK_KEATON;
    if(item==ITEM_MM_MASK_KEATON)return PLAYER_IA_MM_MASK_KEATON;
    if(item==ITEM_MM_MASK_BUNNY)return PLAYER_IA_MM_MASK_BUNNY;
    if(item==ITEM_MM_MASK_TRUTH)return PLAYER_IA_MM_MASK_TRUTH;
    return item==ITEM_NONE?PLAYER_IA_NONE:PLAYER_IA_SWORD_MASTER;
}
s32 Player_ItemIsItemAction(s32 item,s32 action) {return Player_ItemToItemAction(item)==action;}
s32 Player_ItemIsInUse(Player* player,s32 item) {return Player_ItemToItemAction(item)==player->heldItemAction;}
extern "C" s32 func_8008F128(Player*) {return 0;}
extern "C" u8 TransformMasks_HandleFormItemUse(PlayState*,Player*,s32) {return 0;}
void Player_UseItem(PlayState*,Player*,s32 item) {++nativeUses;nativeItem=item;}
'''
production = "\n".join(function(buttons,name) for name in ("ExtButton_GetItem","ExtButton_SetItem","ExtButton_ClearItem"))
production += "\n" + function(equipment,"ExtEquip_GetItemId")
for name in ("ExtEquip_GetButtonItemId","ExtEquip_IsButtonItem","ExtEquip_GetLegacyButtonItem"):
    if name+"(" in equipment:
        production += "\n"+function(equipment,name)
production += "\n" + function(equipment,"ExtEquip_ToggleFromCButton")
# `this` is a legal C parameter; rename only that token for the C++ fixture.
production += "\n" + "\n".join(re.sub(r"\bthis\b", "player", function(player,name)) for name in ("Player_GetItemOnButton","Player_ProcessItemButtons"))
production += "\nstatic void Bind(PlayState* play) {auto* pauseCtx=&play->pauseCtx;auto* input=&play->state.input[0];\n"+branch+"\n}\n"
production += "\nstatic u16 Select(PlayState* play) {auto* pauseCtx=&play->pauseCtx;int extEquipPage=1;u16 cursorItem=PAUSE_ITEM_NONE;\n"+selection+"return cursorItem;\n}\n"
checks = r'''
static const u16 masks[8] = {BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DUP,BTN_DDOWN,BTN_DLEFT,BTN_DRIGHT};
int main(int argc,char** argv) {
    PlayState play{};
    if(argc>1 && !strcmp(argv[1],"masks")) {
        Player player{};sControlInput=&play.state.input[0];
        const u16 items[]={ITEM_MASK_KEATON,ITEM_MASK_BUNNY,ITEM_MASK_TRUTH,ITEM_MM_MASK_KEATON,ITEM_MM_MASK_BUNNY,ITEM_MM_MASK_TRUTH};
        const u8 worn[]={PLAYER_MASK_KEATON,PLAYER_MASK_BUNNY,PLAYER_MASK_TRUTH,PLAYER_MASK_KEATON,PLAYER_MASK_BUNNY,PLAYER_MASK_TRUTH};
        for(int item=0;item<6;item++)for(int button=4;button<8;button++) {
            memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
            gSaveContext.equips.buttonItems[button]=items[item];player.currentMask=worn[item];
            Player_ProcessItemButtons(&player,&play);
            assert(player.currentMask==worn[item] && "worn native/MM counterpart mask ignores D-pad binding");
        }
        puts("PASS OoT worn mask retention: native and MM counterparts on all four D-pad slots");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"dispatch")) {
        Player player{};sControlInput=&play.state.input[0];
        for(int row=0;row<4;row++) for(int col=1;col<4;col++) for(int button=1;button<8;button++) {
            memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
            memset(gSaveContext.equips.cButtonSlots,SLOT_NONE,sizeof(gSaveContext.equips.cButtonSlots));
            gSaveContext.ship.extButtons={};current[row]=0;enabled=0;nativeUses=0;
            ExtButton_SetItem(button,0x0240+row*3+col-1);
            play.state.input[0].press.button=masks[button];
            assert(Player_GetItemOnButton(&play,button)==ExtEquip_GetItemId(row,col));
            Player_ProcessItemButtons(&player,&play);
            assert(current[row]==col && nativeUses==0 && "C/D equipment did not reach native dispatcher toggle");
            Player_ProcessItemButtons(&player,&play);
            assert(current[row]==0 && nativeUses==0);
        }
        memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
        gSaveContext.equips.buttonItems[1]=ITEM_EXT_BOOTS_2;gSaveContext.equips.cButtonSlots[0]=SLOT_TRADE_ADULT;
        play.state.input[0].press.button=BTN_CLEFT;nativeUses=0;
        Player_ProcessItemButtons(&player,&play);
        assert(nativeUses==1 && nativeItem==ITEM_EXT_BOOTS_2 && "Pendant was misrouted to Climb Boots");
        puts("PASS OoT native player dispatch: all twelve equipment C/D toggles; Pendant keeps native item path");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"names")) {
        for(int row=0;row<4;row++) for(int col=1;col<4;col++) {
            play.pauseCtx.cursorY[PAUSE_EQUIP]=row;play.pauseCtx.cursorX[PAUSE_EQUIP]=col;
            own=0; assert(Select(&play)==PAUSE_ITEM_NONE && "unowned equipment cell publishes item title");
            own=1; assert(Select(&play)==ExtEquip_GetItemId(row,col));
        }
        puts("PASS OoT equipment titles: every unowned cell empty; owned titles retained");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"toggle")) {
        for(int row=0;row<4;row++) for(int col=1;col<4;col++) {
            enabled=0;current[row]=0;own=1;allow=1;
            ExtEquip_ToggleFromCButton(ExtEquip_GetItemId(row,col));
            assert(current[row]==col && "first equipment binding use must enable equipment");
            ExtEquip_ToggleFromCButton(ExtEquip_GetItemId(row,col));
            assert(current[row]==0 && "second press must remove equipment");
        }
        puts("PASS OoT equipment toggles: enable from inactive, equip and remove all twelve");return 0;
    }
    for(int row=0;row<4;row++) for(int col=1;col<4;col++) for(int button=1;button<8;button++) {
        memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
        memset(gSaveContext.equips.cButtonSlots,SLOT_NONE,sizeof(gSaveContext.equips.cButtonSlots));
        gSaveContext.ship.extButtons={};
        play.pauseCtx.cursorY[PAUSE_EQUIP]=row;play.pauseCtx.cursorX[PAUSE_EQUIP]=col;
        play.state.input[0].press.button=masks[button];
        own=1;allow=1;Bind(&play);
        assert(ExtButton_GetItem(button)==0x0240+row*3+col-1 && "equipment assignment ignored target or lost distinct equipment identity");
        for(int other=1;other<8;other++) if(other!=button) assert(ExtButton_GetItem(other)==ITEM_NONE);
        Bind(&play); assert(ExtButton_GetItem(button)==ITEM_NONE && "repeat assignment must clear only target");
        own=0;Bind(&play);assert(ExtButton_GetItem(button)==ITEM_NONE && "unowned equipment bound");
    }
    puts("PASS OoT equipment assignment: twelve items, seven independent buttons, toggle-off binding, ownership");
}
'''
flags=["-std=c++20","-w","-DF3DEX_GBI_2","-DCONTROLLERBUTTONS_T=uint32_t",'-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"','-DCVAR_PREFIX_SETTING="gSettings"']
if args.sanitize: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie","-g"]
includes=["-I"+str(ROOT/p) for p in ("soh","soh/include","soh/include/PR","soh/src","soh/assets","soh/soh","libultraship/include","combo","combo/menu")]
failed=False
with tempfile.TemporaryDirectory(prefix="oot-equipment-controls-") as td:
    source=Path(td)/"test.cpp";source.write_text(prefix+production+checks)
    for combo in (False,True):
        binary=Path(td)/("combo" if combo else "solo")
        subprocess.run([os.environ.get("CXX","c++"),*flags,*( ["-DCOMBO_BUILD"] if combo else []),*includes,str(source),"-o",str(binary)],check=True)
        for mode in ("assignment","names","toggle","dispatch","masks"):
            result=subprocess.run([str(binary),mode],env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0","UBSAN_OPTIONS":"halt_on_error=1"})
            failed |= result.returncode != 0
sys.exit(int(failed))
