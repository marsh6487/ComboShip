#!/usr/bin/env python3
"""Exercise the actual optional-reward collision guards with native types."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'mm/2s2h/Enhancements/Gameplay/ComboVanillaRewards.cpp').read_text()


def function(signature):
    start=source.index(signature);brace=source.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


fixture=(ROOT/'tests/combo_vanilla/mm_rewards_geometry_test.cpp').read_text()
fixture=fixture.replace('/* BINDINGS */',source[source.index('struct Binding {'):source.index('bool NativePrerequisite')])
fixture=fixture.replace('/* PRODUCTION_FUNCTIONS */','\n'.join(function(name) for name in
    ('bool Active','bool Safe','bool ReadNativeAnchors','bool NativeAnchor','bool DryFloor','bool ClearApproach','bool Placement')))
native_actor=(ROOT/'mm/src/code/z_actor.c').read_text()
yaw_start=native_actor.index('    s16 rotY = (actorEntry->rot.y >> 7) & 0x1FF;')
yaw_end=native_actor.index('    if (!(actorEntry->id & 0x4000))',yaw_start)
yaw=native_actor[yaw_start:yaw_end].replace('    s16 rotZ = (actorEntry->rot.z >> 7) & 0x1FF;','')
fixture=fixture.replace('/* NATIVE_SPAWN_YAW */','static s16 NativeSpawnYaw(ActorEntry* actorEntry) {\n'+yaw+'\nreturn rotY;\n}')
with tempfile.TemporaryDirectory(prefix='vanilla-mm-geometry-') as tmp:
    cpp=Path(tmp)/'test.cpp';cpp.write_text(fixture)
    flags=['-std=c++20','-w','-DF3DEX_GBI_2']
    for path in ('mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
        flags+=['-I'+str(ROOT/path)]
    if '--sanitize' in sys.argv:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    binary=Path(tmp)/'test'
    native=['mm/2s2h/resource/type/Scene.cpp',
            'mm/2s2h/resource/type/scenecommand/SetAlternateHeaders.cpp',
            'mm/2s2h/resource/type/scenecommand/SetEntranceList.cpp',
            'mm/2s2h/resource/type/scenecommand/SetStartPositionList.cpp']
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),
                    *[str(ROOT/path) for path in native],'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
