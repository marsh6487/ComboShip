#!/usr/bin/env python3
"""Production MM solo lifecycle/native types; no linked graph/fill claim."""
import os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='mm-solo-entry-') as td:
 b=Path(td)/'test';flags=['-std=c++20','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT/'tests/dungeon_rewards/headless.h')]
 for name in ('.','combo','mm/include','mm/include/PR','mm','mm/2s2h','mm/src','mm/assets','libultraship/include'):flags+=['-I'+str(ROOT/name)]
 subprocess.run([os.environ.get('CXX','c++'),*flags,str(Path(__file__).with_name('mm_solo_entrypoint_test.cpp')),'-o',str(b)],check=True);subprocess.run([str(b)],check=True)
print('PASS native-type MM solo entrypoint guards/dispatch/save-play-seed-receipt restoration on success/failure')
