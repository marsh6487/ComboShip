#!/usr/bin/env python3
"""Complete touched native units, actual headers and unchanged CMake compiler options.

Provide real dependencies with CPATH. SONG_PATH_FILTER narrows a repeat after an
individual fix; default covers both original/Combo native units and actors.
"""
from pathlib import Path
import os,re,subprocess
ROOT=Path(__file__).resolve().parents[3]
common=['-DLOG_LEVEL_GAME_PRINTS=6','-DSPDLOG_ACTIVE_LEVEL=0','-DIMGUI_DEFINE_MATH_OPERATORS','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT)]
settings=dict(re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',(ROOT/'CMake/soh-cvars.cmake').read_text()))
for name,value in settings.items():common+=['-D'+name+'="'+value+'"']
for name,value in re.findall(r'set\((CVAR_\w+) "([^"]+)"\)',(ROOT/'CMake/lus-cvars.cmake').read_text()):
    for key,resolved in settings.items():value=value.replace('${'+key+'}',resolved)
    common+=['-D'+name+'="'+value+'"']
for host in ('mm','soh'):
    flags=common+['-DMM_BUILD_DLL' if host=='mm' else '-DSOH_BUILD_DLL']
    for p in (host+'/include',host+'/include/PR',host+('/2s2h' if host=='mm' else '/soh'),host,host+'/src',host+'/assets','libultraship/include','combo','combo/menu'):
        flags+=['-isystem',str(ROOT/p)]
    if host=='mm':
        paths=['mm/2s2h/Rando/'+p for p in ('SongEvents.cpp','Menu.cpp','DrawItem.cpp','Logic/GeneratePools.cpp','Spoiler/Apply.cpp','Spoiler/Generate.cpp','ActorBehavior/EnDai.cpp','ActorBehavior/DmChar01.cpp','ActorBehavior/DmChar08.cpp','Logic/Regions/North.cpp','Logic/Regions/East.cpp','Logic/Regions/West.cpp','Logic/Regions/TerminaField.cpp','Logic/Regions/SpiderHouses.cpp')]
        paths+=['mm/src/code/z_message.c']
    else:
        paths=['soh/soh/Enhancements/randomizer/'+p for p in ('SongEvents.cpp','SeedContext.cpp','logic.cpp','location_access.cpp','settings.cpp','item_list.cpp','draw.cpp','randomizer.cpp','Plandomizer.cpp','3drando/item_pool.cpp','3drando/spoiler_log.cpp')]
        paths+=['soh/src/code/z_parameter.c','soh/src/code/z_kankyo.c','soh/src/code/z_play.c','soh/src/code/z_message_PAL.c','soh/soh/SaveManager.cpp']
    paths += [str(p.relative_to(ROOT)) for p in (ROOT/host/'src/overlays/actors').glob('*/z_*.c') if 'SongEvents' in p.read_text()]
    if os.environ.get('SONG_PATH_FILTER'):paths=[p for p in paths if re.search(os.environ['SONG_PATH_FILTER'],p)]
    for mode in ('original','combo'):
        for path in paths:
            cpp=path.endswith('.cpp');compiler=os.environ.get('CXX','c++') if cpp else os.environ.get('CC','cc')
            # Existing native CMake allowances are replayed, never added to source/build policy.
            language=['-fpermissive','-include','PR/ultratypes.h' if host=='mm' else 'libultraship/libultra.h','-include','nlohmann/json.hpp'] if cpp else ['-Wno-incompatible-pointer-types','-Wno-int-conversion']
            subprocess.run([compiler,'-std=c++20' if cpp else '-std=c11',*flags,*language,*(['-DCOMBO_BUILD'] if mode=='combo' else []),'-fsyntax-only',str(ROOT/path)],check=True)
            print('PASS',host,mode,path,flush=True)
