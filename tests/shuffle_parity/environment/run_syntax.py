#!/usr/bin/env python3
"""Compile complete touched native environmental/UI translation units in both modes.

Supply repository dependency include roots via CPATH. JSON is an existing
project precompiled-header dependency, included explicitly for this probe.
"""
from pathlib import Path
import os, subprocess
ROOT=Path(__file__).resolve().parents[3]
# libultraship/cmake/dependencies/patches/imgui-fixes-and-config.patch:112
# pins the production ImTextureID interface to void*. Raw official headers need the same define.
flags=['-DImTextureID=void*','-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
       '-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-include','nlohmann/json.hpp']
for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
 flags+=['-isystem',str(ROOT/path)]
for mode in ('original','combo'):
 for path in ('ActorBehavior/BgIcicle.cpp','ActorBehavior/ActorBehavior.cpp','CheckTracker/CheckTracker.cpp',*[str(p.relative_to(ROOT/'mm/2s2h/Rando')) for p in sorted((ROOT/'mm/2s2h/Rando/Logic/Regions').glob('*.cpp')) if p.name in ('Central.cpp','East.cpp','GreatBayTemple.cpp','MilkRoad.cpp','Moon.cpp','North.cpp','SnowheadTemple.cpp','South.cpp','SpiderHouses.cpp','TerminaField.cpp','West.cpp')],'StaticData/Checks.cpp','StaticData/Options.cpp','Logic/GeneratePools.cpp','Menu.cpp','../CustomItem/CustomItem.cpp'):
  subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if mode=='combo' else []),
                  '-fsyntax-only',str(ROOT/'mm/2s2h/Rando'/path)],check=True)
  print('PASS complete native environmental translation unit',mode,path,flush=True)

 for path in tuple('mm/src/overlays/actors/'+p for p in ('ovl_Bg_Icicle/z_bg_icicle.c','ovl_En_Ishi/z_en_ishi.c','ovl_Obj_Bombiwa/z_obj_bombiwa.c','ovl_Obj_Hamishi/z_obj_hamishi.c','ovl_Obj_Swprize/z_obj_swprize.c','ovl_Obj_Bean/z_obj_bean.c')):
  cflags=[f for f in flags if f not in ('-std=c++20','-include','nlohmann/json.hpp')]
  subprocess.run([os.environ.get('CC','cc'),'-std=c11',*cflags,*(['-DCOMBO_BUILD'] if mode=='combo' else []),'-fsyntax-only',str(ROOT/path)],check=True)
  print('PASS complete native environmental C translation unit',mode,path,flush=True)
