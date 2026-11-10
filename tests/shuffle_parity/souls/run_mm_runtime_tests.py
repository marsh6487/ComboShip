#!/usr/bin/env python3
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
assert (ROOT/'mm/2s2h/Rando/SoulShuffleRuntime.cpp').exists(), 'native MM optional soul runtime absent'
def body(source,signature):
    a=source.index(signature);b=source.index('{',a);e=b+1;depth=1
    while depth:
        depth+=(source[e]=='{')-(source[e]=='}');e+=1
    return source[a:e]
from mm_runtime_fixture import prepare_mm
with tempfile.TemporaryDirectory(prefix="mm-soul-runtime-") as tmp:
    prepare_mm(tmp)
    for mode in ('original','combo'):
        exe=Path(tmp)/mode
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-fmax-errors=5','-O0','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',*(['-DCOMBO_BUILD'] if mode=='combo' else []),
            '-I'+tmp,*['-I'+str(ROOT/p) for p in ('mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo')],
            str(ROOT/'tests/shuffle_parity/souls/mm_runtime_test.cpp'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
