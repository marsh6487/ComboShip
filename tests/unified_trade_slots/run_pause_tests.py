#!/usr/bin/env python3
"""Execute the real native pause input block through same-frame normalization/equip."""
import argparse
import os
from pathlib import Path
import runpy
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
base=runpy.run_path(str(ROOT/'tests/oot_pause_inventory/run_tests.py'))
function=base['function']
parser=argparse.ArgumentParser()
parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()
inventory=(ROOT/'soh/mods/extended_inventory.c').read_text()
forms=(ROOT/'soh/mods/transformation_masks/custom_forms.cpp').read_text().replace('extern "C" ','')
ui=(ROOT/'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c').read_text()
scope=(ROOT/'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c').read_text()
trade=(ROOT/'soh/mods/items/logic/trade_items.c').read_text()
prefix=base['PREFIX'].replace('#define IS_RANDO 0','#define IS_RANDO 1')
prefix=prefix.replace('void KaleidoScope_HandleItemCycles(PlayState* play) {}','void KaleidoScope_HandleItemCycles(PlayState* play);')
prefix+='''
void ItemGrantAudit_Begin(const char*,int,int,int) {}
void ItemGrantAudit_End(void) {}
void Nei_SetOwnedItem(uint8_t slot,uint16_t item) { nei.ownedItems[slot-24]=item; }
s32 Flags_GetRandomizerInf(RandomizerInf flag) { return 0; }
bool CanMaskSelect(void) { return false; }
void KaleidoScope_HandleItemCycleExtras(PlayState*,u8,bool,u8,u8,bool) {}
'''
production='\n'.join(function(inventory,n) for n in (
    'TravelTools_IsOwned','MagicMirror_IsOwned','ExtInv_Reset','ExtInv_Update','ExtInv_CanSwitchPage',
    'ExtInv_SwitchPage','ExtInv_GetCurrentPage','ExtInv_GetMaxPages','ExtInv_IsCustomItemsEnabled',
    'ExtInv_IsMmMasksEnabled','ExtInv_GetInventorySlot'))
normalization=trade[trade.index('#define TRADE_ADULT_COUNT'):trade.index('extern void* MmAssets_LoadResource')]
normalization+='\n'+'\n'.join(function(forms,n) for n in ('RitoMask_IsOwned','RitoItem_NoteCellItem','OotMaskWheel_OwnsItem','OotMaskWheel_Neighbor','RitoMask_MigrateLegacy','RitoItem_SyncCell'))
prefix+='void RitoItem_SyncCell(void); s32 TradeAdult_OwnedCount(void); s32 TradeAdult_OwnedAt(s32); u8 TradeAdult_ItemId(s32); u8 TradeAdult_PrevItem(u8);\n'
handler=function(ui,'KaleidoScope_HandleItemCycles')
production+='\n'+handler[:handler.index('    // Bottle Randomizer wheels A/B')]+'}\n'
production+='\n'+function(scope,'KaleidoScope_MoveCursorToSpecialPos')
draw=function(ui,'KaleidoScope_DrawItemSelect')
production+='\nvoid Step(PlayState* play){\n'+draw[draw.index('    Input* input'):draw.index('    OPEN_DISPS')]+draw[draw.index('    // Update extended inventory pagination timer'):draw.index('    gDPSetCombineLERP(OVERLAY_DISP')]+'}\n'
checks=base['CHECKS'][:base['CHECKS'].index('int main(')]+r'''
int main(){
    // A retired Letter must not be equipped from a stale cursor/native cell
    // on the same frame that early pause normalization removes it.
    reset();
    nei.tradeAdultOwned=(1u<<22)|(1u<<2);
    nei.tradeAdultRevision[22]=1;
    gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;
    gSaveContext.inventory.items[SLOT_TRADE_ADULT]=ITEM_LETTER_ZELDA;
    play.pauseCtx.cursorPoint[PAUSE_ITEM]=22;
    play.pauseCtx.cursorSlot[PAUSE_ITEM]=22;
    play.pauseCtx.cursorX[PAUSE_ITEM]=4;
    play.pauseCtx.cursorY[PAUSE_ITEM]=3;
    play.pauseCtx.cursorItem[PAUSE_ITEM]=ITEM_LETTER_ZELDA;
    tick(BTN_CLEFT,0);
    assert(nei.tradeAdultRevision[22]==1);
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]!=ITEM_LETTER_ZELDA);
    assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_COJIRO);
    assert(equipCalls==1 && equippedItem==ITEM_COJIRO && equippedSlot==SLOT_TRADE_ADULT);
    // A completed native/editor write arrives after load; same-frame C press
    // must see the repaired corner instead of equipping a stale Letter there.
    for(int target=22;target<=23;++target) {
        reset();
        gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;
        play.pauseCtx.cursorPoint[PAUSE_ITEM]=target;
        play.pauseCtx.cursorX[PAUSE_ITEM]=target%6;
        play.pauseCtx.cursorY[PAUSE_ITEM]=target/6;
        play.pauseCtx.cursorSlot[PAUSE_ITEM]=target;
        play.pauseCtx.cursorItem[PAUSE_ITEM]=target==23?ITEM_LETTER_ZELDA:PAUSE_ITEM_NONE;
        tick(BTN_CLEFT,0);
        assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE && "pause must normalize before an empty cell skips late cycles");
        assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT]==ITEM_LETTER_ZELDA);
        assert(!(equipCalls && equippedItem==ITEM_LETTER_ZELDA && equippedSlot==SLOT_TRADE_CHILD) && "same-frame equip must not use stale pre-normalization slot");
        if(target==22) assert(equipCalls==1 && equippedItem==ITEM_LETTER_ZELDA && equippedSlot==SLOT_TRADE_ADULT);
    }
    reset();nei.tradeAdultOwned=1u<<22;
    play.pauseCtx.cursorPoint[PAUSE_ITEM]=22;play.pauseCtx.cursorSlot[PAUSE_ITEM]=22;
    play.pauseCtx.cursorX[PAUSE_ITEM]=4;play.pauseCtx.cursorY[PAUSE_ITEM]=3;
    play.pauseCtx.cursorItem[PAUSE_ITEM]=PAUSE_ITEM_NONE;
    tick(BTN_CLEFT,0);
    assert(equipCalls==1 && equippedItem==ITEM_LETTER_ZELDA && equippedSlot==SLOT_TRADE_ADULT && "shared-only Letter must materialize before empty-cursor navigation");
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE);
    reset();
    ExtInv_SwitchPage();
    assert(ExtInv_GetCurrentPage()==1);
    cooldown();
    // Page1 -> page2 -> page0, with a fresh legacy write on the return frame.
    tick(BTN_L,0);assert(ExtInv_GetCurrentPage()==2);cooldown();
    gSaveContext.inventory.items[SLOT_TRADE_CHILD]=ITEM_LETTER_ZELDA;
    play.pauseCtx.cursorPoint[PAUSE_ITEM]=22;play.pauseCtx.cursorSlot[PAUSE_ITEM]=22;
    play.pauseCtx.cursorX[PAUSE_ITEM]=4;play.pauseCtx.cursorY[PAUSE_ITEM]=3;
    tick(BTN_L,0);
    assert(ExtInv_GetCurrentPage()==0);
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD]==ITEM_NONE);
    assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_LETTER_ZELDA);
    puts("PASS native pause: consumed/stale/empty cells and same-frame C equip normalize before lookup");
}
'''
flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCOMBO_BUILD','-DCONTROLLERBUTTONS_T=uint32_t','-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"','-DCVAR_PREFIX_SETTING="gSettings"']
if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
includes=['soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/soh','soh/src/overlays/misc/ovl_kaleido_scope','libultraship/include','combo','combo/menu']
with tempfile.TemporaryDirectory(prefix='unified-trade-pause-') as td:
    path=Path(td);source=path/'test.c';source.write_text(prefix+'\n'+production+'\n'+checks)
    module=path/'normalization.cpp'
    module.write_text('#include "z64.h"\n#include "macros.h"\n#include "mods/extended_inventory.h"\n#include "mods/extended_equipment.h"\n#undef IS_RANDO\n#define IS_RANDO 1\nextern "C" {\nextern SaveContext gSaveContext;\ns32 Flags_GetRandomizerInf(RandomizerInf);\n'+normalization+'\n}\n')
    common=[*flags[2:],*['-I'+str(ROOT/p) for p in includes]]
    subprocess.run([os.environ.get('CC','gcc-14'),'-std=gnu11','-w',*common,'-c',str(source),'-o',str(path/'test.o')],check=True)
    subprocess.run([os.environ.get('CXX','g++-14'),'-std=c++20','-w',*common,'-c',str(module),'-o',str(path/'normalization.o')],check=True)
    subprocess.run([os.environ.get('CXX','g++-14'),*flags,str(path/'test.o'),str(path/'normalization.o'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
