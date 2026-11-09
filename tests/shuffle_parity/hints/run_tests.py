#!/usr/bin/env python3
"""Compile actual MM item/menu/hint/persistence bodies with native headers."""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/ui_visibility'))
from run_control_input_tests import compiler_flags, function
sys.path.insert(0, str(ROOT / "tests/cross_hints"))
from run_item_hint_export_tests import block


def main():
    items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    source = (ROOT / 'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp').read_text()
    menu = (ROOT / 'mm/2s2h/BenGui/BenMenu.cpp').read_text()
    runtime = (ROOT / 'mm/2s2h/Rando/Rando.cpp').read_text()
    start = items.index('std::map<RandoItemId, RandoStaticItem> Items = {')
    end = items.index('\n};', start) + len('\n};')
    table = items[start:end]
    provider = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='mm-desired-native-') as temporary:
        build = Path(temporary)
        (build / 'items.inc').write_text(table)
        convert = (ROOT / 'mm/2s2h/Rando/ConvertItem.cpp').read_text()
        (build / 'convert.inc').write_text(function(convert, 'Rando::ConvertItem'))
        rpg = (ROOT / 'mm/mods/combo_rpg.cpp').read_text()
        (build / 'native_magic.inc').write_text(function(rpg, 'ComboRpg_IsEnabled') + function(rpg, 'ComboRpg_NativeMagicTier'))
        bodies = [function(source, name) for name in (
            'IsExcludedFromSariaPriorityItemCandidates',
            'Rando::GetSariaPriorityItemCandidates',
            'Rando::GetSariaPriorityItemsFromSpoiler',
            'Rando::SetSariaPriorityItemsInSpoiler',
            'Rando::GetSariaPriorityItemsFromSave',
            'Rando::SetSariaPriorityItemsInSave')]
        bodies += ['namespace Rando::StaticData {\n' +
                   function(items, 'GetItemIdFromName') + function(items, 'GetItemName') + '\n}\n']
        (build / 'saria.inc').write_text('\n'.join(bodies))
        (build / 'sensor_menu.inc').write_text(function(menu, 'SensorItemChoices') +
                                               function(menu, 'SensorDesireName'))
        (build / 'placement.inc').write_text(function(runtime, 'Rando::FindMultiItemPlacement'))
        (build / 'selection.inc').write_text(function(source, 'GetProgressiveCheckInLogic') +
                                              function(runtime, 'SensorFindOutstandingCheck') +
                                              function(runtime, 'Rando::GetCrossWorldHintTargets') +
                                              function(source, 'Rando::GetSariaHintTarget') +
                                              function(runtime, 'Rando::GetDesiredItemHintTarget') +
                                              function(runtime, 'SensorGetDesire') +
                                              function(runtime, 'SensorRune_PrepareHint'))
        (build / 'provider.inc').write_text(function(provider, 'MMHintReachableChecks') +
                                             function(provider, 'OOT_GetMMHintCandidates'))
        native = []
        for path, names in (
            ('soh/soh/Enhancements/randomizer/item_location.cpp', (
                'ItemLocation::ItemLocation', 'ItemLocation::SetPlacedItem',
                'ItemLocation::GetPlacedRandomizerGet', 'ItemLocation::HasObtained',
                'ItemLocation::GetCheckStatus', 'ItemLocation::GetIsSkipped', 'ItemLocation::IsExcluded',
                'ItemLocation::SetExcludedOption', 'ItemLocation::IsAddedToPool',
                'ItemLocation::AddToPool', 'ItemLocation::RemoveFromPool', 'ItemLocation::SetPrice')),
            ('soh/soh/Enhancements/randomizer/option.cpp', (
                'OptionValue::OptionValue', 'OptionValue::Get', 'OptionValue::Set')),
            ('soh/soh/Enhancements/randomizer/logic.cpp', ('Logic::SetSaveContext',)),
            ('soh/soh/Enhancements/randomizer/item.cpp', ('Item::GetName', 'Item::GetPrice')),
            ('soh/soh/Enhancements/randomizer/entrance.cpp', (
                'Entrance::IsAddedToPool', 'Entrance::AddToPool', 'Entrance::RemoveFromPool'))):
            text = (ROOT / path).read_text()
            for name in names:
                pos = text.index(name + '(')
                start = text.rfind('\n', 0, pos) + 1
                native.append(block(text, text[start:pos + len(name)]))
        (build / 'provider_native.inc').write_text('namespace Rando {\n' + '\n'.join(native) + '\n}\n')
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
        flags = [f for f in compiler_flags('soh', True)
                 if not f.startswith('-Wno-') and not f.startswith('-std=')
                 and f != '-Werror=implicit-function-declaration']
        binary = build / 'provider'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-I' + str(build),
                        *flags, str(ROOT / 'tests/shuffle_parity/hints/provider_test.cpp'),
                        '-o', str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
