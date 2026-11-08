#!/usr/bin/env python3
"""Compile actual MM item/menu/hint/persistence bodies with native headers."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/ui_visibility'))
from run_control_input_tests import compiler_flags, function


def main():
    items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    source = (ROOT / 'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp').read_text()
    menu = (ROOT / 'mm/2s2h/BenGui/BenMenu.cpp').read_text()
    runtime = (ROOT / 'mm/2s2h/Rando/Rando.cpp').read_text()
    start = items.index('std::map<RandoItemId, RandoStaticItem> Items = {')
    end = items.index('\n};', start) + len('\n};')
    table = items[start:end]
    with tempfile.TemporaryDirectory(prefix='mm-desired-native-') as temporary:
        build = Path(temporary)
        (build / 'items.inc').write_text(table)
        bodies = [function(source, name) for name in (
            'IsExcludedFromSariaPriorityItemCandidates',
            'Rando::GetSariaPriorityItemCandidates',
            'Rando::GetSariaPriorityItemsFromSpoiler',
            'Rando::SetSariaPriorityItemsInSpoiler',
            'Rando::GetSariaPriorityItemsFromSave',
            'Rando::SetSariaPriorityItemsInSave')]
        bodies += ['namespace Rando::StaticData {\n' +
                   function(items, 'GetItemIdFromName') + '\n}\n']
        (build / 'saria.inc').write_text('\n'.join(bodies))
        (build / 'sensor_menu.inc').write_text(function(menu, 'SensorItemChoices') +
                                               function(menu, 'SensorDesireName'))
        (build / 'placement.inc').write_text(function(runtime, 'Rando::FindMultiItemPlacement'))
        (build / 'selection.inc').write_text(function(source, 'GetProgressiveCheckInLogic') +
                                              function(runtime, 'SensorFindOutstandingCheck'))
        for combo in (False, True):
            flags = [f for f in compiler_flags('mm', combo)
                     if not f.startswith('-Wno-') and not f.startswith('-std=')
                     and f != '-Werror=implicit-function-declaration']
            binary = build / ('combo' if combo else 'original')
            command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-I' + str(build),
                       '-include', str(ROOT / 'tests/dungeon_rewards/headless.h'),
                       *flags, str(ROOT / 'tests/shuffle_parity/hints/native_test.cpp'),
                       '-o', str(binary)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
            print('PASS real MM Desired Items/Saria catalogue and persistence',
                  'Combo' if combo else 'original', flush=True)


if __name__ == '__main__':
    main()
