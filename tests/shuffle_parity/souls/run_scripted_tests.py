#!/usr/bin/env python3
"""Run native scripted reward branches before event and transition consumption."""
from pathlib import Path
import os,subprocess,tempfile
from runtime_fixture import prepare_runtime
ROOT=Path(__file__).resolve().parents[3]
s=(ROOT/'soh/src/code/z_demo.c').read_text();a=s.index('        if ((gSaveContext.entranceIndex == ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE)');b=s.index('        } else if (!Flags_GetEventChkInf(EVENTCHKINF_WATCHED_GANONS_CASTLE',a);native=s[a:b]+'        }\n'
with tempfile.TemporaryDirectory(prefix='souls-scripted-') as tmp:
 t=Path(tmp);prepare_runtime(tmp);(t/'scripted.inc').write_text('static void NativeScripted(PlayState*play){\n'+native+'}\n')
 for mode in ('original','combo'):
  exe=t/mode;subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O0','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-I'+tmp,*['-I'+str(ROOT/p) for p in ['soh','soh/include','soh/src','soh/assets','libultraship/include','combo']],*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(ROOT/'tests/shuffle_parity/souls/scripted_test.cpp'),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
