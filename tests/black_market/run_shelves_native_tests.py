#!/usr/bin/env python3
"""Run shelf GBI/collision code using each engine's actual registration functions."""
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/custom_unbound_collision'))
from run_tests import native_unit
with tempfile.TemporaryDirectory(prefix='shelves-native-') as temporary:
    for host in ('mm','soh'):
        camera_index='SurfaceType_GetBgCamIndex' if host=='mm' else 'SurfaceType_GetCamDataIndex'
        names=['BgActor_SetActor','DynaPoly_IsBgIdBgActor','DynaPoly_SetBgActor','BgCheck_GetCollisionHeader',
            'SurfaceType_GetData' if host=='mm' else 'SurfaceType_Get',camera_index,
            'BgCheck_GetBgCamSettingImpl','DynaPolyActor_Init']
        if host=='soh':
            names+=['BgCheck_ReallocTable','ScaleRotPos_Initialize','SSList_SetNull','DynaLookup_ResetLists',
                'DynaLookup_Reset','DynaLookup_ResetVtxStartIndex','BgActor_Initialize','DynaPoly_GrowBgActorTable',
                'DynaPoly_IsBgIdInTable']
        native=Path(temporary)/(host+'.c');obj=Path(temporary)/(host+'.o')
        bodies=native_unit(host,names)
        declarations=''
        if host=='soh':
            bodies+='\nvoid Fixture_GrowBgActorTable(PlayState* play, DynaCollisionContext* dyna, s32 capacity) {\n'
            bodies+='    DynaPoly_GrowBgActorTable(play, dyna, capacity);\n}\n'
            declarations='extern "C" void Fixture_GrowBgActorTable(PlayState*, DynaCollisionContext*, s32);'
        native.write_text(bodies)
        fixture=(ROOT/'tests/black_market/shelves_native_test.cpp').read_text().replace('/* NATIVE COLLISION */',declarations)
        fixture=fixture.replace('SHELF_CAMERA_INDEX',camera_index)
        cpp=Path(temporary)/(host+'.cpp');cpp.write_text(fixture);binary=Path(temporary)/host
        paths=[host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+('/2s2h' if host=='mm' else '/soh'),'libultraship/include','combo']
        flags=['-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-g',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie','-I'+str(ROOT),
            *['-I'+str(ROOT/p) for p in paths]]
        subprocess.run([os.environ.get('CC','cc'),'-std=c11',*flags,'-Werror=implicit-function-declaration',
            '-c',str(native),'-o',str(obj)],check=True)
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags,
            *(['-DFIXTURE_OOT'] if host=='soh' else []),str(cpp),str(obj),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
