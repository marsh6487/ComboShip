#!/usr/bin/env python3
"""Real production source checks, original/Combo guards, existing CMake flags.

CPATH must supply actual pinned dependencies. Native C resource-handle flags and
MM C++ -fpermissive are existing target options, not new warning downgrades.
"""
from pathlib import Path
import argparse,os,re,shlex,subprocess
ROOT=Path(__file__).resolve().parents[3]
SOURCES=(
 'mm/src/overlays/actors/ovl_En_Door/z_en_door.c','soh/src/overlays/actors/ovl_En_Door/z_en_door.c',
 'soh/src/overlays/actors/ovl_Bg_Spot15_Saku/z_bg_spot15_saku.c',
 'soh/src/overlays/actors/ovl_En_Heishi2/z_en_heishi2.c',
 'mm/2s2h/Rando/RustyDoorKeys.cpp','mm/2s2h/Rando/GiveItem.cpp','mm/2s2h/Rando/RemoveItem.cpp',
 'mm/2s2h/Rando/ConvertItem.cpp','mm/2s2h/Rando/Menu.cpp','mm/2s2h/BenPort.cpp',
 'mm/2s2h/Rando/StaticData/Items.cpp','mm/2s2h/Rando/StaticData/Options.cpp',
 'mm/2s2h/Rando/Logic/GeneratePools.cpp','mm/2s2h/Rando/Logic/Regions/Central.cpp',
 'mm/2s2h/Rando/Logic/Regions/East.cpp','mm/2s2h/Rando/Logic/Regions/MilkRoad.cpp',
 'mm/2s2h/FleetShipCombo/FleetComboItemsGlue.cpp',
 'soh/soh/Enhancements/randomizer/settings.cpp','soh/soh/Enhancements/randomizer/LockOverworldDoors.cpp','soh/soh/Enhancements/randomizer/item_list.cpp',
 'soh/soh/Enhancements/randomizer/logic.cpp','soh/soh/Enhancements/randomizer/3drando/item_pool.cpp',
 'soh/soh/Enhancements/randomizer/randomizer.cpp','soh/soh/FleetShipCombo/FleetComboItemsGlue.cpp',
 *('soh/soh/Enhancements/randomizer/location_access/overworld/'+x+'.cpp' for x in ('kakariko','lon_lon_ranch','market','lake_hylia','graveyard','castle_grounds')),
)
p=argparse.ArgumentParser();p.add_argument('--mode',choices=('original','combo','both'),default='both');p.add_argument('sources',nargs='*');a=p.parse_args()
for mode in (('original','combo') if a.mode=='both' else (a.mode,)):
 for filename in a.sources or SOURCES:
  game=filename.split('/')[0];cpp=filename.endswith('.cpp')
  flags=['-std=gnu++20' if cpp else '-std=gnu11','-fsyntax-only','-fmax-errors=5','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING','-DLOG_LEVEL_GAME_PRINTS=0','-DMM_BUILD_DLL' if game=='mm' else '-DSOH_BUILD_DLL']
  if mode=='combo':flags+=['-DCOMBO_BUILD']
  settings=dict(re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',(ROOT/'CMake/soh-cvars.cmake').read_text()))
  for name,value in settings.items():flags+=['-D'+name+'="'+value+'"']
  for name,value in re.findall(r'set\((CVAR_\w+) "([^"]+)"\)',(ROOT/'CMake/lus-cvars.cmake').read_text()):
   for key,resolved in settings.items():value=value.replace('${'+key+'}',resolved)
   flags+=['-D'+name+'="'+value+'"']
  if not cpp:flags+=['-Werror=implicit-function-declaration','-Wno-int-conversion','-Wno-incompatible-pointer-types']
  if cpp:flags+=['-DIMGUI_DEFINE_MATH_OPERATORS']
  if cpp and game=='mm':
   flags+=['-fpermissive']
   for header in ('libultraship/bridge/consolevariablebridge.h','ship/Context.h','ship/resource/ResourceManager.h','ship/window/Window.h','spdlog/spdlog.h','string','vector','map','unordered_map','filesystem','fstream','algorithm','functional','variant','2s2h/GameInteractor/GameInteractor.h','variables.h','functions.h','macros.h','z64.h'):flags+=['-include',header]
  for path in ('.',game+'/include',game+'/include/PR',game+'/src',game,game+'/assets',game+('/2s2h' if game=='mm' else '/soh'),'libultraship/include','libultraship/src','libultraship/extern','combo','combo/menu'):flags+=['-I'+str(ROOT/path)]
  compiler=shlex.split(os.environ.get('CXX' if cpp else 'CC','c++' if cpp else 'cc'))
  result=subprocess.run([*compiler,*flags,str(ROOT/filename)],capture_output=True,text=True)
  print(mode,filename,'PASS' if not result.returncode else 'FAIL',f'({result.stderr.count("warning:")} compiler warnings)',flush=True)
  if result.returncode:print(result.stdout+result.stderr,flush=True);raise SystemExit(result.returncode)
