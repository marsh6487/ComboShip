#!/usr/bin/env python3
"""Syntax-check real cel policy and native integration TUs against current headers."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dependencies', type=Path, required=True, help='prepared dependency source root (imgui, prism, threadpool)')
parser.add_argument('--case', help='one filename stem, e.g. z_actor or ToonLighting')
parser.add_argument('--log-dir', type=Path)
args = parser.parse_args()
values = {}
for path in ('CMake/soh-cvars.cmake', 'CMake/lus-cvars.cmake'):
    for key, value in re.findall(r'set\((CVAR_\w+)\s+"([^"\n]*)"', (ROOT/path).read_text()):
        value = re.sub(r'\$\{(\w+)\}', lambda m: values[m[1]], value)
        values[key] = value
flags = ['-fsyntax-only', '-w', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
         '-DCOMBO_BUILD', '-DNON_EQUIVALENT', '-DNON_MATCHING', '-DLOG_LEVEL_GAME_PRINTS=0']
flags += [f'-D{key}="{value}"' for key, value in values.items()]
flags += ['-I'+str(ROOT/path) for path in ('', 'soh', 'soh/include', 'soh/src', 'soh/assets', 'soh/soh', 'combo', 'combo/menu', 'libultraship/include')]
flags += ['-I'+str(args.dependencies/path) for path in ('imgui', 'imgui/backends', 'prism/src', 'threadpool/include')]
flags += ['-I/usr/include/SDL2']
units = [ROOT/'soh/soh/Enhancements/Graphics'/(name+'.cpp') for name in (
    'ToonLighting', 'WorldLighting', 'DekuStickLight', 'WWClouds', 'WWNightSky', 'WWSkyEnv', 'WWSkyGradient', 'WWWindWisps')]
units += [ROOT/'soh/soh/SohGui/SohMenuWindWakerStyle.cpp', ROOT/'soh/soh/Enhancements/game-interactor/GameInteractor_Hooks.cpp']
units += [ROOT/path for path in (ROOT/'tests/cel_sky').joinpath('native_paths.txt').read_text().splitlines() if path.endswith('.c')]
if args.case:
    units = [unit for unit in units if unit.stem == args.case]
    if not units:
        parser.error('unknown source stem')
with tempfile.TemporaryDirectory(prefix='cel-source-compile-') as temporary:
    logs = args.log_dir or Path(temporary)
    logs.mkdir(parents=True, exist_ok=True)
    failed = []
    for unit in units:
        command = ['cc' if unit.suffix == '.c' else 'c++', '-std=gnu2x' if unit.suffix == '.c' else '-std=gnu++20', *flags]
        if unit.suffix == '.c': command += ['-Werror=implicit-function-declaration']
        command += [str(unit)]
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (logs/(unit.stem+'.log')).write_text(result.stdout)
        if result.returncode:
            failed.append(unit.name)
            print('FAIL:', unit.relative_to(ROOT), flush=True)
            print('\n'.join(line for line in result.stdout.splitlines() if 'error:' in line)[:4000], flush=True)
        else:
            print('PASS:', unit.relative_to(ROOT), flush=True)
    if failed:
        raise SystemExit('Native syntax failed: '+', '.join(failed))
    print(f'Native syntax: {len(units)} production translation units PASS')
