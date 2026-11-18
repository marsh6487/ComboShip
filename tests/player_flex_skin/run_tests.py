#!/usr/bin/env python3
"""Compile both real host bridges and execute native FlexLOD plus NS/v1 resolver."""
import os
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags as mm_flags
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--fast-math', action='store_true')
parser.add_argument('--host', choices=['oot', 'mm'])
args = parser.parse_args()

def run(command):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        lines = (result.stdout + result.stderr).splitlines()
        print('\n'.join(lines if len(lines) < 120 else [x for x in lines if 'error:' in x][:30] + lines[-35:]))
        raise SystemExit(result.returncode)
    if result.stdout: print(result.stdout.strip())

def oot_flags():
    result = ['-DF3DEX_GBI_2', '-DLOG_LEVEL_GAME_PRINTS=0', '-DCOMBO_BUILD']
    result += ['-I' + str(ROOT / path) for path in ['soh/include', 'soh/src', 'soh/assets', 'soh', 'soh/mods', 'libultraship/include', 'combo', '']]
    for path in ['CMake/soh-cvars.cmake', 'CMake/lus-cvars.cmake']:
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            result.append(f'-D{key}="{value}"')
    return result

def function(source, name):
    start = source.index('extern "C" const char* ' + name + '(')
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

with tempfile.TemporaryDirectory(prefix='player-flex-skin-') as directory:
    temporary = Path(directory)
    for host in ([args.host] if args.host else ['oot', 'mm']):
        flags = oot_flags() if host == 'oot' else [*mm_flags(), '-DCOMBO_BUILD', '-DPLAYER_SKIN_MM']
        cpp = 'soh/soh/Enhancements/cosmetics/PlayerFlexSkin.cpp' if host == 'oot' else 'mm/2s2h/Enhancements/Graphics/PlayerFlexSkin.cpp'
        run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags, '-fsyntax-only', cpp])
        game = 'soh' if host == 'oot' else 'mm'
        (temporary / 'pak_query.inc').write_text(function((ROOT / game / 'mods/pak_loader/pak_loader.cpp').read_text(), 'PakLoader_GetSkinSkeletonPath'))
        query = temporary / f'pak-query-{host}'
        run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags, '-I' + str(temporary),
             'tests/player_flex_skin/pak_query_test.cpp', '-o', str(query)])
        run([str(query)])
        for mode in ['legacy', 'wide']:
            native = [*flags, '-O1', '-g', '-ffunction-sections', '-fdata-sections']
            if args.fast_math: native += ['-O2', '-ffast-math']
            if mode == 'wide': native += ['-DGBI_FLOAT_MTX=1', '-DGBI_S32_VTX=1']
            if args.sanitize:
                native += ['-fsanitize=address,undefined,bounds,float-cast-overflow', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
            obj = temporary / f'{host}-{mode}.o'
            run([os.environ.get('CC', 'cc'), '-std=gnu2x', *native, '-Wno-incompatible-pointer-types',
                 '-Wno-int-conversion', '-Wno-discarded-qualifiers', '-c',
                 f'{game}/src/code/z_skelanime.c', '-o', str(obj)])
            binary = temporary / f'draw-{host}-{mode}'
            run([os.environ.get('CXX', 'c++'), '-std=c++20', *native, 'tests/player_flex_skin/draw_test.cpp', str(obj), '-Wl,--gc-sections', '-lm', '-o', str(binary)])
            run([str(binary)])
            bridge = temporary / f'bridge-{host}-{mode}'
            run([os.environ.get('CXX', 'c++'), '-std=c++20', *native, 'tests/player_flex_skin/bridge_test.cpp', '-Wl,--gc-sections', '-o', str(bridge)])
            run([str(bridge)])
            print('PASS', host, mode, flush=True)
