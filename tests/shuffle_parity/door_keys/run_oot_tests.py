#!/usr/bin/env python3
"""Execute actual native OoT door bridge and native save/actor/catalog types."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='oot-rusty-') as td:
 for mode in ('original','combo'):
  exe=Path(td)/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O0','-fmax-errors=5','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t',*(['-DCOMBO_BUILD'] if mode=='combo' else []),*['-I'+str(ROOT/p) for p in ('soh','soh/include','soh/src','soh/assets','libultraship/include','combo')],str(Path(__file__).with_name('oot_runtime_test.cpp')),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
