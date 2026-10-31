"""Compile actual integration TUs against game headers (not simplified actor mocks)."""
import argparse
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--dependencies',type=Path,required=True)
p.add_argument('--legacy-pointer-warnings',action='store_true');p.add_argument('--solo',action='store_true');p.add_argument('--json-include',type=Path,default=ROOT.parent/'native-deps')
p.add_argument('sources',nargs='*');args=p.parse_args()
values={}
for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
    for key,value in re.findall(r'set\((CVAR_\w+)\s+"([^"\n]*)"',(ROOT/config).read_text()):
        values[key]=re.sub(r'\$\{(\w+)\}',lambda m:values[m[1]],value)
sources=args.sources or ['soh/mods/actors/trutefel/trutefel_enemies.cpp',
    'soh/mods/actors/trutefel/trutefel_actor_reg.cpp','mm/mods/trutefel/trutefel_enemies.cpp']
for source in sources:
    game='mm' if source.startswith('mm/') else 'soh'
    c=source.endswith('.c')
    cmd=['cc' if c else 'c++','-std=gnu2x' if c else '-std=gnu++20','-fsyntax-only',
         '-DIMGUI_DEFINE_MATH_OPERATORS','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING','-DLOG_LEVEL_GAME_PRINTS=0']
    if not args.solo:cmd+=['-DCOMBO_BUILD']
    if not c:cmd+=['-w']
    if c:cmd+=['-Werror=implicit-function-declaration','-Werror=incompatible-pointer-types']
    if c and args.legacy_pointer_warnings:cmd+=['-Wno-error=incompatible-pointer-types']
    cmd += [f'-D{k}="{v}"' for k,v in values.items()]
    paths=['',game,game+'/include',game+'/src',game+'/assets',game+'/mods',game+'/soh' if game=='soh' else game+'/2s2h',
           'combo','combo/menu','libultraship/include']
    cmd+=['-I'+str(ROOT/x) for x in paths]
    cmd+=['-idirafter',str(ROOT/'libultraship/include/libultraship/libultra')]
    cmd+=['-I'+str(args.json_include)]
    cmd+=['-I'+str(args.dependencies/x) for x in ['imgui','native-headers','spdlog/include','fmt/include','stb','thread-pool-v4/include','sdl2/include']]
    result=subprocess.run(cmd+[str(ROOT/source)],cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if result.returncode:
        print(result.stdout);raise SystemExit(result.returncode)
    print('PASS native TU:',source)
