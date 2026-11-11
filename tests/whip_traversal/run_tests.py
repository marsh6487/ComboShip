#!/usr/bin/env python3
"""Run production whip aim, draw dispatch, reticle, flight and surface analysis.

Native engine types; extracted functions are verbatim production bodies. Mesh,
audio, camera and collision-query services are fixture boundaries, not gameplay.
"""
from pathlib import Path
from run_native_compile import run_native_compile
import os
import re
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent

def function(source, name):
    match = re.search(r'^.*\b' + name + r'\([^;]*?\)\s*\{', source, re.M)
    if not match:
        raise ValueError(name)
    start = match.start()
    cursor = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]

with tempfile.TemporaryDirectory(prefix='whip-traversal-') as temporary:
    output = Path(temporary)
    for host in ('mm', 'soh'):
        logic = (ROOT / host / 'mods/items/logic/item_whip.c').read_text()
        camera = (ROOT / host / 'mods/items/helpers/camera_helper.c').read_text()
        grapple = (ROOT / host / 'mods/items/helpers/grappling_helper.c').read_text()
        common = (ROOT / host / 'mods/items/custom_items_common.c').read_text()
        dispatch = function(common, 'CustomItems_OverrideDraw')
        # Omit unrelated item meshes, retaining the complete actual reticle dispatch.
        dispatch = 's32 CustomItems_OverrideDraw(Player* p, PlayState* play) {\n' + dispatch[dispatch.index('    // Draw reticle'):]
        source = ''
        if host == 'mm':
            source += 's8 gNeiFpAimMode; static s16 sFpAimTimer;\n'
            start = camera.index('static Vtx sReticleVtx[')
            source += camera[start:camera.index('\n};', start)+3] + '\n'
        source += '\n'.join(function(camera, name) for name in (
            'FirstPerson_Init', 'FirstPerson_Update', 'FirstPerson_Exit',
            'FirstPerson_GetAimYaw', 'FirstPerson_GetAimPitch', 'FirstPerson_DrawReticle'))
        source += '\n' + dispatch
        # Match production: camera/common precede the alias-heavy item header.
        source += '\n#include "mods/items/logic/item_whip.h"\n'
        # Include the real shape classifier and its private geometric helpers.
        source += '\n' + re.sub(r'^#include[^\n]*', '', grapple[:grapple.index('// Grapple_FindTarget')], flags=re.M)
        source += '\n'.join(function(logic, name) for name in (
            'Whip_IsGrappleActor', 'Whip_IsParalyzeTarget', 'Whip_IsDisarmTarget',
            'Whip_ApplyParalyze', 'Whip_ApplyDisarm', 'Whip_ApplyBoomerangDamage',
            'Whip_FindGrappleActor', 'Whip_FindNearbyEnemy', 'WhipStateEquip', 'WhipStateExtending'))
        drawing = (ROOT / host / 'mods/items/objects/object_whip.c').read_text()
        source += '\n' + '\n'.join(re.findall(r'^#define WHIP_BODY_(?:SEGMENT|MAX_SEGS|SCALE)[^\n]*', drawing, re.M))
        source += '\n' + function(drawing, 'Whip_DrawSnakeBody')
        (output/'production.inc').write_text(source)
        includes = ('.', host, host+'/include', host+'/include/PR', host+'/src', host+'/assets',
                    host+('/2s2h' if host=='mm' else '/soh'), 'libultraship/include', 'combo')
        for combo in (False, True):
            binary = output / (host + ('-combo' if combo else '-standalone'))
            command = [os.environ.get('CXX','c++'), '-std=c++20', '-w', '-DF3DEX_GBI_2',
                       '-DLOG_LEVEL_GAME_PRINTS=0', '-ffunction-sections', '-fdata-sections',
                       '-Wl,--gc-sections', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                       '-fno-omit-frame-pointer', '-no-pie', '-I'+str(output),
                       *['-I'+str(ROOT/p) for p in includes]]
            if host=='mm': command.append('-DWHIP_FIXTURE_MM')
            if combo: command.append('-DCOMBO_BUILD')
            result = subprocess.run(command+[str(HERE/'native_test.cpp'),'-lm','-o',str(binary)], text=True, capture_output=True)
            if result.returncode: raise SystemExit(result.stderr)
            result = subprocess.run([str(binary), *(['flight-only'] if '--flight-only' in sys.argv else [])], env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'}, text=True, capture_output=True)
            print(binary.name + ': ' + result.stdout + result.stderr, flush=True)
            if result.returncode: raise SystemExit(result.returncode)

# Compile the actual full player units as well as the behavioral fixture.
run_native_compile()
