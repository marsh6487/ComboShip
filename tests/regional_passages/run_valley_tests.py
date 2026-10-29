#!/usr/bin/env python3
"""Portable native valley fixtures; optional actual archives verify the active graph."""
from pathlib import Path
import os, subprocess, tempfile, json, sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/custom_unbound_collision'))
from run_tests import native_unit
with tempfile.TemporaryDirectory(prefix='combo-valley-') as td:
    for name in ('valley_test','valley_resource_test'):
        binary=Path(td)/name
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT/'combo'),str(ROOT/'tests/regional_passages'/f'{name}.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    mesh_text=subprocess.check_output([str(Path(td)/'valley_test'),'--mesh'],text=True)
    band_text=subprocess.check_output([str(Path(td)/'valley_test'),'--bands'],text=True)
    bands=json.dumps([[float(x) for x in line.split()] for line in band_text.splitlines()])
    subprocess.run([os.environ.get('PYTHON','python3'),str(ROOT/'tests/regional_passages/valley_archive_test.py'),'--bands',bands],input=mesh_text,text=True,check=True)
    host=ROOT/'soh/soh/Enhancements/Gameplay/ComboFountainValley.cpp'
    assert host.exists(), 'missing process-owned valley collision actor and resource lifecycle'

    import re
    def function(source,name):
        match=re.search(r'^\w+[ *]+(?:'+re.escape(name)+r')\([^;{]*\)\s*\{',source,re.M)
        assert match,name
        start=match.start();end=source.index('{',start)+1;depth=1
        while depth:
            depth+=(source[end]=='{')-(source[end]=='}');end+=1
        return source[start:end]
    source=host.read_text()
    body=source.replace(function(source,'Draw'),'void Draw(Actor*,PlayState*) {}')
    body=body.replace(function(body,'Register'),'').replace('RegisterShipInitFunc valleyInit(Register);','')
    native_names=['BgCheck_ReallocTable','ScaleRotPos_Initialize','SSList_SetNull',
        'DynaLookup_ResetLists','DynaLookup_Reset','DynaLookup_ResetVtxStartIndex','BgActor_Initialize',
        'DynaPoly_GrowBgActorTable','BgActor_SetActor','DynaPoly_IsBgIdBgActor','DynaPoly_IsBgIdInTable',
        'DynaPoly_IsBgIdAllocated','DynaPoly_SetBgActor','DynaPoly_GetActor','DynaPoly_DeleteBgActor','DynaPolyActor_Init']
    native_c=Path(td)/'native.c';native_object=Path(td)/'native.o'
    native_c.write_text(native_unit('soh',native_names)+
        '\nvoid Fixture_GrowBgActorTable(PlayState* play, DynaCollisionContext* dyna, s32 capacity) {\n'
        '    DynaPoly_GrowBgActorTable(play, dyna, capacity);\n}\n')
    native='extern "C" void Fixture_GrowBgActorTable(PlayState*, DynaCollisionContext*, s32);'
    fixture=(ROOT/'tests/regional_passages/valley_native_test.cpp').read_text()
    # The native hash handler resolves typed vertex owners on each execution.
    # Exercise its actual pointer/stride handling and packet preservation.
    renderer=function((ROOT/'libultraship/src/fast/interpreter.cpp').read_text(),
                      'gfx_vtx_hash_handler_custom')
    renderer=re.search(r'^#define MAX_VERTICES[^\n]*',
        (ROOT/'libultraship/include/fast/interpreter.h').read_text(),re.M)[0]+'\n'+renderer
    generated=Path(td)/'native.cpp';generated.write_text(fixture.replace('/* NATIVE_ENGINE */',native).replace('/* NATIVE_PRODUCTION */',body).replace('/* NATIVE_RENDERER */',renderer))
    env=dict(os.environ)
    if env.get('COMBO_NEI_FIXTURE_CPATH'):env['CPATH']=env['COMBO_NEI_FIXTURE_CPATH']+os.pathsep+env.get('CPATH','')
    paths=['soh','soh/soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include','combo','soh/soh/Enhancements/Gameplay']
    flags=['-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY',*['-I'+str(ROOT/p)for p in paths]]
    host_object=Path(td)/'valley-production.o'
    # Match MSVC's rejection of const filenames passed to legacy char* APIs.
    strict_flags=[flag for flag in flags if flag!='-w']+['-Werror=write-strings']
    subprocess.run([os.environ.get('CXX','c++'),*strict_flags,'-c',str(host),'-o',str(host_object)],env=env,check=True)
    subprocess.run([os.environ.get('PYTHON','python3'),str(ROOT/'tests/regional_passages/valley_linkage_test.py'),str(host_object)],env=env,check=True)
    sources=['libultraship/src/fast/resource/type/DisplayList.cpp','libultraship/src/fast/resource/type/Vertex.cpp','libultraship/src/ship/resource/Resource.cpp']
    binary=Path(td)/'native'
    sanitizer=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
    subprocess.run([os.environ.get('CC','cc'),'-std=c11',*flags[1:],*sanitizer,
        '-Werror=implicit-function-declaration','-c',str(native_c),'-o',str(native_object)],env=env,check=True)
    subprocess.run([os.environ.get('CXX','c++'),*flags,*sanitizer,str(generated),str(native_object),
        *[str(ROOT/p)for p in sources],'-o',str(binary)],env=env,check=True)
    subprocess.run([str(binary)],env={**env,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
