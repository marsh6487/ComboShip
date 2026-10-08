#!/usr/bin/env python3
"""Check changed sword/Ikana translation units using a configured native build.

The compilation database may belong to another checkout of this repository.
Source include roots are remapped; generated build/dependency paths are retained.
This checks syntax with real compiler settings, without claiming a linked build.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
FILES = (
    'soh/soh/Enhancements/randomizer/draw.cpp',
    'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp',
    'soh/src/code/z_player_lib.c', 'mm/src/code/z_player_lib.c',
    'soh/src/overlays/actors/ovl_player_actor/z_player.c',
    'mm/src/overlays/actors/ovl_player_actor/z_player.c',
    'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c',
    'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c',
    'mm/mods/items/logic/adult_link_render.cpp',
)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compile-commands', type=Path, required=True)
args = parser.parse_args()
records = json.loads(args.compile_commands.read_text())
for relative in FILES:
    row = next((r for r in records if r['file'].endswith('/' + relative)), None)
    if row is None:
        raise RuntimeError('Missing native compilation record: ' + relative)
    original = row['file'][:-len(relative)]
    command = shlex.split(row['command']) if 'command' in row else row['arguments']
    clean = []
    cursor = 0
    while cursor < len(command):
        argument = command[cursor]
        cursor += 1
        if argument == '-o':
            cursor += 1
            continue
        if argument == '-c':
            continue
        clean.append(argument.replace(original, str(ROOT) + '/'))
    result = subprocess.run([*clean, '-fsyntax-only'], cwd=row['directory'], capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise RuntimeError(relative + ' syntax failed')
    print(f'PASS native compiler record syntax: {relative} ({result.stderr.count("warning:")} warnings)', flush=True)
