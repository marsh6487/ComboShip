#!/usr/bin/env python3
"""Execute MM's actual button writes, equipment action, wheel and input readers.

Uses real MM save/pause/item headers; only audio, icons and acquired-cell lookup
are fixtures. --source-ref reproduces a regression without modifying checkout.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
def flags():
    return ["-DF3DEX_GBI_2", "-w", "-DCONTROLLERBUTTONS_T=uint32_t", *["-I"+str(ROOT/p) for p in ("mm","mm/include","mm/include/PR","mm/src","mm/assets","mm/2s2h","libultraship/include","combo","combo/menu")]]
from run_mm_scene_randomization_tests import function

PREFIX = r'''
#include "mods/ext_buttons/ext_buttons.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "mods/extended_equipment.h"
#include "mods/extended_inventory.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
SaveContext gSaveContext{};
u8 gItemSlots[77]{};
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save() { return &nei; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return nei.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { nei.ownedItems[slot - 24] = item; }
uint8_t ExtInv_GetOotSlotItem(int) { return ITEM_NONE; }
void ExtInv_SetOotSlotItem(int, uint8_t) {}
#ifdef COMBO_BUILD
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
#endif
extern "C" s32 TradeAdult_OwnedCount() { return 0; }
extern "C" u8 TradeAdult_CellItem() { return ITEM_NONE; }
static int cIcons, dIcons, errors, dpadEnabled = 1;
static bool pacci,riding,cryonis;
extern "C" u8 Pacci_UltrahandModeActive(){return pacci;}
extern "C" u8 MasterCycle_IsRiding(){return riding;}
extern "C" u8 Cryonis_ModeActive(){return cryonis;}
extern "C" u8 ItemInput_ButtonIsClaimed(u16);
extern "C" void Interface_LoadItemIconImpl(PlayState*, u8) { ++cIcons; }
extern "C" void Interface_Dpad_LoadItemIconImpl(PlayState*, u8) { ++dIcons; }
extern "C" void Audio_PlaySfx(u16 id) { errors += id == NA_SE_SY_ERROR; }
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return !strcmp(name,"gEnhancements.Dpad.DpadEquips") ? dpadEnabled : fallback;
}
extern "C" uint32_t GameInteractor_Dpad(GIDpadType, uint32_t buttons) { return dpadEnabled ? buttons : 0; }
static s16 sEquipSubPage=1, sEquipCursorX=1, sEquipCursorY;
u8 gExtEquipGridNameContext;
typedef struct { s16 item, equipType, index; const char* ootIcon; } EquipCell;
static bool owned = true;
static void KaleidoEquip_GetCell(s16, s16 row, s16 col, EquipCell* cell) {
    *cell = {s16(ITEM_EXT_SWORD_1 + row*3 + col-1),row,col,nullptr};
}
static u8 KaleidoEquip_CellOwned(EquipCell*) { return owned; }
static s16 KaleidoEquip_UpgradeNameItem(s16, s16) { return -1; }
static s16 KaleidoEquip_UpgradeValue(s16) { return 0; }
#define EQUIP_SUBPAGE_EXT 1
#define EQUIP_EXT_ROW_BOOTS 3
#define EQUIP_CELL(row,col) ((row)*6+((col)?(col)+2:0))
static s16 gCurrentItemCyclingSlot=-1;
static int page;
int ExtInv_GetInventorySlot(int visual) { return page ? visual+24 : (visual==13 ? SLOT_LENS_OF_TRUTH : visual); }
static void Reset() {
    gSaveContext={}; nei={};
    memset(gSaveContext.save.saveInfo.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
    memset(gSaveContext.save.saveInfo.equips.cButtonSlots,SLOT_NONE,sizeof(gSaveContext.save.saveInfo.equips.cButtonSlots));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems,ITEM_NONE,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots,SLOT_NONE,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots));
    memset(gSaveContext.save.saveInfo.inventory.items,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.inventory.items));
    for(auto& item:nei.ownedItems)item=ITEM_NONE;
    cIcons=dIcons=errors=0;dpadEnabled=1;owned=true;pacci=riding=cryonis=false;page=0;gCurrentItemCyclingSlot=-1;
}
'''

CHECKS = r'''
int main(int argc,char** argv) {
  assert(argc==2); PlayState play{};
  if (!strcmp(argv[1],"independent")) {
    // All 24 custom cells, the full-ID tools, and each equipment piece must
    // assign directly to every D direction while preserving the complete C bank.
    u16 custom[]={ITEM_ROCS_FEATHER_SKIJER,ITEM_ROCS_CAPE,ITEM_DESIRE_SENSOR,ITEM_HYLIAS_GRACE,
      ITEM_ZONAI_PERMAFROST,ITEM_DEMISE_DESTRUCTION,ITEM_DEKU_LEAF,ITEM_SWITCH_HOOK,ITEM_MOGMA_MITTS,
      ITEM_GUST_JAR,ITEM_BALL_AND_CHAIN,ITEM_WHIP,ITEM_SPINNER,ITEM_CANE_OF_SOMARIA,ITEM_DOMINION_ROD,
      ITEM_TIME_GATE,ITEM_BOMB_ARROWS,ITEM_ROD_FIRE,ITEM_ROD_ICE,ITEM_ROD_LIGHT,ITEM_BEETLE,ITEM_SHOVEL,
      ITEM_MINISH_CAP,ITEM_LANTERN,ITEM_ELEMENTAL_WAND,EXT_ITEM_SHEIKAH_SLATE,EXT_ITEM_MAGIC_MIRROR,
      EXT_ITEM_PHANTOM_HOURGLASS,EXT_ITEM_SHADOW_CRYSTAL,EXT_ITEM_ROD_OF_SEASONS};
    for(u16 item:custom) for(int d=0;d<4;++d) {
      Reset(); ExtButton_SetItem(0,EQUIP_SLOT_C_LEFT,item>=0x200?item:0x222);
      if(item<0x200)BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=item;
      C_SLOT_EQUIP(0,EQUIP_SLOT_C_LEFT)=79;
      ExtButton_SetDpadItem(0,d,EXT_ITEM_PHANTOM_HOURGLASS);
      auto before=gSaveContext.save.saveInfo.equips;
      auto extBefore=gSaveContext.save.shipSaveInfo.extButtons;
      assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_D_RIGHT+d,item,79) && "NEI Dpad write must be handled");
      assert(ExtButton_GetDpadItem(0,d)==item && "full NEI identity must survive");
      assert(!memcmp(&before,&gSaveContext.save.saveInfo.equips,sizeof(before)) && "Dpad assignment must not write C bank");
      assert(!memcmp(&extBefore,&gSaveContext.save.shipSaveInfo.extButtons,sizeof(extBefore)));
      assert(cIcons==0 && dIcons==1);
    }
    Reset(); BUTTON_ITEM_EQUIP(0,1)=ITEM_BOW;C_SLOT_EQUIP(0,1)=SLOT_BOW;
    assert(!ExtButton_EquipItem(&play,PAUSE_EQUIP_D_UP,ITEM_BOW,SLOT_BOW) && "native-only swaps retain native owner");
    for(int form=1;form<4;++form)assert(BUTTON_ITEM_EQUIP(form,1)==ITEM_NONE);
  } else if (!strcmp(argv[1],"families")) {
    for(u16 incoming:{u16(ITEM_DINS_FIRE),u16(ITEM_ROCS_FEATHER_SKIJER)}) {
      Reset();u16 other=incoming==ITEM_DINS_FIRE?ITEM_ROCS_FEATHER_SKIJER:ITEM_DINS_FIRE;
      BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=other;C_SLOT_EQUIP(0,EQUIP_SLOT_C_LEFT)=72;
      assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_C_DOWN,incoming,72));
      assert(ExtButton_GetItem(0,EQUIP_SLOT_C_LEFT)==other && "overlapping virtual/custom bands must not identify a different family as this item");
      assert(ExtButton_GetItem(0,EQUIP_SLOT_C_DOWN)==incoming);
    }
    Reset();BUTTON_ITEM_EQUIP(0,1)=ITEM_ROCS_FEATHER_SKIJER;C_SLOT_EQUIP(0,1)=72;
    BUTTON_ITEM_EQUIP(0,2)=ITEM_GUST_JAR;C_SLOT_EQUIP(0,2)=85;
    assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_C_DOWN,ITEM_ROCS_CAPE,72));
    assert(ExtButton_GetItem(0,1)==ITEM_GUST_JAR && ExtButton_GetItem(0,2)==ITEM_ROCS_CAPE && "genuine progressive shared-family C swap remains intact");
    for(u16 item:{u16(ITEM_PICTOGRAPH_BOX),u16(ITEM_POWDER_KEG)})for(int d=0;d<4;++d){
      Reset();u8 slot=item==ITEM_PICTOGRAPH_BOX?SLOT_LENS_OF_TRUTH:SLOT_BOMB;
      BUTTON_ITEM_EQUIP(0,1)=item;C_SLOT_EQUIP(0,1)=slot;
      assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_D_RIGHT+d,item,slot));
      assert(ExtButton_GetItem(0,1)==item && ExtButton_GetDpadItem(0,d)==item);
    }
    for(u16 item:{u16(ITEM_LENS_OF_TRUTH),u16(ITEM_BOMB)}) {
      Reset();assert(!ExtButton_EquipItem(&play,PAUSE_EQUIP_D_UP,item,item==ITEM_BOMB?SLOT_BOMB:SLOT_LENS_OF_TRUTH));
    }
  } else if (!strcmp(argv[1],"equipment")) {
    const u16 buttons[]={BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
    for(int row=0;row<4;++row)for(int col=1;col<=3;++col)for(int b=0;b<7;++b) {
      Reset();sEquipCursorY=row;sEquipCursorX=col;play={};
      play.state.input[0].press.button=buttons[b]; DispatchEquipment(&play);
      u16 item=EQUIPMENT_BINDING_ID;
      assert((b<3?ExtButton_GetItem(0,b+1):ExtButton_GetDpadItem(0,b-3))==item && "all equipment rows accept C and Dpad");
      if(b<3)assert(C_SLOT_EQUIP(0,b+1)==SLOT_NONE);
      else assert(cIcons==0 && dIcons==1);
      DispatchEquipment(&play);
      assert((b<3?ExtButton_GetItem(0,b+1):ExtButton_GetDpadItem(0,b-3))==ITEM_NONE && "same equipment assignment clears requested binding");
      owned=false;DispatchEquipment(&play);assert(errors==1);
    }
    Reset();dpadEnabled=0;sEquipCursorY=0;sEquipCursorX=1;play={};
    play.state.input[0].press.button=BTN_DUP;DispatchEquipment(&play);assert(cIcons==0&&dIcons==0);
    // Empty equipment publishes NONE; every acquired piece publishes its own title identity.
    for(int row=0;row<4;++row)for(int col=1;col<=3;++col)for(int has=0;has<2;++has) {
      sEquipCursorY=row;sEquipCursorX=col;owned=has;Publish(&play.pauseCtx);
      assert(play.pauseCtx.cursorItem[PAUSE_MASK]==(has?ITEM_EXT_SWORD_1+row*3+col-1:PAUSE_ITEM_NONE));
    }
  } else if (!strcmp(argv[1],"wheel_name")) {
    for(int direction:{-40,40}) {
      Reset();play={};play.pauseCtx.cursorSlot[PAUSE_ITEM]=13;
      play.pauseCtx.cursorItem[PAUSE_ITEM]=ITEM_LENS_OF_TRUTH;
      gSaveContext.save.saveInfo.inventory.items[SLOT_LENS_OF_TRUTH]=ITEM_LENS_OF_TRUTH;
      gCurrentItemCyclingSlot=13;play.pauseCtx.stickAdjX=direction;
      KaleidoScope_HandleItemCycleExtras(&play,13,1,ITEM_LENS_OF_TRUTH,ITEM_PICTOGRAPH_BOX,1);
      assert(ExtInv_GetSlotItem(SLOT_LENS_OF_TRUTH)==ITEM_PICTOGRAPH_BOX);
      assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_PICTOGRAPH_BOX && "wheel must publish the current name identity immediately");
    }
  } else if (!strcmp(argv[1],"arrow_alias")) {
    for(u16 item:{u16(ITEM_ROCS_FEATHER_SKIJER),u16(ITEM_ROCS_CAPE)}) {
      Reset();ExtButton_SetItem(0,EQUIP_SLOT_C_LEFT,EXT_ITEM_MAGIC_MIRROR);
      assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_C_LEFT,item,79));
      assert(ExtButton_GetItem(0,EQUIP_SLOT_C_LEFT)==item && "custom feather/cape are not temporary magic arrows");
    }
  } else assert(false);
  printf("PASS MM %s\n",argv[1]);
}
'''

def claim_definition(read=None):
    path='mm/mods/items/helpers/equip_helper.c'
    src=read(path) if read else (ROOT/path).read_text()
    return function(src,'ItemInput_ButtonIsClaimed')+'\n' if 'u8 ItemInput_ButtonIsClaimed(' in src else ''

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--source-ref');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true')
    args=ap.parse_args()
    def read(path):
        return subprocess.check_output(['git','show',args.source_ref+':'+path],cwd=ROOT,text=True) if args.source_ref else (ROOT/path).read_text()
    ui=read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c')
    item=read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c')
    buttons=read('mm/mods/ext_buttons/ext_buttons.cpp')
    code=PREFIX+claim_definition(read)+'\nextern "C" {\n'+buttons[buttons.index('u16 ExtButton_GetItem'):buttons.rindex('} // extern')]+ '\n}\n'
    equipment=read('mm/mods/extended_equipment.c')
    for name in ('ExtEquip_GetButtonItemId','ExtEquip_IsButtonItem','ExtEquip_GetLegacyButtonItem'):
        if name+'(' in equipment:code+=function(equipment,name)+'\n'
    code+=function(ui,'KaleidoEquip_AssignCButton')+'\n'
    first=ui.index('} else if ((sEquipSubPage == EQUIP_SUBPAGE_EXT)')
    last=ui.index('} else if (CHECK_BTN_ALL(input->press.button, BTN_CUP))',first)
    code+='void DispatchEquipment(PlayState* play) { Input* input=&play->state.input[0]; if(false) { '+ui[first:last]+'}}\n'
    first=ui.index('    // --- Publish cursor state');last=ui.index('    // Keep the shared cursor machinery',first)
    code+='void Publish(PauseContext* pauseCtx) { EquipCell cell;\n'+ui[first:last]+'}\n'
    code+=function(item,'KaleidoScope_HandleItemCycleExtras')+'\n'+CHECKS.replace('EQUIPMENT_BINDING_ID','ExtEquip_GetButtonItemId(row,col)' if 'ExtEquip_GetButtonItemId(' in equipment else 'ITEM_EXT_SWORD_1+row*3+col-1')
    with tempfile.TemporaryDirectory(prefix='nei-controls-mm-') as td:
        path=Path(td);source=path/'test.cpp';binary=path/'test';source.write_text(code)
        sanitize=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if args.sanitize else []
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),*([] if args.solo else ['-DCOMBO_BUILD']),*sanitize,str(source),'-o',str(binary)],check=True)
        results=[subprocess.run([str(binary),case],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')).returncode for case in ('independent','equipment','wheel_name','arrow_alias','families')]
        return int(any(results))

if __name__=='__main__':raise SystemExit(main())
