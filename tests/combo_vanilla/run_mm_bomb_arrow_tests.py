#!/usr/bin/env python3
"""Exercise earned bomb element through MM's real effective-element and pause selector paths."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
inventory=(ROOT/'mm/mods/extended_inventory.c').read_text()
kaleido=(ROOT/'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c').read_text()


def block(source,signature,offset=0):
    start=source.index(signature,offset);brace=source.index('{',start);end=brace+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


code=r'''
#include "global.h"
#include "mods/items/logic/twilight_upgrade.h"
#include <cassert>
#include <string_view>
#include <iostream>
SaveContext gSaveContext{};
static bool vanilla=true;
static bool Combo_IsVanilla() { return vanilla; }
static int enabled,bombMode,reads,icons,draws;
extern "C" NeiSaveData* Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
extern "C" s32 CVarGetInteger(const char* key,s32 fallback) {
    ++reads;
    if(std::string_view(key)=="gEnhancements.SkijerNEI.SW97Medallions") return enabled;
    if(std::string_view(key)=="gMods.BombArrows.Mode") return bombMode;
    return fallback;
}
#define SW97_ENABLED() CVarGetInteger("gEnhancements.SkijerNEI.SW97Medallions",0)
'''
for name in ('sSw97ElemQuest','sSw97ElemVanillaSlot'):
    code+=re.search(r'static const uint8_t '+name+r'\[SW97_ELEM_COUNT\] = \{.*?\n\};',inventory,re.S)[0]+'\n'
native_inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
for name in ('gUpgradeMasks','gUpgradeShifts'):
    code+=re.search(r'u(?:8|32) '+name+r'\[\] = \{.*?\n\};',native_inventory,re.S)[0]+'\n'
twilight=(ROOT/'mm/mods/items/logic/twilight_upgrade.c').read_text()
code+=block(twilight,'u8 TwilightUpgrade_HasBombArrows')+'\n'
for signature in ('uint8_t BombArrows_RandoMode','uint8_t Sw97_BombArrowsOwned',
                  'uint8_t Sw97_ElementWheelEnabled','uint8_t Sw97_ElementOwned',
                  'static uint8_t Sw97_ElementAllowed','uint8_t Sw97_ElementCount','uint8_t Sw97_ElementAt',
                  'uint8_t Sw97_GetElement','void Sw97_SetElement','uint8_t Sw97_ElementNeighbor',
                  'uint8_t Sw97_EffectiveElement'):
    if signature in inventory:
        code+=block(inventory,signature)+'\n'
code+=r'''
static s16 gCurrentItemCyclingSlot=-1;
#define SW97_BOW_CELL 3
#define SW97_SLING_CELL 6
static int ExtInv_GetInventorySlot(int cell) { return cell==3?SLOT_BOW:77; }
static u16 ExtInv_GetSlotItem(int slot) { return slot<24?gSaveContext.save.saveInfo.inventory.items[slot]:ITEM_NONE; }
extern "C" void Audio_PlaySfx(u16) {}
static void Sw97_RefreshButtonIcons(PlayState*) { ++icons; }
static void Sw97Wheel_Draw(PlayState*,s32,u8) { ++draws; }
'''
code+=block(kaleido,'static void Sw97Wheel_Handle')+'\n'
for call,wrapper in (('        Sw97Wheel_Handle','NativeHandle'),('        Sw97Wheel_Draw','NativeDraw')):
    start=kaleido.rfind('    if (',0,kaleido.index(call))
    code+='static void '+wrapper+'(PlayState* play) {\n'+block(kaleido,kaleido[start:kaleido.index('{',start)],start)+'\n}\n'
code+=r'''
int main() {
    PlayState play{};
    gSaveContext.save.saveInfo.inventory.items[SLOT_BOW]=ITEM_BOW;
    // All foreign medallions and native arrows present must not turn on their
    // optional element modes when the user's SW97 preference is disabled.
    Nei_Save()->ootQuestItems=0xFFFFFFFF;
    gSaveContext.save.saveInfo.inventory.items[SLOT_ARROW_FIRE]=ITEM_ARROW_FIRE;
    gSaveContext.save.saveInfo.inventory.items[SLOT_ARROW_ICE]=ITEM_ARROW_ICE;
    gSaveContext.save.saveInfo.inventory.items[SLOT_ARROW_LIGHT]=ITEM_ARROW_LIGHT;
    play.pauseCtx.cursorSlot[PAUSE_ITEM]=3;
    play.state.input[0].press.button=BTN_A;
    enabled=0;
    assert(Sw97_EffectiveElement(0)==SW97_ELEM_NONE);
    NativeHandle(&play); NativeDraw(&play);
    assert(gCurrentItemCyclingSlot==-1 && draws==0);
    for(bool twilight : {false,true}) {
        Nei_Save()->bombArrowsOwned=!twilight;
        Nei_Save()->twilightUpgrade=twilight?TWILIGHT_UPGRADE_ALL:0;
        Nei_Save()->sw97BowElement=SW97_ELEM_NONE;
        assert(Sw97_ElementCount(0)==2);
        assert(Sw97_ElementAt(0,0)==SW97_ELEM_NONE && Sw97_ElementAt(0,1)==SW97_ELEM_BOMB);
        NativeHandle(&play);
        assert(gCurrentItemCyclingSlot==3);
        play.state.input[0].press.button=0;
        play.pauseCtx.stickAdjX=40;
        NativeHandle(&play);
        assert(Sw97_EffectiveElement(0)==SW97_ELEM_BOMB && icons>0);
        NativeDraw(&play); assert(draws>0);
        Sw97_SetElement(0,SW97_ELEM_FIRE);
        assert(Sw97_EffectiveElement(0)==SW97_ELEM_BOMB);
        enabled=1;
        Sw97_SetElement(0,SW97_ELEM_FIRE);
        assert(Sw97_EffectiveElement(0)==SW97_ELEM_FIRE);
        enabled=0;
        assert(Sw97_EffectiveElement(0)==SW97_ELEM_NONE);
        gCurrentItemCyclingSlot=-1;
        play.state.input[0].press.button=BTN_A;
        play.pauseCtx.stickAdjX=0;
    }
    // Native weapon ownership remains a prerequisite for its pause selector.
    gSaveContext.save.saveInfo.inventory.items[SLOT_BOW]=ITEM_NONE;
    NativeHandle(&play); assert(gCurrentItemCyclingSlot==-1);
    vanilla=false; enabled=0;
    Nei_Save()->bombArrowsOwned=1; Nei_Save()->sw97BowElement=SW97_ELEM_BOMB;
    assert(Sw97_EffectiveElement(0)==SW97_ELEM_NONE);
    enabled=1; assert(Sw97_EffectiveElement(0)==SW97_ELEM_BOMB);
    std::cout<<"PASS MM earned Bomb Arrows/Twilight effective shots and native pause selector with default SW97 off; other modes and seeded/solo controls\n";
}
'''
with tempfile.TemporaryDirectory(prefix='vanilla-mm-bomb-arrow-') as tmp:
    cpp=Path(tmp)/'test.cpp';cpp.write_text(code)
    flags=['-std=c++20','-w','-DF3DEX_GBI_2']
    for path in ('mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
        flags+=['-I'+str(ROOT/path)]
    if '--sanitize' in sys.argv:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    binary=Path(tmp)/'test'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
