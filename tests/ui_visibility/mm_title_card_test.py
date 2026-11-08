#!/usr/bin/env python3
"""Execute native MM boss/place title cards and their independent aging."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT=Path(__file__).resolve().parents[2]
fixture=(ROOT/'tests/ui_visibility/mm_interface_fixture.c').read_text().split('/* PRODUCTION */')[0]
fixture+='Gfx* Gfx_SetupDL52_NoCD(Gfx* p) { return p+1; }\n'
fixture+=function((ROOT/'mm/src/code/z_lib.c').read_text(),'Math_StepToS')
actor=(ROOT/'mm/src/code/z_actor.c').read_text()
fixture+=function(actor,'TitleCard_Update')+function(actor,'TitleCard_Draw')
fixture+=r'''
int main(void) {
 GraphicsContext gfx={0}; GameState state={0}; TitleCardContext card={0};
 state.gfxCtx=&gfx;
 card.width=32; card.height=16; card.x=160; card.y=120;
 card.alpha=255; card.intensity=255; card.durationTimer=10;
 for (int step=0; step<3; ++step) {
  gfx.work.p=work; gfx.polyOpa.p=buffer; gfx.polyXlu.p=xlu; gfx.overlay.p=overlay; gfx.debug.p=debug;
  enabled=1; hidden=step==1;
  TitleCard_Update(&state,&card);
  TitleCard_Draw(&state,&card);
  assert(card.durationTimer==9-step);
  assert(hidden ? gfx.overlay.p==overlay : gfx.overlay.p>overlay);
 }
 puts("PASS native MM title cards: normal, hidden, restored; duration continues");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-ui-titlecard-') as directory:
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
