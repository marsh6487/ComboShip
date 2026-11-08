#!/usr/bin/env python3
"""Compile the real MM dump, settings, pools and confinement with native types.

Engine configuration/RNG services and region membership are fixture boundaries.
The complete native catalogs and production function bodies are compiled unchanged.
"""
import argparse
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if match is None:
        raise ValueError('Missing production function: ' + name)
    return block_from(source, match.start())


def catalog(path, macro, declaration):
    source = path.read_text()
    begin = source.index('#define ' + macro + '(')
    body = source.index(declaration, begin)
    end = source.index('\n};', body) + 3
    return source[begin:end] + '\n#undef ' + macro + '\n'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--json-include', type=Path, default=Path('/usr/include'))
    p.add_argument('--sanitize', action='store_true')
    p.add_argument('--observe', action='store_true', help='Print baseline leaks without asserting the fix')
    p.add_argument('--failure-path-only', action='store_true')
    p.add_argument('--adapter-ref', help='Use the historical BenPort adapter body for a red baseline control')
    args = p.parse_args()
    data = ROOT / 'mm/2s2h/Rando/StaticData'
    item_functions = (data / 'Items.cpp').read_text()
    ben = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
    if args.adapter_ref:
        ben = subprocess.check_output(['git', 'show', args.adapter_ref + ':mm/2s2h/BenPort.cpp'],
                                      cwd=ROOT, text=True)
    starts = (ROOT / 'mm/2s2h/Rando/StartingItems.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='shuffle-boss-keys-') as directory:
        build = Path(directory)
        inc = catalog(data / 'Checks.cpp', 'RC', 'std::map<RandoCheckId, RandoStaticCheck> Checks')
        inc += catalog(data / 'Items.cpp', 'RI', 'std::map<RandoItemId, RandoStaticItem> Items')
        inc += catalog(data / 'Options.cpp', 'RO', 'std::map<RandoOptionId, RandoStaticOption> Options')
        inc += '\n'.join(function(item_functions, n) for n in ('GetItemIdFromName', 'ComboItemEmitNames',
                                                   'GetItemDisplayName', 'GetItemIdFromDisplayName'))
        (build / 'catalog.inc').write_text(inc)
        (build / 'starting.inc').write_text('\n'.join(function(starts, n) for n in (
            'GetComputedStartingItems', 'GetStartingItemsFromSave', 'SetStartingItemsInSave')))
        ship = (ROOT / 'mm/2s2h/FleetShipCombo/FleetShipCombo.cpp').read_text()
        (build / 'unified.inc').write_text(function(ship, 'FleetCombo_UnifiedPoolActive'))
        (build / 'adapter.inc').write_text('\n'.join(function(ben, n) for n in (
            'MM_DumpRandoSettings', 'MM_RestoreRandoSettings', 'MM_DumpRandoStaticData')))
        flags = ['-std=c++20', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX=1', '-DGBI_S32_VTX=1',
                 '-include', str(ROOT / 'tests/dungeon_rewards/headless.h'), '-I' + str(build),
                 '-I' + str(ROOT), '-I' + str(ROOT / 'combo')]
        for path in ('mm/include', 'mm/include/PR', 'mm/2s2h', 'mm', 'mm/src', 'mm/assets',
                     'libultraship/include'):
            flags += ['-isystem', str(ROOT / path)]
        if args.json_include.resolve() != Path('/usr/include'):
            flags += ['-isystem', str(args.json_include.resolve())]
        if args.sanitize:
            flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                      '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        binary = build / 'boss-keys'
        subprocess.run([os.environ.get('CXX', 'c++'), *flags,
                        str(Path(__file__).with_name('native_test.cpp')),
                        str(ROOT / 'mm/2s2h/Rando/Logic/GeneratePools.cpp'),
                        str(ROOT / 'mm/2s2h/Rando/Logic/PlacementConstraints.cpp'),
                        '-o', str(binary)], check=True)
        subprocess.run([str(binary), *(['--observe'] if args.observe else []),
                        *(['--failure-path-only'] if args.failure_path_only else [])], check=True,
                       timeout=360 if args.sanitize else 120)


if __name__ == '__main__':
    main()
