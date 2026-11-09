#!/usr/bin/env python3
"""Compile complete touched native lottery/UI translation units in both modes.

Supply repository dependency include roots via CPATH. JSON is an existing
project precompiled-header dependency, included explicitly for this probe.
"""
from pathlib import Path
import os, subprocess
ROOT=Path(__file__).resolve().parents[3]
flags=['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
       '-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-include','nlohmann/json.hpp']
for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
 flags+=['-isystem',str(ROOT/path)]
for mode in ('original','combo'):
 for path in ('ActorBehavior/EnKujiya.cpp','Logic/Regions/Central.cpp','Menu.cpp'):
  subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if mode=='combo' else []),
                  '-fsyntax-only',str(ROOT/'mm/2s2h/Rando'/path)],check=True)
  print('PASS complete native lottery translation unit',mode,path,flush=True)
