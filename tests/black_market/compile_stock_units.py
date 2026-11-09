#!/usr/bin/env python3
"""Compile Black Market stock/purchase units with real headers; never link/run a game.

Supply third-party build headers through CPATH or --include. Each log records
the command, dependency hashes, source hash and compiler result.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
UNITS = [
    'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp',
    'soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp',
    'soh/src/code/z_play.c',
]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--include', type=Path, action='append', default=[])
parser.add_argument('--unit', choices=UNITS, action='append')
parser.add_argument('--jobs', type=int, default=2)
parser.add_argument('--standalone', action='store_true', help='compile without COMBO_BUILD')
parser.add_argument('--out', type=Path, default=ROOT.parent / 'black-market-verification')
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compile_unit(unit):
    host = unit.split('/')[0]
    source = ROOT / unit
    c = source.suffix == '.c'
    roots = [ROOT] + [ROOT / host / p for p in ('', 'include', 'include/PR', 'src', 'assets', 'mods',
                                      '2s2h' if host == 'mm' else 'soh')]
    roots += [ROOT / p for p in ('combo', 'combo/menu', 'combo/title', 'libultraship/include',
                                'libultraship', 'ZAPDTR/ZAPD/resource/type', 'torch/lib/n64graphics')]
    roots += args.include
    prefixes = re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                          (ROOT / 'CMake' / ('2ship-cvars.cmake' if host == 'mm' else 'soh-cvars.cmake')).read_text())
    standard = ('gnu17' if host == 'mm' else 'gnu2x') if c else 'c++20'
    command = [os.environ.get('CC' if c else 'CXX', 'cc' if c else 'c++'), '-std=' + standard,
               '-w', '-DF3DEX_GBI_2', *([] if args.standalone else ['-DCOMBO_BUILD']), '-DFMT_HEADER_ONLY', '-DENABLE_OPENGL',
               '-D_CONSOLE', '-DLOG_LEVEL_GAME_PRINTS=6', '-fvisibility=hidden',
               '-DMM_BUILD_DLL' if host == 'mm' else '-DSOH_BUILD_DLL',
               *[f'-D{key}="{value}"' for key, value in prefixes]]
    if host == 'soh':
        command += ['-Dfunc_80041DB8=SurfaceType_GetWallFlags']
    if c:
        command += ['-DNON_EQUIVALENT', '-DNON_MATCHING', '-DCONTROLLERBUTTONS_T=uint32_t', '-Wno-error=incompatible-pointer-types', '-Wno-error=int-conversion']
    else:
        command += ['-fpermissive', '-Wno-narrowing']
    slug = host + '-' + source.stem + ('-standalone' if args.standalone else '')
    obj, dep = args.out / (slug + '.o'), args.out / (slug + '.d')
    command += ['-I' + str(path) for path in roots]
    command += ['-MMD', '-MF', str(dep), '-c', str(source), '-o', str(obj)]
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    hashes = {'source': digest(source)}
    if result.returncode == 0:
        dependencies = shlex.split(dep.read_text().replace('\\\n', ''))[1:]
        hashes['dependencies'] = {str(Path(p)): digest(Path(p)) for p in dependencies}
        hashes['object'] = digest(obj)
    (args.out / (slug + '-dependencies.json')).write_text(json.dumps(hashes, sort_keys=True, indent=2) + '\n')
    (args.out / (slug + '.log')).write_text('Command: ' + json.dumps(command) + '\nCPATH: ' +
        os.environ.get('CPATH', '') + '\n' + result.stdout + '\nExit: ' + str(result.returncode) + '\n')
    print(('PASS' if result.returncode == 0 else 'FAIL') + ' native market unit: ' + unit, flush=True)
    if result.returncode:
        print(result.stdout[:4000], flush=True)
    return result.returncode


with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
    results = list(pool.map(compile_unit, args.unit or UNITS))
raise SystemExit(any(results))
