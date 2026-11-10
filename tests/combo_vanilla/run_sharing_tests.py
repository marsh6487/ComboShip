#!/usr/bin/env python3
"""Compile the real Vanilla sharing policy with both native NEI save structures."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
flags = ['-DCOMBO_BUILD', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
if '--sanitize' in sys.argv:
    flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
def extract(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

for game in ('soh', 'mm'):
    with tempfile.TemporaryDirectory(prefix='vanilla-sharing-' + game + '-') as tmp:
        exe = Path(tmp) / 'sharing'
        sync_path = ROOT / game / ('soh' if game == 'soh' else '2s2h') / 'FleetShipCombo/FleetSync.cpp'
        sync = sync_path.read_text()
        snippets = []
        objects = []
        if game == 'mm':
            inventory = (ROOT / 'mm/mods/extended_inventory.c').read_text()
            snippets.append(extract(inventory, 'void RitoMask_Grant('))
            conversions = (ROOT / 'mm/2s2h/BenJsonConversions.hpp').read_text()
            snippets += [extract(conversions, 'inline void to_json(json& j, const NeiSaveData& n)'),
                         extract(conversions, 'inline void from_json(const json& j, NeiSaveData& n)')]
            native_nei = (ROOT / 'mm/mods/nei_save.cpp').read_text()
            snippets += [extract(native_nei, 'extern "C" uint8_t Nei_HookshotLevel('),
                         extract(native_nei, 'extern "C" uint8_t Nei_HookshotVariant(')]
        if game == 'soh':
            # The load/mask migration calls the unified trade helpers. They are
            # C in production; keep their native enum rules and strict warnings.
            trade = (ROOT / 'soh/mods/items/logic/trade_items.c').read_text()
            trade = trade[trade.index('#define TRADE_ADULT_COUNT'):trade.index('extern void* MmAssets_LoadResource')]
            declarations = re.findall(r'^(?:s32|u8|void) \w+\([^{};]*\) (?=\{)', trade, re.M)
            snippets.append('extern "C" {\n' + ';\n'.join(declarations) + ';\n}')
            equipment = (ROOT / 'soh/mods/extended_equipment.h').read_text()
            trade_source = ('#include "soh/include/z64item.h"\n#include "soh/mods/nei_save.h"\n'
                            'typedef uint8_t u8;\ntypedef int32_t s32;\n' +
                            next(line for line in equipment.splitlines() if line.startswith('#define ITEM_EXT_BOOTS_2 ')) +
                            '\n' + trade)
            (Path(tmp) / 'trade.c').write_text(trade_source)
            cflags = [flag for flag in flags if not flag.startswith('-std=')]
            obj = Path(tmp) / 'trade.o'
            subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), '-std=gnu11', *cflags,
                            '-I', str(ROOT / 'soh'),
                            '-c', str(Path(tmp) / 'trade.c'), '-o', str(obj)], check=True)
            objects.append(str(obj))
            forms = (ROOT / 'soh/mods/transformation_masks/custom_forms.cpp').read_text()
            for signature in ('extern "C" uint8_t RitoMask_IsOwned(',
                              'extern "C" void RitoItem_NoteCellItem(',
                              'static uint8_t OotMaskWheel_OwnsItem(',
                              'extern "C" uint8_t OotMaskWheel_Neighbor(',
                              'extern "C" void RitoMask_MigrateLegacy(',
                              'extern "C" void RitoMask_Grant('):
                snippets.append(extract(forms, signature))
            player = (ROOT / 'soh/mods/extended_player.c').read_text()
            snippets.append(extract(player, 'u8 Nei_ArmsHookVariant('))
            feather = (ROOT / 'soh/soh/Enhancements/randomizer/RocsFeatherCycle.c').read_text()
            snippets.append(extract(feather, 'void VanillaRocsFeather_Grant('))
        snippets += [extract(sync, 'static void ExtractVanillaShared('), extract(sync, 'static void ApplyVanillaShared('),
                     extract(sync, 'void NetSend('), extract(sync, 'uint64_t NetHash(')]
        hash_branch = extract(sync, 'else if (op == "hash") {')
        snippets.append('static void HandleHash(const nlohmann::json& p) ' + hash_branch[hash_branch.index('{'):])
        # Exercise the actual routing guards. Native/Randomizer bodies after the
        # boundary are already covered by their existing regression suites.
        for signature in ('void ExtractShared(', 'void ApplyShared(', 'void ApplyRegistryToNatives()',
                          'void ApplyFcRegistryToNatives()', 'void ExtractEquips(', 'void ApplyEquips('):
            function = extract(sync, signature)
            pos = function.index('    if (Combo_IsVanilla()) {')
            guard = extract(function[pos:], '    if (Combo_IsVanilla()) {')
            unused = '    (void)sh;\n' if 'Equips(' in signature else ''
            snippets.append(function[:pos] + guard + '\n' + unused + '    ++legacyCalls;\n}')
        shared = sync_path.with_name('FleetSharedItems.cpp').read_text()
        # Keep the actual soul receipt/reconciliation wrappers and complete
        # shared runtime. The fixture supplies only native flag/grant services.
        soul_path = ROOT / ('soh/soh/Enhancements/randomizer/SoulShuffleShared.cpp' if game == 'soh'
                            else 'mm/2s2h/Rando/SoulShuffleShared.cpp')
        soul = soul_path.read_text()
        prefix = 'SOH' if game == 'soh' else 'MM'
        snippets += [extract(soul, 'extern "C" int ' + prefix + '_ShareSoulReceipt('),
                     extract(soul, 'extern "C" void ' + prefix + '_ApplySharedSoulState(')]
        fanout = extract(shared, 'extern "C" void FleetShared_OnNativeObtained(')
        boundary = fanout.index('    // Share the concrete' if game == 'mm' else '    // Native magic')
        snippets.append(fanout[:boundary] + '    (void)nativeId; ++legacyFanouts;\n}')
        snippets.append('typedef const char* (*FnExtractSharedState)(void);\n' +
                        extract(shared, 'static void PullFromPeerWhenReady()'))
        globals_path = ROOT / ('soh/soh/OTRGlobals.cpp' if game == 'soh' else 'mm/2s2h/BenPort.cpp')
        snippets.append(extract(globals_path.read_text(), 'extern "C" COMBO_EXPORT void ' +
                                ('SOH' if game == 'soh' else 'MM') + '_GrantSharedItem('))
        (Path(tmp) / 'native_adapter.inc').write_text('\n'.join(snippets))
        includes = ['-I', tmp, '-I', str(ROOT / game / 'include'), '-I', str(ROOT / game), '-I', str(ROOT / game / ('soh' if game == 'soh' else '2s2h'))]
        subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, *includes,
                        *(['-DTEST_MM'] if game == 'mm' else []),
                        str(ROOT / 'tests/combo_vanilla/sharing_test.cpp'), *objects, '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
