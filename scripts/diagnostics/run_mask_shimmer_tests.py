#!/usr/bin/env python3
"""Exercise native and cross-game mask/remains shimmer with both engines' real headers/GBI."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    match = re.search(r'^(?:static )?(?:void|s32) ' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise ValueError(name)
    pos, depth = match.end(), 1
    while depth:
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[match.start():pos]

flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0', '-DCONTROLLERBUTTONS_T=uint32_t',
         '-DNON_EQUIVALENT', '-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='mask-shimmer-') as temporary:
    build = Path(temporary)
    for host, game in [('mm', 'mm'), ('oot', 'soh')]:
        native = (ROOT / game / 'src/code/z_draw.c').read_text()
        names = ['GetItem_GetShimmerColor', 'GetItem_Draw']
        if host == 'mm':
            names.insert(1, 'GetItem_DrawShimmer')
        fixture = (ROOT / 'tests/mask_shimmer/presentation_test.cpp').read_text()
        fixture = fixture.replace('/* PRODUCTION_NATIVE */', 'extern "C" {\n' + '\n'.join(function(native, n) for n in names) + '\n}')
        source = build / 'test.cpp'
        source.write_text(fixture)
        includes = [game, game+'/include', game+'/include/PR', game+'/src', game+'/assets', game+'/2s2h',
                    game+'/soh', game+'/mods', 'libultraship/include', 'libultraship/src', 'combo/menu']
        binary = build / host
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=gnu++20', *flags,
                        *(['-DHOST_MM'] if host == 'mm' else []), *['-I'+str(ROOT/p) for p in includes],
                        str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
