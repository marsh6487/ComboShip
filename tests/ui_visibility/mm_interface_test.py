#!/usr/bin/env python3
"""Execute MM's complete Interface_Draw and native Minimal UI predicates."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'mm/src/code/z_parameter.c').read_text()
fixture = (ROOT/'tests/ui_visibility/mm_interface_fixture.c').read_text()
names = ['Interface_Draw']
if 'static bool Interface_IsFullUi(' in source:
    names.insert(0, 'Interface_IsFullUi')
# Native inline capacity policy indexes the engine's item-slot mapping.
# Keep that table verbatim instead of inventing slots at this UI boundary.
inventory = (ROOT/'mm/src/code/z_inventory.c').read_text()
slots_begin = inventory.index('u8 gItemSlots[77] = {')
slots_end = inventory.index('};', slots_begin) + 2
production = inventory[slots_begin:slots_end] + '\n' + '\n'.join(function(source, name) for name in names)
fixture = fixture.replace('/* PRODUCTION */', production)
with tempfile.TemporaryDirectory(prefix='mm-ui-interface-') as directory:
    path = Path(directory)/'test.c'
    binary = Path(directory)/'test'
    path.write_text(fixture)
    command = ['cc', '-std=gnu11', '-g', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
                    '-DLOG_LEVEL_GAME_PRINTS=0', '-Wno-incompatible-pointer-types', '-Wno-int-conversion',
                    '-Wno-discarded-qualifiers', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                    *['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets','mm',
                                               'mm/2s2h','libultraship/include','combo']],
                    str(path), '-lm', '-o', str(binary)]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print('\n'.join(dict.fromkeys(line for line in result.stderr.splitlines()
              if 'error:' in line or 'undefined reference' in line)))
        result.check_returncode()
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
