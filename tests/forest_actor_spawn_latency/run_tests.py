#!/usr/bin/env python3
"""Run the forest spawners through real MM hook, main/draw and fade ordering.

Native Play/Player/Actor layouts, GameInteractor registration/dispatch,
GameState_Update, Play_Main, fade arithmetic and Player_InCsMode execute.
Room readiness and entrance movement are controlled inputs. Actor allocation,
collision queries, resource lookup, unrelated update systems and GPU drawing
are boundaries; this is a frame-sequencing regression, not a full game run.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--case', choices=('all', 'incoming', 'idle', 'guards', 'field'), default='all')
parser.add_argument('--source-ref', help='Replay a committed ComboSaria.cpp against the current native engine')
args = parser.parse_args()


def function(code, signature):
    start = code.index(signature)
    cursor = code.index('{', start) + 1
    depth = 1
    while depth:
        depth += (code[cursor] == '{') - (code[cursor] == '}')
        cursor += 1
    return code[start:cursor]


actor_path = 'mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp'
actor = (subprocess.check_output(['git', 'show', args.source_ref + ':' + actor_path], cwd=ROOT, text=True)
         if args.source_ref else (ROOT / actor_path).read_text())
game = (ROOT / 'mm/src/code/game.c').read_text()
play = (ROOT / 'mm/src/code/z_play.c').read_text()
player = (ROOT / 'mm/src/code/z_player_lib.c').read_text()
fade = (ROOT / 'mm/src/code/z_fbdemo_fade.c').read_text()
library = (ROOT / 'mm/src/code/z_lib.c').read_text()
hooks = (ROOT / 'mm/2s2h/GameInteractor/GameInteractor.cpp').read_text()

production = '\n'.join(re.findall(r'^(?:constexpr (?:u16|s16) k\w+ = [^;]+;|bool s(?:SpawnAttempted|HorseSpawnAttempted) = false;)$', actor, re.M))
production += '\n' + function(actor, 'bool IsSongGiftParam(')
production += '\n' + function(actor, 'extern "C" int ComboSaria_IsParam(')
production += '\n' + function(actor, 'void ResetSariaResources(')
# The optional helpers make the same runner compile the pre-fix source for RED.
for signature in ('bool ForestSpawnReady(', 'void SpawnForestSaria(', 'void SpawnForestEpona(',
                  'void SpawnForestCompanions(', 'void RegisterSaria('):
    if signature in actor:
        production += '\n' + function(actor, signature)

native = '\n'.join(function(hooks, signature) for signature in (
    'void GameInteractor_ExecuteOnGameStateMainStart(', 'void GameInteractor_ExecuteOnGameStateMainFinish(',
    'void GameInteractor_ExecuteOnGameStateDrawFinish(', 'void GameInteractor_ExecuteOnGameStateUpdate('))
native += '\n' + function(game, 'void GameState_Update(')
native += '\n' + function(play, 'void Play_Main(')
native += '\n' + '\n'.join(function(player, signature) for signature in (
    'bool Player_InBlockingCsMode(', 'bool Player_InCsMode('))
native += '\n' + fade[fade.index('typedef enum TransitionFadeDirection'):fade.index('static Gfx')]
native += '\n' + function(library, 's32 Math_StepToS(')
native += '\n' + '\n'.join(function(fade, signature) for signature in (
    'void TransitionFade_Start(', 'void* TransitionFade_Init(', 'void TransitionFade_Destroy(',
    'void TransitionFade_Update(', 's32 TransitionFade_IsDone(', 'void TransitionFade_SetColor(',
    'void TransitionFade_SetType('))
# Execute the actual transition initialization and running switch cases. The
# transition factory is the boundary; it selects the real native fade callbacks.
start = play.index('case TRANS_MODE_INSTANCE_INIT:')
end = play.index('\n    // update non-instance transitions', start)
native += '\nstatic void NativeTransitionStep(PlayState* this) {\n    switch (this->transitionMode) {\n'
native += play[start:end] + '\n}\n'
native = re.sub(r'\bthis\b', 'self', native)
fixture = (ROOT / 'tests/forest_actor_spawn_latency/spawn_latency_test.cpp').read_text()
fixture = fixture.replace('/* PRODUCTION */', production).replace('/* NATIVE */', native)

with tempfile.TemporaryDirectory(prefix='forest-spawn-latency-') as temporary:
    directory = Path(temporary)
    source, binary = directory / 'test.cpp', directory / 'test'
    source.write_text(fixture)
    flags = ['-std=c++20', '-g', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-fno-pie', '-no-pie']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    includes = ('.', 'combo', 'mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets',
                'libultraship/include')
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                    *['-I' + str(ROOT / path) for path in includes], str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary), args.case], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
