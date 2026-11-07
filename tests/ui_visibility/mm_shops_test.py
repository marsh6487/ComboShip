#!/usr/bin/env python3
"""Execute all four native shop cursor/prompt draw leaves with real actor types."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT = Path(__file__).resolve().parents[2]
fixture = (ROOT/'tests/ui_visibility/mm_interface_fixture.c').read_text().split('/* PRODUCTION */')[0]
source, test = fixture + '#include "objects/gameplay_keep/gameplay_keep.h"\n', 'int main(void) {\n GraphicsContext gfx = {0}; PlayState play = {0}; play.state.gfxCtx=&gfx;\n'
for actor, prefix in [('En_Fsn','EnFsn'),('En_Ossan','EnOssan'),('En_Sob1','EnSob1'),('En_Trt','EnTrt')]:
    directory = f'mm/src/overlays/actors/ovl_{actor}'
    source += f'#include "overlays/actors/ovl_{actor}/z_{actor.lower()}.h"\n'
    native = (ROOT/directory/f'z_{actor.lower()}.c').read_text()
    source += function(native, prefix+'_DrawCursor') + function(native,prefix+'_DrawTextRec')
    cursor_args = '&actor, &play' if actor == 'En_Fsn' else '&play, &actor'
    test += f''' {{ {prefix} actor = {{0}};
      for (int hide=0; hide<3; ++hide) {{
       enabled=1; hidden=hide==1; gfx.overlay.p=overlay;
       {prefix}_DrawCursor({cursor_args}, 100,100,1,1);
       assert(hidden ? gfx.overlay.p==overlay : gfx.overlay.p>overlay);
       gfx.overlay.p=overlay;
       {prefix}_DrawTextRec(&play,255,255,255,255,100,100,1,0,0,1,1);
       assert(hidden ? gfx.overlay.p==overlay : gfx.overlay.p>overlay);
      }}
    }}\n'''
test += 'puts("PASS native MM shop cursors/prompts: all four shops normal, hidden, restored"); }\n'
with tempfile.TemporaryDirectory(prefix='mm-ui-shops-') as directory:
    path = Path(directory)/'test.c'
    path.write_text(source+test)
    binary = Path(directory)/'test'
    command = ['cc','-std=gnu11','-g','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',
               '-DLOG_LEVEL_GAME_PRINTS=0','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
               *['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets','mm',
                                          'mm/2s2h','libultraship/include','combo']],str(path),'-lm','-o',str(binary)]
    result = subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print(result.stderr)
        result.check_returncode()
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
