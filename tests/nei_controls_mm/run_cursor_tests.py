#!/usr/bin/env python3
"""Guard against stale empty-cell equip actions with the actual MM cursor body."""
import argparse
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('cursor_fixture',ROOT/'tests/mm_pause_inventory/run_tests.py')
fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)

CHECKS=r'''
int main() {
    const u16 buttons[]={BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
    for(int page=0;page<3;++page)for(int cell=0;cell<24;++cell)for(u16 button:buttons)for(int boundary=0;boundary<2;++boundary) {
        reset();sExtInvState.currentPage=page;
        play.pauseCtx.cursorPoint[PAUSE_ITEM]=cell;
        play.pauseCtx.cursorSlot[PAUSE_ITEM]=cell;
        play.pauseCtx.cursorXIndex[PAUSE_ITEM]=cell%6;
        play.pauseCtx.cursorYIndex[PAUSE_ITEM]=cell/6;
        // Seed exactly the stale state reported: previous nonempty identity with
        // the new cell empty, before pressing either bank's assignment button.
        play.pauseCtx.cursorItem[PAUSE_ITEM]=EXT_ITEM_ROD_OF_SEASONS;
        play.state.input[0].press.button=button;
        play.pauseCtx.stickAdjY=boundary&&cell<6?40:0;
        KaleidoScope_UpdateItemCursor(&play);
        assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==PAUSE_ITEM_NONE);
        assert(play.pauseCtx.mainState==PAUSE_MAIN_STATE_IDLE && "empty cell must not equip the previous cell's identity");
    }
    puts("PASS MM empty-cell stale identity rejected for all 72 cells and seven C/D inputs");
}
'''

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--source-ref');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true');args=ap.parse_args()
    def read(path):
        return subprocess.check_output(['git','show',args.source_ref+':'+path],cwd=ROOT,text=True) if args.source_ref else (ROOT/path).read_text()
    inv=read('mm/mods/extended_inventory.c');ui=read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c')
    first=inv.index('static const uint8_t sOotPage0Map[24]');mapping=inv[first:inv.index('\n};',first)+3]
    prefix=fixture.PREFIX.replace('void ItemGrantAudit_Begin', '#ifdef COMBO_BUILD\nvoid ItemGrantAudit_Begin').replace('void ItemGrantAudit_End(void) {}','void ItemGrantAudit_End(void) {}\n#endif').replace('return fallback;', 'if (!strcmp(name,"gEnhancements.Dpad.DpadEquips")) return 1; return fallback;')
    prefix=prefix.replace('return 0; }\nconst char* PauseItemDesc_Get', 'return buttons; }\nconst char* PauseItemDesc_Get')
    code=prefix+mapping+'\n'+'\n'.join(fixture.function(inv,n) for n in ('TravelTools_IsOwned','MagicMirror_IsOwned','ExtInv_Reset','ExtInv_Update','ExtInv_CanSwitchPage','ExtInv_SwitchPage','ExtInv_GetCurrentPage','ExtInv_GetMaxPages','ExtInv_IsCustomItemsEnabled','ExtInv_IsMmMasksEnabled','ExtInv_GetInventorySlot'))
    # Reuse only setup helpers, not the preexisting fixture's assertions.
    reset=fixture.CHECKS[:fixture.CHECKS.index('int main(')]
    code+='\n'+fixture.function(ui,'KaleidoScope_UpdateItemCursor')+reset+CHECKS
    with tempfile.TemporaryDirectory(prefix='nei-controls-mm-cursor-') as td:
        p=Path(td);source=p/'test.cpp';binary=p/'test';source.write_text(code)
        flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t']
        if not args.solo:flags+=['-DCOMBO_BUILD']
        if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        includes=['-I'+str(ROOT/p) for p in ('mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo','combo/menu')]
        subprocess.run(['c++',*flags,*includes,str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))

if __name__=='__main__':main()
