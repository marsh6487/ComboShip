#!/usr/bin/env python3
"""Read-only production timing simulation; no engine pacing is modified."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    cursor, depth = brace + 1, 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]

queue = (ROOT/'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
events = (ROOT/'mm/2s2h/GameInteractor/GameInteractor.cpp').read_text()
items = (ROOT/'mm/2s2h/CustomItem/CustomItem.cpp').read_text()
native = (ROOT/'mm/src/code/z_player_lib.c').read_text()
selectors = (ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
initial = (ROOT/'mm/2s2h/Rando/Logic/SeedGeneration.cpp').read_text()
producer = (ROOT/'mm/2s2h/Rando/MiscBehavior/OnFileCreate.cpp').read_text()
player = (ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
production = 'static bool queued=false; static RandoCheckId queuedCheck=RC_UNKNOWN; static ActorFunc sCheckQueueGive=nullptr;\n'
production += '\n'.join(function(queue, name) for name in ('void Rando::MiscBehavior::CheckQueue()',
                                                          'void Rando::MiscBehavior::CheckQueueReset()'))
production += '\nnamespace Rando::StaticData {\n' + function(selectors, 'bool ShouldShowGetItemCutscene(') + '\n}\n'
production += '\nnamespace Rando {\n' + function(initial, 'void PrepareNewRandomizedSave()') + '\n}\n'
production += '\nvoid NativeStartingChecksEligible() {\n' + '\n'.join(line for line in producer.splitlines() if 'RANDO_SAVE_CHECKS[RC_STARTING_ITEM_' in line and '.eligible = true;' in line) + '\n}\n'
production += '\n' + function(player, 's32 func_8083A878(').replace('this', 'self')
production += '\n' + function(player, 'void Player_StartMode_D(').replace('this', 'self')
production += '\n' + function(native, 'bool Player_InBlockingCsMode(')
production += '\n' + function(events, 'void ProcessEvents(')
start = items.index('static ColliderCylinderInit')
production += '\n' + items[start:items.index('// #endregion', start)]
production += '\n' + '\n'.join(function(items, name) for name in (
    'EnItem00* CustomItem::Spawn(', 'void CustomItem00_Init(',
    'void CustomItem00_ItemTouched(', 'void CustomItem00_Update('))

with tempfile.TemporaryDirectory(prefix='mm-rando-timing-') as directory:
    output = Path(directory)
    (output/'production.inc').write_text(production)
    includes = ('', 'mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets',
                'libultraship/include','combo','combo/menu')
    binary = output/'timing'
    command = [*shlex.split(os.environ.get('CXX','c++')), '-std=c++20','-w','-DF3DEX_GBI_2',
               '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
               *['-I'+str(ROOT/path) for path in includes], '-I'+str(output)]
    if '--sanitize' in sys.argv:
        command += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    for combo in (False, True):
        print('MODE: '+('Combo' if combo else 'standalone'), flush=True)
        subprocess.run(command+(['-DCOMBO_BUILD'] if combo else [])+[str(HERE/'native_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
