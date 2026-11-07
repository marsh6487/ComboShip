#!/usr/bin/env python3
"""Replay native Saria scene-entry, init and talk/update bodies with real types.

Actor allocation, resource/animation services and player talk-button input are
boundaries. The controlled actor row is not proof of a user's active scene pack.
"""
import os
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from, functions
from runtime_hint_boundary import fixture as runtime_hint_fixture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--actor-source', type=Path, default=ROOT / 'soh/src/overlays/actors/ovl_En_Sa/z_en_sa.c')
    args = parser.parse_args()
    actor_source = args.actor_source.read_text()
    actor_helpers = (ROOT / 'soh/src/code/z_actor.c').read_text()
    declarations = actor_source[actor_source.index('typedef enum {'):actor_source.index('const ActorInit')]
    declarations += actor_source[actor_source.index('static ColliderCylinderInit'):actor_source.index('s16 func_80AF5560(')]
    actor_names = ('func_80AF5560', 'EnSa_GetTextId', 'EnSa_UpdateTalkState', 'func_80AF57D8',
                   'EnSa_ChangeAnim', 'func_80AF5DFC', 'EnSa_Init', 'func_80AF6448',
                   'func_80AF67D0', 'func_80AF683C', 'EnSa_Update')
    helper_names = ('Actor_SpawnEntry', 'Flags_GetEventChkInf', 'Flags_SetEventChkInf',
                    'Flags_GetInfTable', 'Flags_SetInfTable', 'Actor_ProcessTalkRequest',
                    'Actor_OfferTalkExchange', 'Actor_OfferTalkExchangeEquiCylinder',
                    'Actor_OfferTalk', 'Npc_UpdateTalking')
    actor_functions, helper_functions = functions(actor_source), functions(actor_helpers)
    body = declarations + '\n'
    body += '\n'.join(helper_functions[name] for name in helper_names)
    body += '\n' + '\n'.join(actor_functions[name] for name in actor_names)
    fixture = (ROOT / 'tests/saria_magic/spawn_interaction_test.c').read_text()
    fixture = fixture.replace('/* PRODUCTION */', body)
    includes = ('.', 'soh', 'soh/soh', 'soh/include', 'soh/include/PR', 'soh/assets',
                'soh/src', 'libultraship/include', 'combo')
    with tempfile.TemporaryDirectory(prefix='saria-native-spawn-') as directory:
        source, binary = Path(directory) / 'test.c', Path(directory) / 'test'
        native_object, runtime_source = Path(directory) / 'native.o', Path(directory) / 'runtime.cpp'
        source.write_text(fixture)
        runtime_source.write_text(runtime_hint_fixture())
        subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-w',
                        '-Wno-error=incompatible-pointer-types', '-Wno-error=int-conversion',
                        '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-g', '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', '-no-pie',
                        *['-I' + str(ROOT / path) for path in includes],
                        '-c', str(source), '-o', str(native_object)], check=True)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-g', '-DCOMBO_BUILD',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                        '-I' + str(ROOT / 'combo'), '-I' + os.environ.get('NLOHMANN_JSON_INCLUDE', '/usr/include'),
                        str(runtime_source), str(native_object), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
