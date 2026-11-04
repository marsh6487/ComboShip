#!/usr/bin/env python3
"""Run the production MM outline-vertex loops against native engine/save types."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from run_tests import PREFIX, ROOT, function

PREFIX = PREFIX.replace('void ItemGrantAudit_Begin(const char* origin, int item, int slot, int immediate) {}', '')
PREFIX = PREFIX.replace('void ItemGrantAudit_End(void) {}', '')
inv = (ROOT / 'mm/mods/extended_inventory.c').read_text()
scope = (ROOT / 'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope_NES.c').read_text()
start = inv.index('static const uint8_t sOotPage0Map[24]')
mapping = inv[start:inv.index('\n};', start) + 3]
start = scope.index('        // QUAD_ITEM_GRID_SELECTED_C_LEFT,')
end = scope.index('\n    if (pauseCtx->pageIndex != PAUSE_MASK)', start)
loops = scope[start:end].rsplit('\n    }', 1)[0]
code = PREFIX + mapping + '\n' + function(inv, 'ExtInv_GetInventorySlot')
if 'int ExtInv_GetItemOutlineVisualSlot(' in inv:
    code += '\n' + function(inv, 'ExtInv_GetItemOutlineVisualSlot')
code += '\nstatic void BuildOutlines(PauseContext* pauseCtx) { s16 i=24*4,j,k;\n' + loops + '\n}\n'
bomb = (ROOT / 'mm/2s2h/Enhancements/Equipment/BombArrows.cpp').read_text()
code += '#include <stdarg.h>\nPlayState* gPlayState;\nstatic bool sBombSlotIsBombArrowMode=true;\nstatic bool IsBombArrowButton(s32 btn,s32 dpad) {return true;}\n'
code += function(bomb, 'OnDrawItemEquippedOutline')
code += '\nstatic void Hook(bool* should, ...) {va_list args;va_start(args,should);OnDrawItemEquippedOutline(should,args);va_end(args); }\n'
code += r'''
static PlayState play;
static Vtx vertices[31*4];
static void seed(int page) {
    memset(&gSaveContext,0,sizeof(gSaveContext));
    memset(&play,0,sizeof(play));
    memset(&gSaveContext.save.saveInfo.equips,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.equips));
    memset(&gSaveContext.save.shipSaveInfo.dpadEquips,ITEM_NONE,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips));
    gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    sExtInvState.currentPage=page;
    play.pauseCtx.itemVtx=vertices;
    play.pauseCtx.alpha=255;
    memset(vertices,0,sizeof(vertices));
    for(int v=0;v<24;v++) {
        vertices[v*4].v.ob[0]=-94+(v%6)*32;
        vertices[v*4].v.ob[1]=56-(v/6)*32;
    }
}
static void check(int index,int visual) {
    if(visual<0) {assert(vertices[index*4].v.ob[0]==-300);return;}
    assert(vertices[index*4].v.ob[0]==vertices[visual*4].v.ob[0]+ITEM_GRID_SELECTED_QUAD_MARGIN);
    assert(vertices[index*4].v.ob[1]==vertices[visual*4].v.ob[1]-ITEM_GRID_SELECTED_QUAD_MARGIN);
}
int main(void) {
    seed(0);
    C_SLOT_EQUIP(0,EQUIP_SLOT_C_LEFT)=SLOT_DEKU_STICK;
    C_SLOT_EQUIP(0,EQUIP_SLOT_C_RIGHT)=SLOT_DEKU_NUT;
    DPAD_SLOT_EQUIP(0,EQUIP_SLOT_D_RIGHT)=SLOT_OCARINA;
    SaveContext before=gSaveContext;
    BuildOutlines(&play.pauseCtx);
    check(24,0);check(26,1);check(27,7);
    assert(!memcmp(&before,&gSaveContext,sizeof(before)));
    for(int page=0;page<3;page++)for(int slot=0;slot<256;slot++) {
        seed(page);
        int visual=-1;
        if(page==0 && slot<24)for(int v=0;v<24;v++)if(sOotPage0Map[v]==slot){visual=v;break;}
        C_SLOT_EQUIP(0,EQUIP_SLOT_C_LEFT)=slot;
        DPAD_SLOT_EQUIP(0,EQUIP_SLOT_D_RIGHT)=slot;
        before=gSaveContext;
        BuildOutlines(&play.pauseCtx);
        check(24,visual);check(27,visual);
        assert(!memcmp(&before,&gSaveContext,sizeof(before)));
    }
    for(int page=0;page<3;page++)for(int dpad=0;dpad<2;dpad++) {
        seed(page);gPlayState=&play;
        C_SLOT_EQUIP(0,EQUIP_SLOT_C_LEFT)=SLOT_BOW;
        DPAD_SLOT_EQUIP(0,EQUIP_SLOT_D_RIGHT)=SLOT_BOW;
        BuildOutlines(&play.pauseCtx);
        ItemId item=ITEM_BOW;
        bool should=page==0;
        before=gSaveContext;
        Hook(&should,&item,dpad?EQUIP_SLOT_D_RIGHT:EQUIP_SLOT_C_LEFT,dpad,PAUSE_ITEM);
        assert(should==(page==0));
        check(dpad?27:24,page==0?2:-1);
        assert(!memcmp(&before,&gSaveContext,sizeof(before)));
    }
    puts("PASS native MM C/D-pad outlines: remapped cells, all slot bytes, hidden pages and unchanged saves");
    puts("PASS bomb-arrow hook: C/D-pad bomb-cell mapping and hidden-page suppression");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-assignment-outlines-') as tmp:
    folder=Path(tmp); source=folder/'test.c';binary=folder/'test';source.write_text(code)
    flags=['-std=gnu11','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-ffunction-sections','-fdata-sections',
           '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie','-g']
    includes=['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo','combo/menu']
    for combined in (False, True):
        subprocess.run([os.environ.get('CC','cc'),*flags,*(['-DCOMBO_BUILD'] if combined else []),
                        *['-I'+str(ROOT/p) for p in includes],str(source),'-Wl,--gc-sections','-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
