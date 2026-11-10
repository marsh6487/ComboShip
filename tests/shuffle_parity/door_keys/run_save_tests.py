#!/usr/bin/env python3
"""Execute production MM JSON serializer; real dependency headers only."""
from pathlib import Path
import argparse,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--json-include',type=Path);p.add_argument('--spdlog-include',type=Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='rusty-save-') as td:
 for mode in ('original','combo'):
  exe=Path(td)/mode;flags=['-std=c++20','-O0','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DFMT_HEADER_ONLY','-I'+str(ROOT)]
  if mode=='combo':flags+=['-DCOMBO_BUILD']
  for path in ('mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo'):flags+=['-isystem',str(ROOT/path)]
  for path in (a.json_include,a.spdlog_include):
   if path:flags+=['-isystem',str(path)]
  subprocess.run([os.environ.get('CXX','c++'),*flags,str(Path(__file__).with_name('save_test.cpp')),str(ROOT/'mm/2s2h/Rando/SoloEntrances.cpp'),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
