#!/usr/bin/env python3
"""Run the production child-horse entry/summon code against engine boundaries."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
from run_time_pedestal_tests import block_from, functions

ROOT = Path(__file__).resolve().parents[2]
source = functions((ROOT / 'soh/src/code/z_horse.c').read_text())
names = ('Horse_CanSpawn', 'Horse_CanSpawnYoung', 'Horse_YoungEponaAssetsAvailable', 'Horse_CanUseYoungEpona',
         'Horse_FindYoungEpona', 'Horse_GetActorSaveData', 'Horse_SaveYoungEpona',
         'Horse_SpawnYoungEpona', 'Horse_TrySummonYoungEpona', 'Horse_InitPlayerHorse')
fixture = (ROOT / 'soh/tests/young_epona_test.c').read_text().replace(
    '/* PRODUCTION_HORSE */', '\n'.join(source[n] for n in names if n in source))
npc_source = functions((ROOT / 'soh/src/overlays/actors/ovl_En_Horse_Link_Child/z_en_horse_link_child.c').read_text())
fixture = fixture.replace('/* PRODUCTION_RANCH_UPDATE */', npc_source['EnHorseLinkChild_Update'])
scene_source = (ROOT / 'soh/soh/unbound/SceneDB.cpp').read_text()
seed_start = scene_source.index('struct VanillaScene {')
scene_bridge = '#include "soh/unbound/SceneDB.h"\n#include <algorithm>\n'
scene_bridge += scene_source[seed_start:scene_source.index('struct VanillaEntrance {', seed_start)]
# Entrance registration and mounted resource I/O are outside this horse fixture.
# Execute the real vanilla scene seeds, lookup, and horse gate at its new boundary.
scene_bridge += '\nSceneDB::SceneDB() { SeedVanillaScenes(); }\n'
scene_bridge += 'SceneDB* SceneDB::Instance = new SceneDB();\n'
for signature in ('void SceneDB::SeedVanillaScenes()', 'SceneDB::Entry& SceneDB::RetrieveEntry(int32_t id)',
                  'extern "C" int32_t SceneDB_HorseAllowed(int32_t id)'):
    scene_bridge += block_from(scene_source, scene_source.index(signature)) + '\n'
flags = ['-std=gnu2x', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-Icombo', '-Icombo/menu', '-DLOG_LEVEL_GAME_PRINTS=0', '-DNDEBUG',
         '-Werror=implicit-function-declaration', '-Wno-int-conversion',
         '-Wno-incompatible-pointer-types', '-Wno-discarded-qualifiers',
         '-Wno-discarded-array-qualifiers',
         '-Ilibultraship/include', '-Isoh/include', '-Isoh/src', '-Isoh/assets', '-Isoh', '-Isoh/mods',
         '-fsanitize=undefined', '-fno-sanitize-recover=all']
for path in ('CMake/soh-cvars.cmake', 'CMake/lus-cvars.cmake'):
    for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT/path).read_text()):
        flags.append(f'-D{key}="{value}"')
with tempfile.TemporaryDirectory(prefix='young-epona-test-') as folder:
    cfile, binary = Path(folder)/'test.c', Path(folder)/'test'
    scene_file, fixture_object = Path(folder)/'scene_bridge.cpp', Path(folder)/'test.o'
    cfile.write_text(fixture)
    scene_file.write_text(scene_bridge)
    subprocess.run([os.environ.get('CC', 'cc'), *flags, '-c', str(cfile), '-o', str(fixture_object)], cwd=ROOT, check=True)
    cpp_flags = ['-std=c++20', *(flag for flag in flags[1:] if not flag.startswith('-W'))]
    subprocess.run([os.environ.get('CXX', 'c++'), *cpp_flags, str(scene_file), str(fixture_object),
                    '-lm', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
    if 'Horse_CanUseYoungEpona' in source:
        for path in ('soh/src/code/z_horse.c', 'soh/src/code/z_message_PAL.c',
                     'soh/src/overlays/actors/ovl_En_Horse/z_en_horse.c',
                     'soh/src/overlays/actors/ovl_En_Horse_Link_Child/z_en_horse_link_child.c',
                     'soh/src/overlays/actors/ovl_player_actor/z_player.c'):
            subprocess.run([os.environ.get('CC', 'cc'), *flags, '-fsyntax-only', path], cwd=ROOT, check=True)
        print('PASS: all five affected C translation units compile')
