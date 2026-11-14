#!/usr/bin/env python3
"""Read-only extracted-production probe of OoT page-0 trade/mask materialization.

Production files are only read. Generated code and binary stay outside the repo.
No runtime/save/graphics claim: engine input/audio/icon glue is controlled here.
"""
from pathlib import Path
import argparse, os, re, subprocess, tempfile
parser=argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
args=parser.parse_args()

ROOT=Path(__file__).resolve().parents[2]
TEMP=tempfile.TemporaryDirectory(prefix="unified-trade-slots-")
OUT=Path(TEMP.name)
files={
 'trade':'soh/mods/items/logic/trade_items.c',
 'forms':'soh/mods/transformation_masks/custom_forms.cpp',
 'inv':'soh/mods/extended_inventory.h',
 'pause':'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c',
 'equip':'soh/mods/extended_equipment.h',
}
src={k:(ROOT/v).read_text() for k,v in files.items()}
def func(source,name):
    m=re.search(r'^(?:(?:extern "C"|static|inline)\s+)*(?:uint16_t|uint8_t|u8|u16|s32|int|bool|void)\s+'+re.escape(name)+r'\([^;{}]*\)\s*\{',source,re.M)
    assert m, name
    i=m.end(); depth=1
    while depth:
        depth+=(source[i]=='{')-(source[i]=='}'); i+=1
    return source[m.start():i]

prefix=r'''
#include "soh/include/z64item.h"
#include "soh/mods/nei_save.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
extern "C" uint8_t RitoMask_IsOwned(void);
using u8=uint8_t; using u16=uint16_t; using s16=int16_t; using s32=int32_t;
#define osSyncPrintf(...) ((void)0)
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
struct ItemEquips { u8 buttonItems[8]; u8 cButtonSlots[7]; u16 equipment; };
struct SaveContext { struct { u8 items[24]; } inventory; ItemEquips equips,childEquips,adultEquips; };
SaveContext gSaveContext{};
NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save(){return &nei;}
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot){ assert(slot>=24 && slot<72);return nei.ownedItems[slot-24];}
extern "C" void Nei_SetOwnedItem(uint8_t slot,uint16_t item){assert(slot>=24&&slot<72);nei.ownedItems[slot-24]=item;}
unsigned char Picto_IsOwned(){return 0;}
unsigned char PowerKeg_IsOwned(){return 0;}
static unsigned slotWrites=0;
void ItemGrantAudit_Begin(const char*,int item,int slot,int){++slotWrites;}
void ItemGrantAudit_End(){}
static bool randomizer=true;
#define IS_RANDO randomizer
static bool randomizerFlags[RAND_INF_MAX]{};
static bool Flags_GetRandomizerInf(RandomizerInf flag){return randomizerFlags[flag];}
static int currentPage=0;
static int ExtInv_GetCurrentPage(){return currentPage;}
static bool CanMaskSelect(){return false;}
struct Input { struct { u16 button=0; } cur,press; };
struct PauseContext { int cursorSlot[1]={0}; int cursorColorSet=0,stickRelX=0,stickRelY=0; };
struct PlayState { struct { Input input[1]; } state; PauseContext pauseCtx; };
#define PAUSE_ITEM 0
#define CVAR_SETTING(x) x
#define CHECK_BTN_ALL(value,mask) (((value)&(mask))==(mask))
#define CHECK_BTN_ANY(value,mask) (((value)&(mask))!=0)
#define CHECK_AGE_REQ_ITEM(item) true
constexpr u16 BTN_A=0x8000,BTN_CUP=8,BTN_DRIGHT=0x100,BTN_DUP=0x800,BTN_DLEFT=0x200,BTN_DDOWN=0x400;
constexpr int NA_SE_SY_DECIDE=1,NA_SE_SY_CURSOR=2;
static int gCurrentItemCyclingSlot=-1,gSfxDefaultPos=0,gSfxDefaultFreqAndVolScale=0,gSfxDefaultReverb=0;
static int CVarGetInteger(const char*,int fallback){return fallback;}
static void Audio_PlaySfxGeneral(int,int*,int,int*,int*,int*){}
static void Interface_LoadItemIcon1(PlayState*,int){}
'''
forms_names=('RitoMask_IsOwned','RitoItem_NoteCellItem','OotMaskWheel_OwnsItem',
             'OotMaskWheel_Neighbor','RitoMask_MigrateLegacy','RitoItem_SyncCell')
parts=[prefix]
parts.append(re.search(r'^#define ITEM_EXT_BOOTS_2[^\n]*',src['equip'],re.M).group())

parts.extend(func(src['inv'],name) for name in ('ExtInv_GetSlotItem','ExtInv_SetSlotItem'))
trade=src['trade']
parts.append(trade[trade.index('#define TRADE_ADULT_COUNT'):trade.index('extern void* MmAssets_LoadResource')])
parts.extend(func(src['forms'],name) for name in forms_names)
parts.append(func((ROOT/'soh/src/code/z_parameter.c').read_text(), 'Inventory_ReplaceItem'))
parts.append(func(src['pause'],'KaleidoScope_HandleItemCycleExtras'))
# Execute the first TWO complete production page-0 blocks, ending before the
# unrelated bottle/custom-item wheels. Neither block's body is rewritten.
handle=func(src['pause'],'KaleidoScope_HandleItemCycles')
trade_blocks=handle[:handle.index('    // Bottle Randomizer wheels A/B')]
trade_blocks=trade_blocks.replace('KaleidoScope_HandleItemCycles(',
                                 'Probe_ProductionTradeAndMaskPage0(',1)+'}\n'
parts.append(trade_blocks)

parts.append((ROOT/'tests/unified_trade_slots/test.cpp').read_text())
cpp=OUT/'test.cpp'
cpp.write_text('\n\n'.join(parts))
for combined in (True,False):
    flags=['-DCOMBO_BUILD'] if combined else []
    if args.sanitize: flags += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    command=[os.environ.get('CXX','g++-14'),'-std=c++20','-Wall','-Wextra','-Wno-sign-compare','-Wno-parentheses','-Wno-unused-function','-Wno-unused-variable','-Wno-unused-parameter','-Wno-enum-compare','-Wno-extra', *flags, '-I'+str(ROOT),'-I'+str(ROOT/'soh'),str(cpp),'-o',str(OUT/'test')]
    subprocess.run(command,check=True)
    subprocess.run([str(OUT/'test')],check=True)

for combined in (True,False):
    source=(ROOT/'tests/unified_trade_slots/native_test.cpp').read_text()
    production=re.search(r'^#define ITEM_EXT_BOOTS_2[^\n]*',src['equip'],re.M).group()+'\n'+trade[trade.index('#define TRADE_ADULT_COUNT'):trade.index('extern void* MmAssets_LoadResource')]
    production+='\n'+'\n'.join(func(src['forms'],name) for name in forms_names[:-1])
    production+='\n'+'\n'.join(func((ROOT/'soh/src/code/z_parameter.c').read_text(),name) for name in ('Rando_Inventory_SwapAgeEquipment','Inventory_ReplaceItem','Inventory_HatchWeirdEgg','Inventory_HatchPocketCucco'))
    production+='\n'+re.search(r'u8 gItemSlots\[\] = \{.*?\n\};',(ROOT/'soh/src/code/z_inventory.c').read_text(),re.S).group()
    production+='\n'+func((ROOT/'soh/mods/extended_inventory.c').read_text(),'ExtInv_GetItemSlot')
    production+='\n'+func((ROOT/'soh/soh/FleetShipCombo/FleetSync.cpp').read_text(),'FoldNativesIntoRegistry')
    native=OUT/'native.cpp';native.write_text(source.replace('// PRODUCTION',production))
    includes=['soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include']
    flags=['-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DSOH_BUILD_DLL']
    if combined: flags+=['-DCOMBO_BUILD']
    if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    subprocess.run([os.environ.get('CXX','g++-14'),'-std=c++20','-w',*flags,*['-I'+str(ROOT/p) for p in includes],str(native),'-o',str(OUT/'native')],check=True)
    subprocess.run([str(OUT/'native')],check=True)

mm_trade=(ROOT/'mm/mods/items/logic/trade_items.c').read_text()
mm_inv=(ROOT/'mm/mods/extended_inventory.c').read_text()
production=mm_trade[mm_trade.index('#define TRADE_ADULT_COUNT'):]
production+='\n'+re.search(r'^#define OOT_MASK_COUNT[^\n]*',mm_inv,re.M).group()
production+='\n'+'\n'.join(func(mm_inv,name) for name in ('OotMask_IsOwnedIndex','OotMask_OwnedCount','OotMask_OwnedAt','OotMask_CursorIndex','OotMask_CellItem'))
production+='\n'+re.search(r'static const uint8_t sOotPage0Map\[24\] = \{.*?\n\};',mm_inv,re.S).group()
production+='\n'+func(mm_inv,'ExtInv_GetInventorySlot')
mm_source=OUT/'mm.cpp';mm_source.write_text((ROOT/'tests/unified_trade_slots/mm_test.cpp').read_text().replace('// PRODUCTION',production))
flags=['-DCOMBO_BUILD','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DMM_BUILD_DLL']
if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
includes=['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/mods','mm/2s2h','libultraship/include']
subprocess.run([os.environ.get('CXX','g++-14'),'-std=c++20','-w',*flags,*['-I'+str(ROOT/p) for p in includes],str(mm_source),'-o',str(OUT/'mm')],check=True)
subprocess.run([str(OUT/'mm')],check=True)

subprocess.run([os.environ.get('PYTHON','python3'),'-B',str(ROOT/'tests/unified_trade_slots/run_pause_tests.py'),*(['--sanitize'] if args.sanitize else [])],check=True)
