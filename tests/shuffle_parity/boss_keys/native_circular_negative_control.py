#!/usr/bin/env python3
"""Actual linked MM oracle controls for the multiset-preserving circular seed."""
import argparse
from collections import Counter
import ctypes
import json
from pathlib import Path

import native_boss_key_regression as native


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    payload = json.loads(args.input.read_text())
    original, altered, probe = payload['original'], payload['altered'], payload['probe']
    data = probe['dump']; before = original['mm']['placements']; after = altered['mm']['placements']
    rows = native.metadata(data)
    def check(source):
        matches = [name for name, row in rows.items() if row['sourceName'] == source]
        native.require(len(matches) == 1 and matches[0] in before, 'native circular check missing: ' + source)
        return matches[0]
    boss = check('RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP')
    heart = check('RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_HEART_CONTAINER')
    key = next(name for name, item in before.items() if native.plain(item) == 'Stone Tower Boss Key')
    foreign = {(row['checkGame'], row['checkName']) for row in original.get('foreign', [])}
    native.require(len({key, boss, heart}) == 3 and
                   all(('mm', name) not in foreign for name in (key, boss, heart)),
                   'native circular control requires three distinct native MM checks')
    native.require(Counter(before.values()) == Counter(after.values()) and
                   original.get('foreign', []) == altered.get('foreign', []),
                   'circular seed changed qualified supply or ownership')
    expected = json.loads(json.dumps(original))
    p = expected['mm']['placements']; p[key], p[boss], p[heart] = p[heart], p[key], p[boss]
    native.require(expected == altered, 'circular seed changed fields outside the exact three-cycle')
    native.require(native.plain(before[boss]) == "Twinmold's Remains" and
                   native.plain(after[boss]) == 'Stone Tower Boss Key' and
                   native.plain(after[heart]) == "Twinmold's Remains", 'native goal reward moved outside the key lock')
    host = native.Native('mm', False)
    host.restore(original['mm']['settings'])
    host.call('SetComboRandoSeed', original['masterSeed'], args=(ctypes.c_uint64,))
    host.call('SetCheckPrices', json.dumps(original['mm']['prices']).encode(), args=(ctypes.c_char_p,))
    reset = host.bind('Combo_MM_Rando_Reset')
    owned = host.bind('Combo_MM_Rando_SetOwnedItems', args=(ctypes.c_char_p,))
    reach = host.bind('Combo_MM_Rando_GetReachableChecks', ctypes.c_char_p)
    restore = host.bind('Combo_MM_Rando_Restore')
    inventory = [row['name'] for row in data['pool']] + [row['item'] for row in data['fixed']]
    native.require(inventory.count('Stone Tower Boss Key') == 1, 'native boss-key supply changed')
    results = {}
    try:
        for label, items in [('missing', [item for item in inventory if item != 'Stone Tower Boss Key']),
                             ('owned', inventory)]:
            reset(); owned(json.dumps(items).encode()); reachable = set(json.loads(reach()))
            results[label] = {name: name in reachable for name in (key, boss, heart)}
        native.require(results['missing'][key] and not results['missing'][boss] and not results['missing'][heart],
                       'actual native boss room does not enforce its missing key')
        native.require(all(results['owned'].values()), 'actual native correct key cannot open both boss-room checks')
    finally:
        restore()
    native.write(args.output, {'actualNativeGates': results, 'keyCheck': key, 'bossWarp': boss, 'bossHeart': heart,
                              'exactQualifiedMultisetPreserved': True, 'foreignMetadataPreserved': True})
    print('PASS actual native missing/owned Stone Boss Key gates; exact three-cycle supply/goal/ownership preserved')


if __name__ == '__main__':
    main()
