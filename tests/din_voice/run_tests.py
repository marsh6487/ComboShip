#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ui_visibility'))
from run_control_input_tests import compiler_flags,function
parser=argparse.ArgumentParser();parser.add_argument('archive',type=Path);args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='din-native-voices-') as tmp:
    tmp=Path(tmp);resources=tmp/'resources'
    with zipfile.ZipFile(args.archive) as archive:
        for name in archive.namelist():
            if name.startswith('custom/din_voice/'):
                assert '..' not in Path(name).parts
                p=resources/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(archive.read(name))
    runtime=(ROOT/'mm/mods/voice_pack/din_voice.cpp').read_text()
    (tmp/'runtime.inc').write_text(runtime[runtime.index('namespace {'):])
    native=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    (tmp/'native_voice.inc').write_text(function(native,'Player_AnimSfx_PlayVoice'))
    helper=(ROOT/'mm/mods/items/helpers/item_voice.c').read_text()
    (tmp/'item_voice.inc').write_text(function(helper,'ItemVoice_Play')+function(helper,'ItemVoice_PlayId'))
    for combo in (False,True):
        flags=[f for f in compiler_flags('mm',combo) if not f.startswith('-std=') and not f.startswith('-Wno-') and f!='-Werror=implicit-function-declaration']
        flags+=['-fno-sanitize=shift-base','-I'+str(tmp)]
        binary=tmp/('combo' if combo else 'solo')
        subprocess.run(['c++','-std=c++20',*flags,str(ROOT/'tests/din_voice/runtime_test.cpp'),'-o',str(binary)],check=True)
        for folder in (str(resources),'missing'):
            subprocess.run([str(binary),folder],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        print('PASS actual native voice dispatch + shared item helpers + age loader/mixer',combo,flush=True)
