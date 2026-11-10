#!/usr/bin/env python3
"""Compile changed MM UI units with real native headers in both host guards."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import os
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    'mm/src/code/'+name+'.c' for name in ['game','z_construct','z_map_exp','z_map_disp','z_message',
                                         'z_parameter','z_play_hireso','z_player_lib','z_actor','z_debug_display']
] + [
    f'mm/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c'
    for actor in ['En_Fsn','En_Ossan','En_Sob1','En_Trt','En_Mag']
] + [
    'mm/src/overlays/gamestates/ovl_file_choose/z_file_choose_NES.c',
    'mm/src/overlays/gamestates/ovl_title/z_title.c',
    'mm/src/overlays/gamestates/ovl_select/z_select.c',
    'mm/src/overlays/gamestates/ovl_daytelop/z_daytelop.c',
    'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope_NES.c',
    'mm/src/overlays/actors/ovl_NeiPartners/z_nei_command_melody.c',
    'mm/src/overlays/actors/ovl_player_actor/z_player.c',
    'mm/mods/items/helpers/box_menu.c',
    'mm/2s2h/BenGui/BenMenu.cpp',
    'mm/2s2h/Enhancements/CaneWheelHud.cpp',
    'mm/2s2h/Rando/DesireCompassHud.cpp',
    'mm/2s2h/Enhancements/Graphics/BowReticle.cpp',
    'mm/2s2h/NameTag/NameTag.cpp',
    'mm/2s2h/Network/Harpoon/HarpoonHookHandlers.cpp',
    'mm/2s2h/DeveloperTools/CollisionViewer.cpp',
]
dependencies = Path(os.environ.get('UI_VISIBILITY_DEPS','/usr/include'))
extra_includes = [p for name in ['NLOHMANN_JSON_INCLUDE','IMGUI_DIR','COMBO_NEI_FIXTURE_CPATH']
                  for p in os.environ.get(name,'').split(os.pathsep) if p]
includes = ['','mm/include','mm/include/PR','mm/src','mm/assets','mm','mm/mods','mm/2s2h',
            'libultraship/include','libultraship/src','combo','combo/menu','combo/gui']
flags = ['-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0','-DNDEBUG',
         '-DNON_EQUIVALENT','-DNON_MATCHING','-DIMGUI_DEFINE_MATH_OPERATORS','-DFMT_HEADER_ONLY']
for name in ['libultraship/cmake/cvars.cmake','CMake/lus-cvars.cmake','CMake/2ship-cvars.cmake']:
    if (ROOT/name).exists():
        flags += [f'-D{key}="{value}"' for key,value in
                  re.findall(r'set\((CVAR_\w+) "([^"]+)"',(ROOT/name).read_text())]

def compile_one(entry):
    source, combo = entry
    cpp = source.endswith('.cpp')
    command = [os.environ.get('CXX' if cpp else 'CC','c++' if cpp else 'cc'),
               '-std=gnu++20' if cpp else '-std=gnu11','-w','-fsyntax-only',*flags,
               *([] if cpp else ['-Wno-int-conversion','-Wno-incompatible-pointer-types','-Wno-implicit-function-declaration']),
               *(['-DCOMBO_BUILD'] if combo else []),*['-I'+str(ROOT/p) for p in includes],
               *['-I'+str(dependencies/p) for p in ['','imgui','spdlog-src/include','thread-pool']], source]
    command[-1:-1] = ['-I'+p for p in extra_includes]
    result = subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
    return source,combo,result

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--full', action='store_true', help='Also require GUI C++ dependencies such as SDL2 and libzip')
args = parser.parse_args()
selected = SOURCES if args.full else [s for s in SOURCES if not s.endswith('.cpp')]
entries = [(source,combo) for source in selected for combo in [False,True]]
failures = []
with ThreadPoolExecutor(max_workers=3) as pool:
    for source,combo,result in pool.map(compile_one,entries):
        if result.returncode:
            failures.append(source)
            print('FAIL native MM '+('combo' if combo else 'original')+': '+source,flush=True)
            errors = [line for line in result.stderr.splitlines() if 'error:' in line or 'fatal error:' in line]
            print('\n'.join(errors[:5]),flush=True)
if failures:
    raise SystemExit(1)
print(f'PASS {len(entries)} full MM native syntax checks: original and Combo guards')
