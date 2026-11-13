#!/usr/bin/env python3
"""Exercise the production NEI serializers with native save types and real JSON.

OoT's complete NeiSave_Save/NeiSave_Load functions use a controlled SaveManager
JSON boundary. MM's complete NeiSaveData to_json/from_json functions run directly.
Unrelated sidecar, migration, audit and RPG callbacks are outside this test.
Temporary translation units and binaries are created outside the checkout.
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


def function(source, signature):
    match = re.search(signature + r'\s*\{', source, re.M)
    assert match, signature
    return source[match.start():match.end() - 1] + block(source, match.end() - 1)


def production(game, ref):
    path = 'soh/mods/nei_save.cpp' if game == 'oot' else 'mm/2s2h/BenJsonConversions.hpp'
    source = subprocess.check_output(['git', 'show', ref + ':' + path], cwd=ROOT, text=True) \
        if ref else (ROOT / path).read_text()
    if game == 'oot':
        signatures = (r'^void NeiSave_Save\([^;{}]*\)', r'^void NeiSave_Load\(\)')
    else:
        signatures = (r'^inline void to_json\(json& j, const NeiSaveData& n\)',
                      r'^inline void from_json\(const json& j, NeiSaveData& n\)')
    return '\n\n'.join(function(source, signature) for signature in signatures)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--game', choices=('oot', 'mm', 'both'), default='both')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--source-ref', help='Extract the serializers from a git ref for regression verification')
    args = parser.parse_args()
    json_include = Path(os.environ.get('NLOHMANN_JSON_INCLUDE', '/usr/include'))
    if not (json_include / 'nlohmann/json.hpp').is_file():
        parser.error('Set NLOHMANN_JSON_INCLUDE to a directory containing nlohmann/json.hpp')
    fixture = (ROOT / 'tests/trade_consumption/persistence_test.cpp').read_text()
    games = ('oot', 'mm') if args.game == 'both' else (args.game,)
    with tempfile.TemporaryDirectory(prefix='trade-consumption-persistence-') as temporary:
        out = Path(temporary)
        for game in games:
            tree = 'soh' if game == 'oot' else 'mm'
            source = out / (game + '.cpp')
            source.write_text(fixture.replace('// PRODUCTION_SERIALIZERS', production(game, args.source_ref)))
            includes = [tree, tree + '/include', tree + '/include/PR', tree + '/src', tree + '/assets',
                        tree + '/mods', tree + ('/soh' if game == 'oot' else '/2s2h'), 'libultraship/include']
            flags = ['-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
                     '-DSOH_BUILD_DLL' if game == 'oot' else '-DMM_BUILD_DLL']
            if game == 'mm':
                flags.append('-DPERSISTENCE_MM')
            if args.sanitize:
                flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
            binary = out / game
            command = [os.environ.get('CXX', 'g++-14'), '-std=c++20', '-Wall', '-Wextra',
                       '-Wno-unused-function', '-Wno-unused-parameter', '-Wno-sign-compare',
                       *flags, '-I' + str(json_include), *['-I' + str(ROOT / path) for path in includes],
                       str(source), '-o', str(binary)]
            print('BUILD persistence', game, flush=True)
            subprocess.run(command, check=True)
            subprocess.run([str(binary)], check=True, cwd=out)


if __name__ == '__main__':
    main()
