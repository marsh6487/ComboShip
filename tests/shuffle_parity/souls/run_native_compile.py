#!/usr/bin/env python3
"""Check actual production translation units using an existing configured build.

Only reads the build database; substitutes this checkout's source/include paths.
Never writes the configured build's outputs or dependency files.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
FILES = (
    "mm/2s2h/Rando/Logic/Regions/MilkRoad.cpp",
    'soh/soh/Enhancements/randomizer/SoulShuffleShared.cpp',
    'mm/2s2h/Rando/SoulShuffleShared.cpp',
    'soh/soh/FleetShipCombo/FleetSharedItems.cpp',
    'mm/2s2h/FleetShipCombo/FleetSharedItems.cpp',
    'soh/soh/OTRGlobals.cpp',
    "soh/soh/Enhancements/randomizer/location_access/overworld/gerudo_valley.cpp",
    "soh/soh/Enhancements/randomizer/location_access/overworld/kakariko.cpp",
    "soh/soh/Enhancements/randomizer/location_access/overworld/zoras_river.cpp",
    "soh/soh/Enhancements/randomizer/location_access/dungeons/jabujabus_belly.cpp",
    "mm/2s2h/Rando/Logic/Regions/Central.cpp",
    "mm/2s2h/Rando/Logic/Regions/BeneathTheWell.cpp",
    "soh/src/overlays/actors/ovl_player_actor/z_player.c",
    'mm/src/code/z_actor.c', 'mm/2s2h/Rando/SoulShuffleRuntime.cpp',
    'soh/src/code/z_actor.c', 'soh/src/code/z_collision_check.c',
    'soh/soh/Enhancements/randomizer/SoulShuffleRuntime.cpp',
    'mm/2s2h/Rando/StaticData/Items.cpp', 'mm/2s2h/Rando/StaticData/Options.cpp',
    'mm/2s2h/Rando/GiveItem.cpp', 'mm/2s2h/Rando/RemoveItem.cpp',
    'mm/2s2h/Rando/Logic/GeneratePools.cpp',
    'soh/soh/Enhancements/randomizer/item_list.cpp',
    'soh/soh/Enhancements/randomizer/logic.cpp',
    'soh/soh/Enhancements/randomizer/3drando/item_pool.cpp',
    'soh/soh/Enhancements/randomizer/settings.cpp', 'mm/2s2h/BenPort.cpp',
)
parser = argparse.ArgumentParser()
parser.add_argument('--database', type=Path, required=True)
parser.add_argument('--only', action='append', choices=FILES)
args = parser.parse_args()
entries = json.loads(args.database.read_text())
# Do not import another worktree's native definitions through a compiled PCH.
# Retain the same forced-header contents, remapped to this source tree, as text.
pch_temp = tempfile.TemporaryDirectory(prefix='soul-native-pch-')
for file in args.only or FILES:
    template = file
    if file.endswith(('/SoulShuffleRuntime.cpp', '/SoulShuffleShared.cpp')) and not any(e['file'].endswith('/'+file) for e in entries):
        template = ('mm/2s2h/Rando/StaticData/Items.cpp' if file.startswith('mm/')
                    else 'soh/soh/Enhancements/randomizer/logic.cpp')
    entry = next(e for e in entries if e['file'].endswith('/' + template))
    source_root = entry['file'][:-len(template)-1]
    command = shlex.split(entry['command'])
    rewritten = []
    i = 0
    while i < len(command):
        argument = command[i]
        if argument == '-include' and 'cmake_pch' in command[i + 1]:
            header = Path(pch_temp.name) / (str(len(list(Path(pch_temp.name).iterdir()))) + '.h')
            header.write_text(Path(command[i + 1]).read_text().replace(source_root, str(ROOT)))
            rewritten += ['-include', str(header)]
            i += 2
            continue
        if argument in ('-o', '-MF', '-MT', '-MQ'):
            i += 2
            continue
        if argument not in ('-MD', '-MMD', '-c'):
            if argument == entry['file']:
                rewritten.append(str(ROOT / file))
            else:
                rewritten.append(argument.replace(source_root, str(ROOT)))
        i += 1
    result = subprocess.run([*rewritten, '-fsyntax-only', '-O0'], cwd=entry['directory'],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(file, 'PASS' if result.returncode == 0 else 'FAIL', flush=True)
    if result.returncode:
        print(result.stdout, flush=True)
        raise SystemExit(result.returncode)
