#!/usr/bin/env python3
"""Execute actual OoT spawn/anchor/safety bodies; no game or assets are launched."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--case', default='all', choices=['all', 'center', 'timing', 'safety', 'wall'])
args = parser.parse_args()
source = (ROOT / 'soh/soh/Enhancements/Gameplay/ComboVanillaRewards.cpp').read_text()

def function(signature, code=source):
    start = code.index(signature)
    brace = code.index('{', start)
    cursor, depth = brace + 1, 1
    while depth:
        depth += (code[cursor] == '{') - (code[cursor] == '}')
        cursor += 1
    return code[start:cursor]

production = re.search(r'struct NativeRewardBinding \{.*?\n\};', source, re.S)[0] + '\n'
production += '\n'.join(function(signature) for signature in (
    'static NativeRewardBinding NativeBinding(', 'static bool Safe(',
    'static bool EntranceMatches(', 'static bool Anchor(',
    'static bool SafeRewardPosition(', 'static void SpawnRewards('))
bgcheck = (ROOT/'soh/src/code/z_bgcheck.c').read_text()
math_vectors = (ROOT/'soh/src/code/z_lib.c').read_text()
production += '\n' + '\n'.join(function(signature, math_vectors) for signature in (
    'void Math_Vec3i_ToVec3f(', 'void Math_Vec3f_Diff('))
production += '\n' + '\n'.join(function(signature, bgcheck) for signature in (
    'void CollisionPoly_GetNormalF(', 'void CollisionPoly_GetVertices(', 's32 CollisionPoly_LineVsPoly('))
fixture = (ROOT / 'tests/combo_vanilla/oot_reward_spawn_test.cpp').read_text().replace('/* PRODUCTION */', production)
with tempfile.TemporaryDirectory(prefix='oot-reward-spawn-') as directory:
    work = Path(directory)
    cpp, binary = work/'test.cpp', work/'test'
    cpp.write_text(fixture)
    flags = ['-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DLOG_LEVEL_GAME_PRINTS=0',
             '-fno-pie', '-ffunction-sections', '-fdata-sections']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    paths = (ROOT, ROOT/'combo', ROOT/'soh', ROOT/'soh/include', ROOT/'soh/src', ROOT/'soh/assets',
             ROOT/'soh/soh', ROOT/'libultraship/include')
    includes = ['-I'+str(p) for p in paths]
    math_object = work/'math3d.o'
    subprocess.run([*shlex.split(os.environ.get('CC','cc')), '-std=gnu2x', *flags, *includes,
                    '-c', str(ROOT/'soh/src/code/sys_math3d.c'), '-o', str(math_object)], check=True)
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')), '-std=c++20', *flags, *includes,
                    str(cpp), str(math_object), '-Wl,--gc-sections', '-no-pie', '-lm', '-o', str(binary)], check=True)
    subprocess.run([str(binary), args.case], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
