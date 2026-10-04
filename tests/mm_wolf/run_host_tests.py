#!/usr/bin/env python3
"""Compile production host/core/ext-button accessors on real MM headers and execute lifecycle fixtures."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
def run(command):
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print(result.stdout+result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout: print(result.stdout,end='')
with tempfile.TemporaryDirectory(prefix='mm-wolf-host-') as td:
    # Compile these exact production bodies rather than recreating the native freeze state machine.
    player_source=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    def native_body(signature):
        start=player_source.index(signature)
        brace=player_source.index('{',start)
        depth=1; end=brace+1
        while depth:
            depth+=(player_source[end]=='{')-(player_source[end]=='}'); end+=1
        return player_source[start:end]
    native_actions=Path(td)/'native-freeze.c'
    native_actions.write_text('#include "global.h"\n'
                              'extern Input* sPlayerControlInput;\n'
                              'extern f32 sControlStickMagnitude;\n'
                              'extern s16 sControlStickAngle;\n'
                              's32 Player_InflictDamage(PlayState*, s32);\n'
                              'void func_80836988(Player*, PlayState*);\n'
                              'void func_808339B4(Player*, s32);\n'
                              'void func_80834104(PlayState*, Player*);\n'+
                              native_body('s32 func_8082DE88(Player* this, s32 arg1, s32 arg2) {')+'\n'+
                              native_body('void Player_Action_82(Player* this, PlayState* play) {')+'\n')
    objects=[]
    for i,path in enumerate(['mm/expansions/ssbb/ssbb_character.c','mm/expansions/ssbb/ssbb_skin.c',
                             'mm/src/code/z_skin_matrix.c','mm/src/code/z_lib.c','mm/mods/ext_buttons/ext_buttons.cpp',
                             str(native_actions)]):
        cpp=path.endswith('.cpp'); obj=str(Path(td)/f'{i}.o')
        run([os.environ.get('CXX' if cpp else 'CC','c++' if cpp else 'cc'),'-std=c++20' if cpp else '-std=gnu11',
             *flags(),'-include',str(ROOT/'mm/expansions/ssbb/ssbb_anim.h'),
             *([] if cpp else ['-include',str(ROOT/'mm/include/z64malloc.h')]),
             '-include',str(ROOT/'mm/include/functions.h'),'-include',str(ROOT/'mm/include/variables.h'),
             '-include',str(ROOT/'libultraship/include/libultraship/bridge/consolevariablebridge.h'),
             '-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',
             '-c',str(ROOT/path),'-o',obj]); objects.append(obj)
    binary=str(Path(td)/'host')
    options=['-DMM_WOLF_HOST'] if (ROOT/'mm/mods/forms/wolf_link_host.cpp').exists() else []
    run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),*options,'-DMM_WOLF_NATIVE_HANDOFF',
         '-DWOLF_IMPLEMENTATION="'+str(ROOT/'mm/mods/transformation_masks/wolf_link_form.cpp')+'"',
         '-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',
         str(ROOT/'tests/mm_wolf/host_runtime_test.cpp'),*objects,'-Wl,--gc-sections','-o',binary])
    run([binary,td])
