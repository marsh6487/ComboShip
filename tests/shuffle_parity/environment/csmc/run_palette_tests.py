#!/usr/bin/env python3
"""Verify the real OoT particle CSMC function preserves every palette component."""
from pathlib import Path
import argparse,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[4]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sanitize',action='store_true');args=p.parse_args()
with tempfile.TemporaryDirectory(prefix='environment-palette-') as td:
 for combo in (False,True):
  binary=Path(td)/('combo' if combo else 'original')
  flags=['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-include','global.h']
  for name in ('soh','soh/include','soh/src','soh/assets','libultraship/include'):flags+=['-isystem',str(ROOT/name)]
  if combo:flags+=['-DCOMBO_BUILD']
  if args.sanitize:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
  subprocess.run([os.environ.get('CXX','c++'),*flags,str(Path(__file__).with_name('palette_test.cpp')),str(ROOT/'soh/soh/Enhancements/randomizer/particle_cmc.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
  print('PASS actual OoT CSMC palette guard',combo,flush=True)
