#!/usr/bin/env python3
"""Execute actual imported sword handlers/dispatch with OoT's real GBI headers.

Resource and matrix services are controlled boundaries; no archive or scene
execution is claimed. Export/cache coverage is in run_mm_dungeon_key_tests.py.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
foreign = (ROOT / 'combo/menu/ComboForeignDrawOOT.h').read_text()
info = foreign[foreign.index('struct ComboForeignDrawInfo {'):foreign.index('\n};',foreign.index('struct ComboForeignDrawInfo {'))+3]
macros = foreign[foreign.index('#define COMBO_FOREIGN_MTX'):foreign.index('// Biggoron')]
handlers = macros + '\n' + '\n'.join(function(foreign,name) for name in [
    'OOT_RestoreForeignSegs','OOT_DrawForeignGoronSword','OOT_DrawForeignWeaponFlame',
    'OOT_DrawForeignMasterSword','OOT_DrawForeignCustomGi','OOT_DrawComboForeign'])
flags=['-std=gnu++20','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DLOG_LEVEL_GAME_PRINTS=0',
       '-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='sword-fallback-') as temporary:
    source=Path(temporary)/'render.cpp'
    source.write_text((ROOT/'tests/sword_fallback/render_test.cpp').read_text().replace(
        '/* PRODUCTION_INFO */',info).replace('/* PRODUCTION_HANDLERS */',handlers))
    includes=['.','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/soh',
              'combo/menu','libultraship/include','libultraship/src']
    binary=Path(temporary)/'render'
    subprocess.run([os.environ.get('CXX','c++'),*flags,*['-I'+str(ROOT/p) for p in includes],str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
