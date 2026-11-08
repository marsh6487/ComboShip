#!/usr/bin/env python3
"""Compile and exercise the production optional-night-music lifecycle."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='night-music-') as tmp:
    binary=Path(tmp)/'lifecycle'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie',
                    '-I'+str(ROOT/'combo'),str(ROOT/'tests/night_music/lifecycle_test.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    for host,path,debug_warp in [('soh','soh/soh/Enhancements/audio/SceneNightMusic.cpp',False),
                                 ('soh-debug-warp','soh/soh/Enhancements/audio/SceneNightMusic.cpp',True),
                                 ('mm','mm/2s2h/Enhancements/Audio/SceneNightMusic.cpp',False)]:
        production=(ROOT/path).read_text()
        production=production[production.index('static ComboNightMusic::Lifecycle'):production.index('static void RegisterSceneNightMusic')]
        fixture=(ROOT/'tests/night_music/host_test.cpp').read_text().replace('/* PRODUCTION */',production)
        if host!='mm':
            native_player=(ROOT/'soh/src/code/z_player_lib.c').read_text()
            native_talk=(ROOT/'soh/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
            bodies=[]
            for source,name in [(native_player,'Player_InBlockingCsMode'),(native_player,'Player_InCsMode'),
                                (native_talk,'Player_SetupTalk')]:
                match=re.search(r'^(?:s32|void) '+name+r'\([^;]*?\)\s*\{.*?^}',source,re.M|re.S)
                if match is None:
                    raise RuntimeError('Missing native '+name)
                # C's player parameter is named `this`; only rename that C++ keyword.
                bodies.append(re.sub(r'\bthis\b','player',match[0]))
            fixture=fixture.replace('/* NATIVE_PLAYER */','\n'.join(bodies))
        cpp=Path(tmp)/(host+'.cpp');cpp.write_text(fixture)
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-DTEST_MM='+str(int(host=='mm')),
                        '-DTEST_DEBUG_WARP='+str(int(debug_warp)),
                        '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie',
                        '-I'+str(ROOT/'combo'),str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    # Compile the real full-width command queue and its cancellation/identity tests.
    include=['-I'+str(ROOT/p) for p in ['soh/tests/audio_sequence_stubs','soh/include','soh','libultraship/include']]
    obj=Path(tmp)/'queue.o'
    flags=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    subprocess.run([os.environ.get('CC','cc'),'-std=c11',*flags,*include,'-c',
                    str(ROOT/'soh/src/code/code_800F9280.c'),'-o',str(obj)],check=True)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DHYRULE_FIELD_NIGHT_MUSIC_TEST',*flags,*include,
                    str(ROOT/'soh/tests/hyrule_field_night_music_test.cpp'),
                    str(ROOT/'soh/soh/Enhancements/audio/HyruleFieldNightMusic.cpp'),str(obj),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    print('PASS production audio queue: exact IDs, pending cancellation, native owner preservation')
    # Execute the real BGM table's range expression, not a separate ordering model.
    editor=(ROOT/'soh/soh/Enhancements/audio/AudioEditor.cpp').read_text()
    draw=editor[editor.index('void Draw_SfxTab('):]
    loop=re.search(r'for\s*\([^{}]*\bdefaultValue\b[^{}]*\)\s*\{',draw).group(0)
    code='''#include "audio/NightMusic.h"
#include <map>
#include <cassert>
int main() {
 constexpr uint16_t NA_BGM_FIELD_LOGIC=2,HYRULE_FIELD_NIGHT_MUSIC_ENTRY=0xff02;
 std::map<uint16_t,int> map{{2,0},{3,0},{5,0},{0xff00,0},{0xff01,0},{0xff02,0}};
 std::vector<uint16_t> actual;
 '''+loop+''' actual.push_back(defaultValue); }
 assert((actual==std::vector<uint16_t>{2,0xff02,3,5,0xff00,0xff01}));
}'''
    cpp=Path(tmp)/'row-order.cpp';cpp.write_text(code)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT/'combo'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    print('PASS production Background Music table order: day Hyrule Field, night Hyrule Field')
