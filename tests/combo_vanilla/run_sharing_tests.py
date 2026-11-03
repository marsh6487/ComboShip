#!/usr/bin/env python3
"""Compile the real Vanilla sharing policy with both native NEI save structures."""
import os
from pathlib import Path
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
        if game == 'mm':
            conversions = (ROOT / 'mm/2s2h/BenJsonConversions.hpp').read_text()
            snippets += [extract(conversions, 'inline void to_json(json& j, const NeiSaveData& n)'),
                         extract(conversions, 'inline void from_json(const json& j, NeiSaveData& n)')]
            native_nei = (ROOT / 'mm/mods/nei_save.cpp').read_text()
            snippets += [extract(native_nei, 'extern "C" uint8_t Nei_HookshotLevel('),
                         extract(native_nei, 'extern "C" uint8_t Nei_HookshotVariant(')]
        if game == 'soh':
            forms = (ROOT / 'soh/mods/transformation_masks/custom_forms.cpp').read_text()
            snippets.append(extract(forms, 'extern "C" void RitoItem_NoteCellItem('))
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
                        str(ROOT / 'tests/combo_vanilla/sharing_test.cpp'), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
