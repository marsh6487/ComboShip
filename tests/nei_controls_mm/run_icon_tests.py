#!/usr/bin/env python3
"""Execute the complete MM icon selector and equipment icon paths (no GPU)."""
import argparse, os, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from run_tests import flags, function
PREFIX=r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "assets/2s2h_assets.h"
#include "mods/equipment/ext_equip_icon_assets.h"
#include "menu/ComboItemIconOwnership.h"
#include <cassert>
#include <cstring>
#include <cstdio>
SaveContext gSaveContext{};
int gComboGameMode=COMBO_GAME_BOTH;
u8 gExtEquipSuppressIconOverride;
TexturePtr gItemIcons[131]{};
static NeiSaveData nei;
extern "C" NeiSaveData* Nei_Save(){return &nei;}
void* DinFireShield_ItemIcon(u16){return nullptr;}
u8 KafeiLandmine_Active(){return 0;}
extern "C" u8 ResourceMgr_FileExists(const char*){return 0;}
s32 TradeAdult_CursorIndex(){return 0;}s32 TradeAdult_NeighborIndex(s32){return 0;}
const char* TradeAdult_IconPath(s32){return nullptr;}
s32 OotMask_CursorIndex(){return 0;}s32 OotMask_NeighborIndex(s32){return 0;}
const char* OotMask_IconPath(s32){return nullptr;}
u8 Nei_CaneOwned(){return 0;}
extern "C" u8 Nei_CaneGetType(){return 0;}
u8 Wand_GetMode(){return 0;}void* Wand_ModeIcon(u8){return nullptr;}
u8 Slate_GetRune(){return 0;}void* Slate_RuneIcon(u8){return nullptr;}
u8 ExtEquip_IsEnabled(){return 0;}u8 ExtEquip_GetCurrent(s16){return 0;}
u8 TransformMasks_IsFDSkinMode(){return 0;}
void* MmAssets_LoadFDSwordIcon(){return nullptr;}
void* MmAssets_LoadResource(const char*){return nullptr;}
u8 WeaponUpgrade_KokiriLevel(){return 0;}u8 WeaponUpgrade_HasGilded(){return 0;}
u8 WeaponUpgrade_HasGreatFairy(){return 0;}u8 WeaponUpgrade_HasHammerAxe(){return 0;}
extern "C" s32 CVarGetInteger(const char*,s32 fallback){return fallback;}
u8 TwilightUpgrade_IsClawshotActive(){return 0;}u8 TwilightUpgrade_IsGaleBoomerangActive(){return 0;}
void* MmAssets_LoadHookshotIcon(){return nullptr;}
u8 PowerKeg_IsOwned(){return 0;}u8 PowerKeg_IsOnBombActive(){return 0;}
const char* MmMasks_GetIconPath(u16){return nullptr;}
const NeiItem* Nei_FindByItem(int32_t){return nullptr;}
u8 Lantern_GetFireType(){return 0;}
const char* MmAssets_GetChateauIconPath(){return nullptr;}
'''
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--source-ref');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true');a=ap.parse_args()
 def read(p):return subprocess.check_output(['git','show',a.source_ref+':'+p],cwd=ROOT,text=True) if a.source_ref else (ROOT/p).read_text()
 inv=read('mm/mods/extended_inventory.c');eq=read('mm/mods/extended_equipment.c')
 first=eq.index('static const char* sExtEquipIconPaths[4][3]');last=eq.index('\n};',first)+3
 code=PREFIX+eq[first:last]+'\n'+function(eq,'ExtEquip_GetIcon')+function(eq,'ExtEquip_GetPendantIcon')
 for n in ('ExtEquip_GetButtonItemId','ExtEquip_IsButtonItem','ExtEquip_GetLegacyButtonItem'):
  if n+'(' in eq:code+='\n'+function(eq,n)
 code+='\n'+function(inv,'ExtInv_GetItemIcon')
 code+=r'''
int main(){
 for(int row=0;row<4;++row)for(int col=1;col<=3;++col){
 // The candidate's assignment API is used when available; the control executes
 // the old actual legacy binding identity, exposing Climb Boots/Pendant overlap.
 u16 item=BUTTON_ID;
 const char* actual=(const char*)ExtInv_GetItemIcon(item);
 const char* expected=(const char*)ExtEquip_GetIcon(row,col);
 assert(actual&&expected&&!strcmp(actual,expected)&&"equipment button icon must match its grid icon path");
 }
 assert(!strcmp((const char*)ExtInv_GetItemIcon(ITEM_EXT_BOOTS_2),
  "__OTR__icon_item_static_yar/gItemIconPendantOfMemoriesTex")&&"legacy inventory Pendant keeps its own icon");
 puts("PASS MM all equipment binding icon paths match grid; inventory Pendant stays distinct");
}
'''.replace('BUTTON_ID','ExtEquip_GetButtonItemId(row,col)' if 'ExtEquip_GetButtonItemId(' in eq else 'ITEM_EXT_SWORD_1+row*3+col-1')
 with tempfile.TemporaryDirectory(prefix='nei-controls-mm-icons-') as td:
  p=Path(td);s=p/'test.cpp';b=p/'test';s.write_text(code)
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if a.sanitize else []
  subprocess.run(['c++','-std=c++20',*flags(),*([] if a.solo else ['-DCOMBO_BUILD']),*san,str(s),'-o',str(b)],check=True)
  subprocess.run([str(b)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
if __name__=='__main__':main()
