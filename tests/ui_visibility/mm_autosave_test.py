#!/usr/bin/env python3
"""Execute the late autosave callback and its native icon draw independently of HUD."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT=Path(__file__).resolve().parents[2]
native=(ROOT/'tests/ui_visibility/mm_interface_fixture.c').read_text().split('/* PRODUCTION */')[0]
native+='#include "misc/title_static/title_static.h"\n'
native+=function((ROOT/'mm/src/code/z_parameter.c').read_text(),'Interface_DrawAutosaveIcon')
native+=r'''
static GraphicsContext autosaveGfx;
static PlayState autosavePlay;
PlayState* gPlayState = &autosavePlay;
void AutosaveTest_SetVisibility(int enable, int hide) {
 enabled=enable; hidden=hide; autosavePlay.state.gfxCtx=&autosaveGfx;
 autosaveGfx.work.p=work; autosaveGfx.polyOpa.p=buffer; autosaveGfx.polyXlu.p=xlu;
 autosaveGfx.overlay.p=overlay; autosaveGfx.debug.p=debug;
}
int AutosaveTest_Submitted(void) { return autosaveGfx.overlay.p>overlay; }
'''
callback=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
struct PlayState;
extern "C" {
 extern PlayState* gPlayState;
 void Interface_DrawAutosaveIcon(PlayState*,uint16_t);
 void AutosaveTest_SetVisibility(int,int);
 int AutosaveTest_Submitted(void);
}
static uint32_t iconTimer=100;
'''
callback+=function((ROOT/'mm/2s2h/Enhancements/Saving/AutoSave.cpp').read_text(),'DrawAutosaveIcon')
callback+=r'''
int main() {
 AutosaveTest_SetVisibility(0,1); DrawAutosaveIcon();
 assert(iconTimer==99 && AutosaveTest_Submitted());
 AutosaveTest_SetVisibility(1,1); DrawAutosaveIcon();
 assert(iconTimer==98 && !AutosaveTest_Submitted());
 AutosaveTest_SetVisibility(1,0); DrawAutosaveIcon();
 assert(iconTimer==97 && AutosaveTest_Submitted());
 puts("PASS late MM autosave: hidden icon, live callback timer and restored submission");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-ui-autosave-') as directory:
    directory=Path(directory); c=directory/'native.c';cpp=directory/'callback.cpp';obj=directory/'native.o';binary=directory/'test'
    c.write_text(native);cpp.write_text(callback)
    flags=['-g','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0',
           '-Wno-int-conversion','-Wno-incompatible-pointer-types','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    command=['cc','-std=gnu11',*flags,*['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets',
             'mm','mm/2s2h','libultraship/include','combo']],'-c',str(c),'-o',str(obj)]
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print(result.stderr);result.check_returncode()
    subprocess.run(['c++','-std=c++20','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
                    str(cpp),str(obj),'-lm','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
