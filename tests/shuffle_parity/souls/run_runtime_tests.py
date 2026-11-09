#!/usr/bin/env python3
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
assert (ROOT/'soh/soh/Enhancements/randomizer/SoulShuffleRuntime.cpp').exists(), 'missing real native actor soul bridge'
assert 'Randomizer_SoulShouldUpdateActor(play, actor)' in (ROOT/'soh/src/code/z_actor.c').read_text(), 'native special-death update loop bypasses soul gate'
assert 'Randomizer_SoulCanSpawnNonEnemy' in (ROOT/'soh/soh/Enhancements/randomizer/SoulShuffleRuntime.cpp').read_text(), 'native optional actor gate absent'
with tempfile.TemporaryDirectory(prefix='soul-runtime-') as t:
    actors=(ROOT/'soh/src/code/z_actor.c').read_text()
    a=actors.index('Actor* Actor_Spawn(');b=actors.index('    ActorDBEntry* dbEntry',a)
    (Path(t)/'spawn.inc').write_text(actors[a:b]+'    ++spawnReachesAllocation;return nullptr;\n}\n')
    collision=(ROOT/'soh/src/code/z_collision_check.c').read_text()
    start=collision.index('void CollisionCheck_AC(')
    end=collision.index('\n/**',start)
    (Path(t)/'collision.inc').write_text(collision[start:end])
    for mode in ('original','combo'):
        exe=Path(t)/mode
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-fmax-errors=5','-I'+t,'-O0','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',*(['-DCOMBO_BUILD'] if mode=='combo' else []),
            *['-I'+str(ROOT/p) for p in ('soh','soh/include','soh/src','soh/assets','libultraship/include','combo')],
            str(ROOT/'tests/shuffle_parity/souls/runtime_test.cpp'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
