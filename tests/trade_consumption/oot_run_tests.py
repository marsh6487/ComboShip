#!/usr/bin/env python3
"""Compile actual OoT NPC transitions and grant/hook branches against engine boundaries.

The source bodies are extracted unchanged and compiled as C, like the NPCs.
Rendering, dialogue input, queued receipt delivery and the trade-state API are
controlled boundaries. This tests transaction timing/identity, not game runtime
or the separately tested trade revision/save/sync implementation.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, opening):
    depth, pos = 1, opening + 1
    while depth:
        if source.startswith('//', pos):
            pos = source.index('\n', pos) + 1
            continue
        if source.startswith('/*', pos):
            pos = source.index('*/', pos + 2) + 2
            continue
        if source[pos] in ('"', "'"):
            quote = source[pos]
            pos += 1
            while source[pos] != quote:
                pos += 2 if source[pos] == '\\' else 1
            pos += 1
            continue
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[opening:pos]


def function(source, name):
    match = re.search(r'^(?:static\s+)?(?:void|bool|u8|s32)\s+' + re.escape(name) +
                      r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, name
    return source[match.start():match.end() - 1] + block(source, match.end() - 1)


def read(path):
    return (ROOT / path).read_text()


def native_source():
    actors = {
        'En_Niw_Lady': ('func_80ABAC84', 'func_80ABAB08'),
        'En_Hs': ('func_80A6E740', 'func_80A6E7BC'),
        'En_Ds': ('EnDs_GiveOddPotion', 'EnDs_OfferOddPotion'),
        'En_Ko': ('func_80A99504',),
        'En_Toryo': ('EnToryo_HandleTalking',),
        'En_Kz': ('EnKz_SetupGetItem',),
        'En_Mk': ('func_80AACA94',),
        'En_Go2': ('EnGo2_SetGetItem',),
        'En_Ta': ('EnTa_SetupAwake', 'EnTa_IdleAsleepInCastle', 'EnTa_IdleAsleepInKakariko'),
        'En_Heishi2': ('func_80A54038',),
    }
    result = []
    for actor, names in actors.items():
        source = read(f'soh/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c')
        result.extend(function(source, name) for name in names)
    parameter = read('soh/src/code/z_parameter.c')
    result.append(function(parameter, 'Inventory_DeleteItem'))
    give = function(parameter, 'ItemGrantAudit_ItemGive')
    marker = 'if ((item >= ITEM_WEIRD_EGG) && (item <= ITEM_CLAIM_CHECK)) {'
    start = give.index(marker)
    opening = start + len(marker) - 1
    result.append('u8 Probe_NativeGrant(PlayState* play, u8 item) { s16 i; s32 temp;\n' +
                  give[start:opening] + block(give, opening) + '\nreturn ITEM_NONE;\n}')
    return '\n\n'.join(result)


def rando_source():
    source = read('soh/soh/Enhancements/randomizer/hook_handlers.cpp')
    behavior = function(source, 'RandomizerOnVanillaBehaviorHandler')
    cases = []
    for name in ('VB_TRADE_POCKET_CUCCO', 'VB_TRADE_COJIRO', 'VB_TRADE_ODD_MUSHROOM',
                 'VB_TRADE_ODD_POTION', 'VB_TRADE_SAW', 'VB_ADULT_KING_ZORA_ITEM_GIVE',
                 'VB_TRADE_FROG', 'VB_TRADE_TIMER_EYEDROPS'):
        marker = f'case {name}: {{'
        start = behavior.index(marker)
        opening = start + len(marker) - 1
        cases.append(behavior[start:opening] + block(behavior, opening))
    dispatch = ('static void Probe_RandoBehavior(int id, bool* should, va_list args) {\n'
                'switch (id) {\n' + '\n'.join(cases) + '\ndefault: break;\n}\n}')
    flags = function(source, 'RandomizerOnFlagSetHandler')
    # Whole existing adult-trade consumption region, before unrelated trigger/queue code.
    flags = flags[:flags.index('    if (flagType == FLAG_EVENT_CHECK_INF')] + '}\n'
    grants = read('soh/soh/Enhancements/randomizer/randomizer.cpp')
    child = []
    for name in ('RG_WEIRD_EGG', 'RG_ZELDAS_LETTER'):
        marker = f'if (item == {name}) {{'
        start = grants.index(marker, grants.index('extern "C" u16 Randomizer_Item_Give'))
        opening = start + len(marker) - 1
        child.append(grants[start:opening] + block(grants, opening))
    return flags + dispatch + '\nstatic void Probe_RandoGrant(int item) {\n' + '\n'.join(child) + '\n}'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    template = read('tests/trade_consumption/oot_test.c')
    source = template.replace('/* PRODUCTION_RANDO */', rando_source())
    source = source.replace('/* PRODUCTION_NATIVE */', native_source())
    with tempfile.TemporaryDirectory(prefix='oot-trade-consumption-') as temp:
        out = Path(temp)
        (out / 'test.c').write_text(source)
        flags = ['-std=c2x', '-Wall', '-Wextra', '-Werror=implicit-function-declaration',
                 '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-parameter',
                 '-Wno-sign-compare', '-Wno-parentheses', '-I' + str(ROOT)]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        subprocess.run([os.environ.get('CC', 'cc'), *flags, str(out / 'test.c'), '-lm',
                        '-o', str(out / 'test')], check=True)
        subprocess.run([str(out / 'test')], check=True)


if __name__ == '__main__':
    main()
