#!/usr/bin/env python3
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
assert (ROOT/'soh/soh/Enhancements/randomizer/SoulShuffleRuntime.cpp').exists(), 'missing real native actor soul bridge'
assert 'Randomizer_SoulShouldUpdateActor(play, actor)' in (ROOT/'soh/src/code/z_actor.c').read_text(), 'native special-death update loop bypasses soul gate'
assert 'Randomizer_SoulCanSpawnNonEnemy' in (ROOT/'soh/soh/Enhancements/randomizer/SoulShuffleRuntime.cpp').read_text(), 'native optional actor gate absent'
from runtime_fixture import prepare_runtime
with tempfile.TemporaryDirectory(prefix="soul-runtime-") as t:
    prepare_runtime(t)
    for mode in ('original','combo'):
        exe=Path(t)/mode
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-fmax-errors=5','-I'+t,'-O0','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',*(['-DCOMBO_BUILD'] if mode=='combo' else []),
            *['-I'+str(ROOT/p) for p in ('soh','soh/include','soh/src','soh/assets','libultraship/include','combo')],
            str(ROOT/'tests/shuffle_parity/souls/runtime_test.cpp'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
