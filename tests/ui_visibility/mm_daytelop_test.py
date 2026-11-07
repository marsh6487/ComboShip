#!/usr/bin/env python3
"""Execute day-transition text, retained white transition, clear and countdown."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT=Path(__file__).resolve().parents[2]
fixture=(ROOT/'tests/ui_visibility/mm_interface_fixture.c').read_text().split('/* PRODUCTION */')[0]
fixture+='#include "overlays/gamestates/ovl_daytelop/z_daytelop.h"\n'
fixture+=r'''
static TexturePtr sDayLeftTextures[4],sDayRightTextures[4],sHoursLeftTextures[4];
static int clears;
u8 gSceneSeqState;
void Play_Init(GameState* state) {}
void func_8012CF0C(GraphicsContext* gfx,s32 clearFb,s32 clearZb,u8 r,u8 g,u8 b) { assert(clearFb && clearZb); ++clears; }
'''
source=(ROOT/'mm/src/overlays/gamestates/ovl_daytelop/z_daytelop.c').read_text()
fixture+='\n'.join(function(source,name) for name in ['DayTelop_Draw','DayTelop_Update','DayTelop_Main'])
fixture+=r'''
int main(void) {
 GraphicsContext gfx={0}; DayTelopState day={0}; day.state.gfxCtx=&gfx;
 gfx.work.p=work; gfx.polyXlu.p=xlu; gfx.overlay.p=overlay; gfx.debug.p=debug;
 enabled=1; gSaveContext.save.day=1; day.alpha=255; day.transitionCountdown=120;
 for (int step=0; step<3; ++step) {
  hidden=step==1; gfx.polyOpa.p=buffer;
  DayTelop_Draw(&day);
  assert(hidden ? gfx.polyOpa.p==buffer : gfx.polyOpa.p>buffer);
  DayTelop_Main(&day.state);
  assert(day.transitionCountdown==119-step && clears==step+1);
 }
 gSaveContext.save.day=9; hidden=0; gfx.polyOpa.p=buffer;
 DayTelop_Draw(&day); int normal=gfx.polyOpa.p-buffer;
 hidden=1; gfx.polyOpa.p=buffer; DayTelop_Draw(&day);
 assert(gfx.polyOpa.p>buffer && gfx.polyOpa.p-buffer<normal);
 puts("PASS MM dawn cards: hidden text, live countdown/clear, retained new-day white transition and restore");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-ui-dawn-') as directory:
    path=Path(directory)/'test.c';path.write_text(fixture);binary=Path(directory)/'test'
    command=['cc','-std=gnu11','-g','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',
             '-DLOG_LEVEL_GAME_PRINTS=0','-Wno-int-conversion','-Wno-incompatible-pointer-types',
             '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
             *['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets','mm',
                                        'mm/2s2h','libultraship/include','combo']],str(path),'-lm','-o',str(binary)]
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print(result.stderr);result.check_returncode()
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
