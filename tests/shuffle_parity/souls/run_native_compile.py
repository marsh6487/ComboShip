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

ROOT = Path(__file__).resolve().parents[3]
FILES = (
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
for file in args.only or FILES:
    entry = next(e for e in entries if e['file'].endswith('/' + file))
    source_root = entry['file'][:-len(file)-1]
    command = shlex.split(entry['command'])
    rewritten = []
    i = 0
    while i < len(command):
        argument = command[i]
        if argument in ('-o', '-MF', '-MT', '-MQ'):
            i += 2
            continue
        if argument not in ('-MD', '-MMD', '-c'):
            rewritten.append(argument.replace(source_root, str(ROOT)))
        i += 1
    result = subprocess.run([*rewritten, '-fsyntax-only', '-O0'], cwd=entry['directory'],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(file, 'PASS' if result.returncode == 0 else 'FAIL', flush=True)
    if result.returncode:
        print(result.stdout, flush=True)
        raise SystemExit(result.returncode)
