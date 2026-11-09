#!/usr/bin/env python3
"""Actual linked native solo/combined Glitchless fills and independent replay.

No replacement graph or fabricated assets. All assertions consume native modules'
settings, pool, scene metadata and filled seeds. Each native session is isolated.
"""
import argparse
from collections import Counter
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/shuffle_parity/enemy_drops'))
from native_seed_regression import require, write, prepare_runtime, module_hashes, run, require_fresh_output
from native_seed_config import expected_checks as first_drop_checks

SEED = 20261109
CASE_NAMES = ('own', 'own-excluded', 'stone-normal', 'stone-inverted', 'rusty-keys', 'mm-parity', 'all-off', 'all-parity')
KEY_SCENES = {
    'oot': {**{x + ' Temple Boss Key': ('SCENE_' + x.upper() + '_TEMPLE',)
              for x in ('Forest', 'Fire', 'Water', 'Spirit', 'Shadow')},
            "Ganon's Castle Boss Key": ('SCENE_INSIDE_GANONS_CASTLE',)},
    'mm': {'Woodfall Boss Key': ('SCENE_MITURIN', 'SCENE_MITURIN_BS'),
           'Snowhead Boss Key': ('SCENE_HAKUGIN', 'SCENE_HAKUGIN_BS'),
           'Great Bay Boss Key': ('SCENE_SEA', 'SCENE_SEA_BS'),
           'Stone Tower Boss Key': ('SCENE_INISIE_N', 'SCENE_INISIE_R', 'SCENE_INISIE_BS')}}
ENV_OPTIONS = ('ICICLE', 'ROCK', 'BOULDER', 'RED_BOULDER', 'SOIL')
OOT_FEATURES = ('LockOverworldDoors', 'ShuffleEnemyDrops', 'ShuffleSongEvents', 'ShuffleClock',
                'ShuffleEnemySouls', 'ShuffleNpcSouls', 'ShuffleAnimalSouls', 'ShuffleMiscSouls', 'ShuffleBossSouls')
MM_FEATURES = ('SHUFFLE_RUSTY_KEYS', 'SHUFFLE_DEKU_STICK_BAG', 'SHUFFLE_DEKU_NUT_BAG', 'SHUFFLE_CHILD_WALLET',
               'INFINITE_UPGRADES', 'SHUFFLE_LOTTERY_NIGHTS', 'SHUFFLE_KEY_RINGS', 'SHUFFLE_SONG_EVENTS',
               'SHUFFLE_ENEMY_SOULS', 'SHUFFLE_NPC_SOULS', 'SHUFFLE_ANIMAL_SOULS', 'SHUFFLE_MISC_SOULS',
               'SHUFFLE_BOSS_SOULS') + tuple('SHUFFLE_' + x + '_DROPS' for x in ENV_OPTIONS)


def plain(item):
    return item.removesuffix(' (OOT)').removesuffix(' (MM)')


def scenes(game):
    host = 'soh' if game == 'oot' else 'mm'
    source = (ROOT / host / 'include/tables/scene_table.h').read_text()
    return {token: int(number, 16) for number, line in re.findall(r'/\* 0x([0-9A-Fa-f]+) \*/\s*DEFINE_SCENE\(([^\n]+)', source)
            for token in re.findall(r'\bSCENE_[A-Z0-9_]+\b', line)[:1]}


def item_rows():
    source = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    return {token: name for token, name in re.findall(r'\bRI\((RI_[A-Z0-9_]+),\s*"[^"]*",\s*"([^"]+)"', source)}


def feature_catalogue():
    """Expected identities come from production catalogues, not numeric totals."""
    rusty = {'oot': [], 'mm': []}
    for game, name in re.findall(r'^RUSTY_KEY\((OOT|MM),[^\n]*?"([^"]+)"\)',
                                 (ROOT / 'combo/rando/RustyDoorKeyCatalog.inc').read_text(), re.M):
        rusty[game.lower()].append(name)
    items = item_rows()
    parity = {name for token, name in items.items() if token in ('RI_DEKU_STICK_BAG', 'RI_DEKU_NUT_BAG')
              or token.startswith('RI_INFINITE_') or re.fullmatch(r'RI_(WOODFALL|SNOWHEAD|GREAT_BAY|STONE_TOWER)_KEY_RING', token)}
    environment = set(re.findall(r'MM_ENVIRONMENT_CHECK\((RC_[A-Z0-9_]+),',
                                 (ROOT / 'mm/2s2h/Rando/MMEnvironmentChecks.inc').read_text()))
    environment.update(re.findall(r'MM_ICICLE\((RC_[A-Z0-9_]+),',
                                 (ROOT / 'mm/2s2h/Rando/MMEnvironmentIcicles.inc').read_text()))
    # Native explicit rows, if any, join the append catalogues.
    source = (ROOT / 'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
    environment.update(re.findall(r'\bRC\((RC_[A-Z0-9_]+),\s*RCTYPE_(?:ICICLE|ROCK|BOULDER|RED_BOULDER|SOIL)\b', source))
    iron_boot_rocks = set(re.findall(r'MM_IRON_BOOT_ROCK\((RC_[A-Z0-9_]+)\)',
                                    (ROOT / 'mm/2s2h/Rando/MMEnvironmentIronBootRocks.inc').read_text()))
    require(iron_boot_rocks <= environment, 'Iron Boots source metadata outside environmental catalogue')
    soul_path = ROOT / 'combo/rando/SoulShuffleCatalog.inc'
    souls = {'oot': set(), 'mm': set()}
    for game, name in re.findall(r'^SOUL\((OOT|MM),[^\n]*?"([^"]+)"\)', soul_path.read_text(), re.M):
        souls[game.lower()].add(name)
    shared = re.findall(r'^SHARED_SOUL\([^\n]*?"([^"]+)",\s*"([^"]+)"\)',
                        (ROOT / 'combo/rando/SharedSoulCatalog.inc').read_text(), re.M)
    return {'rusty': rusty, 'parity': parity, 'environment': environment, 'ironBootRocks': iron_boot_rocks, 'souls': souls, 'shared': shared}


def settings_for(case):
    oot = {'gRandoSettings.BossKeysanity': 2, 'gRandoSettings.ShuffleGanonBossKey': 1,
           'gRandoSettings.NoLogic': 0, 'gRandoSettings.AllChecksReachable': 1,
           'gRandoSettings.ExcludedLocations': '', 'gRandoSettings.SkijerCustomItems': 0,
           'gRandoSettings.RustyDoorKeyCatalogVersion': 0}
    mm = {'gRando.Options.RO_PLACEMENT_BOSS_KEYS': 1, 'gRando.Options.RO_LOGIC': 0,
          'gRando.ExcludedChecks': [], 'gRando.StartingItems': []}
    oot.update({'gRandoSettings.' + x: 0 for x in OOT_FEATURES})
    mm.update({'gRando.Options.RO_' + x: 0 for x in MM_FEATURES})
    if case in ('rusty-keys', 'all-parity'):
        oot['gRandoSettings.LockOverworldDoors'] = 1
        oot['gRandoSettings.RustyDoorKeyCatalogVersion'] = 1
        mm['gRando.Options.RO_SHUFFLE_RUSTY_KEYS'] = 1
    if case in ('mm-parity', 'all-parity'):
        for token in ('SHUFFLE_DEKU_STICK_BAG', 'SHUFFLE_DEKU_NUT_BAG', 'SHUFFLE_CHILD_WALLET', 'INFINITE_UPGRADES', 'SHUFFLE_LOTTERY_NIGHTS'):
            mm['gRando.Options.RO_' + token] = 1
        mm['gRando.Options.RO_SHUFFLE_KEY_RINGS'] = 15
        mm['gRando.Options.RO_PLACEMENT_SMALL_KEYS'] = 1
    if case == 'all-parity':
        oot.update({'gRandoSettings.' + x: 1 for x in OOT_FEATURES})
        oot['gRandoSettings.ClockStart'] = 0
        # Real native optional checks provide capacity for all item families.
        oot['gRandoSettings.ShuffleTokens'] = 3
        oot['gRandoSettings.ShufflePots'] = 3
        oot['gRandoSettings.ShuffleFreestanding'] = 3
        for token in MM_FEATURES:
            mm['gRando.Options.RO_' + token] = 15 if token == 'SHUFFLE_KEY_RINGS' else 1
    return {'oot': oot, 'mm': mm}


def saved_setting(settings, key):
    # Production omits legacy0 to preserve old off/24-family snapshot identities.
    # An absent current1 remains a mismatch; no other setting gains a default.
    if key == 'gRandoSettings.RustyDoorKeyCatalogVersion':
        return settings.get(key, 0)
    return settings.get(key)


def module_name(game):
    stem = 'soh' if game == 'oot' else '2ship'
    return stem + '.dll' if sys.platform == 'win32' else 'lib' + stem + ('.dylib' if sys.platform == 'darwin' else '.so')


class Native:
    def __init__(self, game, solo):
        self.game = game
        self.soh = ctypes.CDLL(str(Path(module_name('oot')).resolve()), mode=ctypes.RTLD_GLOBAL)
        self.lib = self.soh
        self.bind('SOH_SetGameMode', args=(ctypes.c_int,))(1 if solo and game == 'oot' else 2 if solo else 0)
        self.bind('SOH_InitRandoHeadless')()
        if game == 'mm':
            self.lib = ctypes.CDLL(str(Path(module_name('mm')).resolve()), mode=ctypes.RTLD_GLOBAL)
            self.bind('MM_SetGameMode', args=(ctypes.c_int,))(2 if solo else 0)
            self.bind('MM_InitRandoHeadless')()
        self.prefix = 'SOH' if game == 'oot' else 'MM'

    def bind(self, name, result=None, args=()):
        fn = getattr(self.lib, name); fn.restype = result; fn.argtypes = list(args); return fn

    def call(self, suffix, *values, result=None, args=()):
        return self.bind(self.prefix + '_' + suffix, result, args)(*values)

    def dump(self, suffix):
        return json.loads(self.call(suffix, result=ctypes.c_char_p))

    def restore(self, values):
        self.call('RestoreRandoSettings', json.dumps(values).encode(), args=(ctypes.c_char_p,))


def metadata(data):
    return data['checkMetadata']


def active_checks(data):
    return {x['name'] for x in data['checks']} | {x['check'] for x in data['fixed']}


def exclusions(case, game, data):
    names = []
    for row in metadata(data).values():
        source = row['sourceName']
        if case == 'own-excluded' and ('BOSS_KEY_CHEST' in source if game == 'oot' else source.endswith(('_BOSS_KEY', '_BOSS_KEY_CHEST'))):
            names.append(source)
        if game == 'mm' and case in ('stone-normal', 'stone-inverted') and source.startswith('RC_STONE_TOWER_TEMPLE_'):
            inverted = '_INVERTED_' in source
            if inverted != (case == 'stone-inverted'):
                names.append(source)
    if game == 'mm':
        # Native GetExcludedChecksFromConfig serializes in numeric RC order.
        # Request that same order while retaining exact snapshot equality below.
        source_ids = {row['sourceName']: row['id'] for row in metadata(data).values()}
        return sorted(set(names), key=source_ids.__getitem__)
    return sorted(set(names))


def validate_keys(game, placements, data, excluded=(), orientation=None):
    rows = metadata(data); table = scenes(game); found = {}
    for key, tokens in KEY_SCENES[game].items():
        locations = [name for name, item in placements.items() if plain(item) == key]
        require(len(locations) == 1, game + ' key count differs: ' + key + ' ' + repr(locations))
        name = locations[0]
        require(name in rows, 'key has no native physical check metadata: ' + name)
        row = rows[name]
        require(row['scene'] in {table[token] for token in tokens}, key + ' outside own native dungeon: ' + name)
        require(not row.get('excluded') and row['sourceName'] not in excluded, 'key placed at excluded check: ' + name)
        if key == 'Stone Tower Boss Key' and orientation:
            require(('_INVERTED_' in row['sourceName']) == (orientation == 'stone-inverted'),
                    'Stone Tower key does not honor forced orientation: ' + row['sourceName'])
        found[key] = {'check': name, **row}
    return found


def expected_environment(case, combined, starting_items):
    catalog = feature_catalogue()
    if case != 'all-parity':
        return set()
    # Match native SourceEligibleForSeed's route policy using its exact source
    # catalogue. These presets intentionally request no starting items.
    require(starting_items == settings_for(case)['mm']['gRando.StartingItems'],
            'native configured starts changed environmental eligibility policy')
    starts_with_iron_boots = 'RI_OOT_IRON_BOOTS' in starting_items
    return catalog['environment'] if combined or starts_with_iron_boots else catalog['environment'] - catalog['ironBootRocks']


def validate_mm_wallets(enabled, placements, data, settings):
    """Child Wallet adds progressive supply; it is resolved only at native grant."""
    require(isinstance(settings, dict), 'native MM wallet settings missing')
    require(settings.get('gRando.Options.RO_SHUFFLE_CHILD_WALLET') == int(enabled),
            'native Child Wallet option changed')
    require(settings.get('gRando.Options.RO_SHUFFLE_TYCOON_WALLET') == 0,
            'native wallet contract requires the configured Tycoon-off preset')
    require(settings.get('gRando.StartingItems') == [], 'native wallet contract requires the configured empty starts')
    names = {row['token']: row['name'] for row in data['items'] if row.get('token') in
             ('RI_PROGRESSIVE_WALLET', 'RI_WALLET_CHILD')}
    require(len(names) == 2, 'native wallet item metadata missing')
    progressive, child = names['RI_PROGRESSIVE_WALLET'], names['RI_WALLET_CHILD']
    checks = (ROOT / 'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
    baseline = len(re.findall(r'^\s*RC\([^\n]+,\s*RI_PROGRESSIVE_WALLET\),\s*$', checks, re.M))
    require(baseline == 2, 'native baseline wallet checks changed')
    expected = baseline + int(enabled)
    supply = Counter([row['name'] for row in data['pool']] + [row['item'] for row in data['fixed']])
    require(supply[progressive] == expected, 'native progressive wallet supply differs: ' + str(supply[progressive]))
    require(supply[child] == 0, 'native pool/fixed contains a phantom concrete Child Wallet')
    counts = Counter(plain(item) for item in placements.values())
    require(counts[progressive] == supply[progressive], 'filled progressive wallet count differs: ' + str(counts[progressive]))
    require(counts[child] == 0, 'filled seed contains a phantom concrete Child Wallet')
    return {'progressiveWallets': expected, 'concreteChildWallets': 0}


def validate_features(case, game, placements, data, combined=False, starting_items=None, settings=None):
    catalog = feature_catalogue()
    # For combined seeds, foreign placements are disambiguated before this call.
    counts = Counter(plain(x) for x in placements.values())
    enabled_rusty = case in ('rusty-keys', 'all-parity')
    enabled_parity = case in ('mm-parity', 'all-parity')
    # Solo vanilla unshuffled checks are exported as well; new shuffle-only items
    # never belong to that vanilla complement.
    for name in catalog['rusty'][game]:
        require(counts[name] == int(enabled_rusty), game + ' rusty identity count: ' + name + ' ' + str(counts[name]))
    if game == 'mm':
        for name in catalog['parity']:
            require(counts[name] == int(enabled_parity), 'MM capacity/infinite/ring identity count: ' + name)
        wallet = validate_mm_wallets(enabled_parity, placements, data, settings)
        active_sources = {metadata(data)[name]['sourceName'] for name in active_checks(data)}
        env = active_sources & catalog['environment']
        expected_env = expected_environment(case, combined, starting_items)
        require(env == expected_env, 'actual MM environment check set differs: ' + repr(sorted(env ^ expected_env)))
        lottery = {x for x in active_sources if x.startswith('RC_CLOCK_TOWN_WEST_LOTTERY')}
        require(len(lottery) == (3 if enabled_parity else 1), 'native lottery check set differs')
    if game == 'oot':
        expected = first_drop_checks([])
        drops = {x for x in active_checks(data) if x.startswith('Global Enemy Drop: ')}
        require(drops == (expected if case == 'all-parity' else set()), 'native first-type drop checks differ')
    if case == 'all-parity' and not combined:
        # Existing boss-soul families are included through actual native metadata,
        # while the common catalogue covers newly semantic soul families.
        for name, expected in native_soul_counts(data).items():
            require(counts[name] == expected, game + ' native soul count: ' + name)
    if game == 'oot':
        require(counts['Clock'] == int(case == 'all-parity'), 'native OoT Clock identity count differs')
    if case == 'all-off':
        soul_names = catalog['souls'][game] | {row['name'] for row in data['items'] if 'Soul' in row['name']}
        require(not any(counts[x] for x in soul_names), 'disabled souls leaked into actual seed')
    return {'rustyIdentities': len(catalog['rusty'][game]) if enabled_rusty else 0,
            'walletSupply': wallet if game == 'mm' else None,
            'nativeEnvironmentChecks': len(expected_env) if game == 'mm' else 0,
            'environmentPolicy': 'combined native OoT route' if combined else 'native solo configured starts'}


def native_soul_counts(data):
    names = {row['name'] for row in data['items'] if 'Soul' in row['name']}
    return Counter([row['name'] for row in data['pool'] if row['name'] in names] +
                   [row['item'] for row in data['fixed'] if row['item'] in names])


def validate_rings(case, placements, data, foreign=()):
    if case not in ('mm-parity', 'all-parity'):
        return
    rows = metadata(data); table = scenes('mm')
    for key, tokens in KEY_SCENES['mm'].items():
        ring = key.replace('Boss Key', 'Key Ring')
        locations = [name for name, item in placements.items() if plain(item) == ring]
        require(len(locations) == 1, 'MM own-dungeon ring count differs: ' + ring)
        name = locations[0]
        require(name not in foreign and name in rows and rows[name]['scene'] in {table[x] for x in tokens},
                'MM ring outside its native dungeon: ' + ring)


def mm_replay(native, filled, data):
    prices = filled.get('prices')
    require(isinstance(prices, dict) and set(filled['placements']) <= prices.keys(),
            'native solo generation omitted authoritative per-check prices')
    require(all(isinstance(name, str) and type(value) is int and 0 <= value <= 65535
                for name, value in prices.items()), 'native solo price payload is invalid')
    reset = native.bind('Combo_MM_Rando_Reset')
    owned = native.bind('Combo_MM_Rando_SetOwnedItems', args=(ctypes.c_char_p,))
    reach = native.bind('Combo_MM_Rando_GetReachableChecks', ctypes.c_char_p)
    restore = native.bind('Combo_MM_Rando_Restore')
    native.call('SetComboRandoSeed', filled['finalSeed'], args=(ctypes.c_uint64,))
    native.call('SetCheckPrices', json.dumps(prices).encode(), args=(ctypes.c_char_p,))
    inventory = []; credited = set(); reachable = set(); spheres = []
    try:
        while True:
            reset(); owned(json.dumps(inventory).encode()); reachable = set(json.loads(reach()))
            newly = (reachable & filled['placements'].keys()) - credited
            if not newly:
                break
            spheres.append(sorted(newly)); credited.update(newly)
            inventory.extend(filled['placements'][x] for x in newly)
        goal = [name for name, row in metadata(data).items() if row['sourceName'] == 'RC_MOON_MAJORA_POT_01']
        require(len(goal) == 1 and goal[0] in reachable, 'actual native solo MM replay cannot reach Majora')
        advancement = {row['name'] for row in data['items'] if row['advancement']}
        missing = [name for name, item in filled['placements'].items()
                   if filled['checks'][name]['shuffled'] and item in advancement and name not in reachable]
        require(not missing, 'actual native MM solo advancement checks unreachable: ' + repr(missing))
        wallet_collected = 0
        if filled.get('settings', {}).get('gRando.Options.RO_SHUFFLE_CHILD_WALLET') == 1:
            wallets = [row['name'] for row in data['items'] if row.get('token') == 'RI_PROGRESSIVE_WALLET']
            require(len(wallets) == 1, 'native replay wallet metadata missing')
            wallet_collected = inventory.count(wallets[0])
            require(wallet_collected > 0, 'native replay never granted progressive Child Wallet supply')
        return {'nativeGoal': goal[0], 'spheres': spheres, 'reachableChecks': len(reachable),
                'nativeProgressiveWalletsCollected': wallet_collected}
    finally:
        restore()


def invalid_budget_probe(output):
    native = Native('oot', True)
    configured = settings_for('all-parity')['oot']
    configured.update({'gRandoSettings.SkijerCustomItems': 1, 'gRandoSettings.ShuffleEnemyDrops': 0})
    for token in ('ShuffleTokens', 'ShufflePots', 'ShuffleGrass', 'ShuffleCrates', 'ShuffleFreestanding', 'ShuffleWonderItems'):
        configured['gRandoSettings.' + token] = 0
    native.restore(configured)
    native.call('SetComboRandoSeed', SEED, args=(ctypes.c_uint64,))
    native.call('SetEnabledTricks', b'[]', args=(ctypes.c_char_p,))
    require(native.call('SetDungeonLayoutsForCombo', b'{"masterQuestDungeons":[],"dungeonLayoutAssets":1}',
                        result=ctypes.c_int, args=(ctypes.c_char_p,)) == 1, 'native Vanilla layout rejected')
    data = native.dump('DumpRandoStaticData')
    require('native OOT item pool generation failed' in data.get('error', ''),
            'native overfull configuration did not report the real budget failure: ' + repr(data.get('error')))
    require(all(data.get(key) == [] for key in ('checks', 'pool', 'fixed')),
            'failed native generation exported a partial or fallback pool')
    filled = json.loads(native.call('FillSoloRandoSeed', SEED, result=ctypes.c_char_p, args=(ctypes.c_uint32,)))
    require(not filled.get('success') and filled.get('fillResult') == -1,
            'native standalone fill did not reject its actual overfull configuration')
    write(output, {'configuredSettings': configured, 'nativeDump': data, 'nativeSolo': filled})


def probe(case, game, mode, output):
    native = Native(game, mode == 'solo')
    configured = settings_for(case)[game]
    native.restore(configured)
    native.call('SetComboRandoSeed', SEED, args=(ctypes.c_uint64,))
    if game == 'oot':
        native.call('SetEnabledTricks', b'[]', args=(ctypes.c_char_p,))
        require(native.call('SetDungeonLayoutsForCombo', b'{"masterQuestDungeons":[],"dungeonLayoutAssets":1}',
                            result=ctypes.c_int, args=(ctypes.c_char_p,)) == 1, 'native Vanilla layout rejected')
    data = native.dump('DumpRandoStaticData')
    require('error' not in data, 'native static dump failed: ' + repr(data.get('error')))
    excluded = exclusions(case, game, data)
    if case == 'own-excluded' or (game == 'mm' and case.startswith('stone-')):
        require(excluded, 'requested exclusion set empty')
    if excluded:
        configured['gRandoSettings.ExcludedLocations' if game == 'oot' else 'gRando.ExcludedChecks'] = ','.join(excluded) if game == 'oot' else excluded
        native.restore(configured); data = native.dump('DumpRandoStaticData')
    actual_settings = native.dump('DumpRandoSettings')
    for key, value in configured.items():
        require(saved_setting(actual_settings, key) == value, 'native setting did not apply: ' + key)
    require('error' not in data, 'native static dump failed after exclusions: ' + repr(data.get('error')))
    require(not data.get('accessibility', {}).get('noLogic'), 'native graph switched to No Logic')
    require(not data.get('startingSouls'), 'unexpected starting souls invalidate native replay')
    result = {'game': game, 'mode': mode, 'case': case, 'settings': actual_settings, 'dump': data, 'excluded': excluded}
    if game == 'oot': result['layouts'] = native.dump('DumpDungeonLayouts')
    if mode == 'solo':
        filled = native.call('FillSoloRandoSeed', SEED, result=ctypes.c_char_p, args=(ctypes.c_uint32,))
        filled = json.loads(filled)
        require(filled.get('success'), 'native solo generation failed: ' + repr(filled))
        for key, value in configured.items():
            require(saved_setting(filled['settings'], key) == value, 'actual solo generation changed native setting: ' + key)
        if game == 'oot':
            require(filled['fillResult'] == 1 and filled['nativeReplayBeatable'], 'native OoT solo replay failed')
        else:
            result['replay'] = mm_replay(native, filled, data)
        result['keys'] = validate_keys(game, filled['placements'], data, excluded, case if case.startswith('stone-') else None)
        result['features'] = validate_features(case, game, filled['placements'], data,
                                               starting_items=filled['settings'].get('gRando.StartingItems'), settings=filled['settings'])
        if game == 'mm': validate_rings(case, filled['placements'], data)
        result['nativeFill'] = filled
    else:
        fixed = {row['check']: row['item'] for row in data['fixed']}
        result['keys'] = validate_keys(game, fixed, data, excluded, case if case.startswith('stone-') else None)
    write(output, result)


def owner_placements(spoiler):
    """Resolve item ownership using actual foreign records, never display guesses."""
    result = {'oot': {}, 'mm': {}}
    foreign = {(row['checkGame'], row['checkName']): row for row in spoiler.get('foreign', [])}
    for game in result:
        for check, item in spoiler[game]['placements'].items():
            row = foreign.get((game, check))
            owner = row['itemGame'] if row else game
            result[owner][game + ':' + check] = row['itemName'] if row else item
    return result


def validate_combined(case, spoiler, probes):
    result = {}; owned = owner_placements(spoiler)
    all_values = [plain(x) for p in owned.values() for x in p.values()]
    for game in ('oot', 'mm'):
        require(not spoiler[game].get('enabledTricks'), 'native combined seed enabled tricks')
        expected_settings = settings_for(case)[game]
        exclusion_key = 'gRandoSettings.ExcludedLocations' if game == 'oot' else 'gRando.ExcludedChecks'
        expected_settings[exclusion_key] = probes[game]['settings'][exclusion_key]
        for key, value in expected_settings.items():
            require(saved_setting(spoiler[game]['settings'], key) == value, 'actual combined generation changed native setting: ' + key)
        result[game] = validate_keys(game, spoiler[game]['placements'], probes[game]['dump'], probes[game]['excluded'],
                                     case if case.startswith('stone-') else None)
        for row in spoiler.get('foreign', []):
            require(plain(row['itemName']) not in KEY_SCENES['oot'] and plain(row['itemName']) not in KEY_SCENES['mm'], 'own dungeon boss key exported foreign')
        # Feature counts consume the real owner across both hosts.
        validate_features(case, game, owned[game], probes[game]['dump'], combined=True,
                          starting_items=probes[game]['settings'].get('gRando.StartingItems'), settings=spoiler[game]['settings'])
    validate_rings(case, spoiler['mm']['placements'], probes['mm']['dump'],
                   {row['checkName'] for row in spoiler.get('foreign', []) if row['checkGame'] == 'mm'})
    require(len(spoiler.get('foreign', [])) > 0, 'actual combined fill has no cross-game placements')
    if case == 'all-parity':
        catalog = feature_catalogue()
        # Merge actual semantic counterpart edges before counting. Many donor
        # aliases share one soul, so edge count is never the pool count.
        groups = []
        for oot, mm in catalog['shared']:
            edge = {('oot', oot), ('mm', mm)}
            touching = [group for group in groups if group & edge]
            for group in touching: edge |= group; groups.remove(group)
            groups.append(edge)
        souls = {(g, name): count for g in ('oot', 'mm')
                 for name, count in native_soul_counts(probes[g]['dump']).items()}
        for group in groups:
            names = {name for _, name in group}
            require(sum(all_values.count(name) for name in names) == 1, 'shared soul component not deduplicated: ' + repr(sorted(group)))
            for identity in group: souls.pop(identity, None)
        for (game, name), count in souls.items():
            require(sum(plain(x) == name for x in owned[game].values()) == count, 'unshared native soul identity lost: ' + game + ':' + name)
        expected_keys = {'sharedSoulsEnemy', 'sharedSoulsNpc', 'sharedSoulsAnimal', 'sharedSoulsMisc'}
        require(expected_keys <= set(spoiler.get('sharedItems', [])), 'consolidated seed omitted shared soul masks')
        result['sharedSoulComponents'] = len(groups)
    return result


def negative_replay(driver, work, folder, spoiler, probes, timeout):
    altered = json.loads(json.dumps(spoiler))
    placements = altered['mm']['placements']
    key_check = next(name for name, item in placements.items() if plain(item) == 'Stone Tower Boss Key')
    goals = [name for name, row in metadata(probes['mm']['dump']).items()
             if row['sourceName'] == 'RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP']
    require(len(goals) == 1 and goals[0] in placements, 'native Stone Tower boss-warp placement absent')
    goal = goals[0]
    foreign = {(row['checkGame'], row['checkName']) for row in altered.get('foreign', [])}
    require(('mm', key_check) not in foreign and ('mm', goal) not in foreign and key_check != goal,
            'circular negative needs two genuine native MM placements')
    placements[key_check], placements[goal] = placements[goal], placements[key_check]
    path = folder / 'circular-negative.json'; write(path, altered)
    with (folder / 'circular-negative.log').open('w') as stream:
        result = subprocess.run([str(driver), '--playthrough', str(path)], cwd=work,
                                env={**os.environ, 'SHIP_HOME': str(work)}, stdout=stream,
                                stderr=subprocess.STDOUT, timeout=timeout)
    require(result.returncode in (1, 3), 'native replay accepted circular boss-key seed or failed setup: ' + str(result.returncode))
    return result.returncode


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime-dir', type=Path); p.add_argument('--driver', type=Path); p.add_argument('--output', type=Path, required=True)
    p.add_argument('--timeout', type=int, default=1800)
    p.add_argument('--case', action='append', choices=CASE_NAMES)
    p.add_argument('--probe-invalid-budget', action='store_true', help=argparse.SUPPRESS)
    p.add_argument('--probe-case', choices=CASE_NAMES, help=argparse.SUPPRESS)
    p.add_argument('--probe-game', choices=('oot', 'mm'), help=argparse.SUPPRESS)
    p.add_argument('--probe-mode', choices=('solo', 'combined'), help=argparse.SUPPRESS)
    args = p.parse_args()
    if args.probe_invalid_budget:
        invalid_budget_probe(args.output); return
    if args.probe_case:
        probe(args.probe_case, args.probe_game, args.probe_mode, args.output); return
    require(args.runtime_dir and args.driver, '--runtime-dir and --driver are required')
    out = args.output.expanduser().absolute(); require_fresh_output(out)
    runtime = args.runtime_dir.resolve(); driver = args.driver.resolve()
    require(driver.is_file(), 'actual comborando executable missing')
    for game in ('oot', 'mm'): require((runtime / module_name(game)).is_file(), 'actual module missing: ' + game)
    feature_catalogue()  # Fail missing source metadata before any expensive work.
    work = prepare_runtime(runtime, out)
    manifest = {'driverSha256': hashlib.sha256(driver.read_bytes()).hexdigest(), 'moduleSha256': module_hashes(work),
                'seed': SEED, 'cases': [], 'proof': 'actual linked native solo and combined fills/replays'}
    run([sys.executable, str(Path(__file__).resolve()), '--probe-invalid-budget',
         '--output', str(out / 'invalid-budget.json')], work, out / 'invalid-budget.log', args.timeout)
    manifest['nativeOverfullBudgetRejected'] = 'PASS'
    for case in args.case or CASE_NAMES:
        folder = out / case; folder.mkdir(); combined = {}
        for game in ('oot', 'mm'):
            for mode in ('solo', 'combined'):
                path = folder / (game + '-' + mode + '.json')
                run([sys.executable, str(Path(__file__).resolve()), '--probe-case', case, '--probe-game', game,
                     '--probe-mode', mode, '--output', str(path)], work, path.with_suffix('.log'), args.timeout)
                if mode == 'combined': combined[game] = json.loads(path.read_text())
        settings = {g: {'settings': combined[g]['settings']} for g in ('oot', 'mm')}
        settings['oot'].update(enabledTricks=[], **combined['oot']['layouts'])
        shared = ['sharedSoulsEnemy', 'sharedSoulsNpc', 'sharedSoulsAnimal', 'sharedSoulsMisc'] if case == 'all-parity' else []
        settings['sharedItems'] = shared
        settings_file = folder / 'consolidated-settings.json'; write(settings_file, settings)
        native_output = work / 'saves/combo/comborando.spoiler.json'
        if native_output.exists(): native_output.unlink()
        command = [str(driver), '--settings', str(settings_file), '--master-seed', str(SEED), '--count', '1',
                   '--starting-game', 'oot', '--shared-items', ','.join(shared) if shared else 'none', '--triforce-required', '0']
        for flag in ('--cross-dungeon-entrances', '--cross-boss-entrances', '--expanded-dungeon-pool',
                     '--cross-interior-entrances', '--cross-grotto-entrances', '--cross-shop-entrances', '--cross-door-interior-entrances'):
            command += [flag, '0']
        run(command, work, folder / 'generate.log', args.timeout)
        require(native_output.is_file(), 'native driver did not write consolidated spoiler')
        saved = folder / 'spoiler.json'; shutil.copy2(native_output, saved); spoiler = json.loads(saved.read_text())
        keys = validate_combined(case, spoiler, combined)
        run([str(driver), '--playthrough', str(saved)], work, folder / 'playthrough.log', args.timeout)
        result = {'name': case, 'bothSoloNativeFillReplay': 'PASS', 'combinedNativeFillReplay': 'PASS',
                  'foreignPlacements': len(spoiler.get('foreign', [])), 'physicalKeys': keys}
        if case == 'own': result['circularNegativeExit'] = negative_replay(driver, work, folder, spoiler, combined, args.timeout)
        manifest['cases'].append(result); write(out / 'manifest.json', manifest)
    print('PASS actual native both-solo + combined Glitchless fill/replay:', len(manifest['cases']), 'cases')


if __name__ == '__main__': main()
