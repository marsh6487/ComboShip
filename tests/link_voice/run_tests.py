#!/usr/bin/env python3
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ui_visibility'))
from run_control_input_tests import compiler_flags,function
parser=argparse.ArgumentParser()
parser.add_argument('archive',type=Path,nargs='?',help='Optional real PCM pack; omitted uses deterministic asset-free PCM')
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='link-native-voices-') as tmp:
    tmp=Path(tmp);resources=tmp/'resources'
    if args.archive:
        with zipfile.ZipFile(args.archive) as archive:
            for name in archive.namelist():
                if name.startswith('custom/din_voice/'):
                    assert '..' not in Path(name).parts
                    p=resources/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(archive.read(name))
    else:
        # Every age/action has distinct bytes. This tests dispatch in CI without
        # distributing audio; the explicit archive mode still tests real PCM.
        manifest={'version':1,'rate':32000,'child':{},'adult':{}}
        for age,label in enumerate(('child','adult')):
            for action in range(32):
                name=f'custom/din_voice/pcm/{label}_{action}.pcm'
                samples=[(age+1)*1000+action*16+(i%16)*4 for i in range(4096)]
                p=resources/name;p.parent.mkdir(parents=True,exist_ok=True)
                p.write_bytes(struct.pack('<4096h',*samples))
                manifest[label][str(action)]=[name]
        (resources/'custom/din_voice/voices.json').write_text(json.dumps(manifest,sort_keys=True))
    malformed=tmp/'malformed/custom/din_voice/voices.json'
    malformed.parent.mkdir(parents=True)
    malformed.write_text('{"version":1,"rate":32000,"child":')
    runtime=(ROOT/'mm/mods/voice_pack/link_voice.cpp').read_text()
    (tmp/'runtime.inc').write_text(runtime[runtime.index('namespace {'):])
    native=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    (tmp/'native_voice.inc').write_text(function(native,'Player_AnimSfx_PlayVoice'))
    save=(ROOT/'mm/mods/nei_save.cpp').read_text()
    adult=(ROOT/'mm/mods/items/logic/adult_link_render.cpp').read_text()
    (tmp/'age_state.inc').write_text(function(save,'Nei_Save')+function(adult[adult.index('extern "C" s32 AdultLink_IsActive'):],'AdultLink_IsActive'))
    forms=(ROOT/'mm/mods/forms/custom_forms.cpp').read_text()
    (tmp/'form_voice.inc').write_text(function(forms[forms.index('extern "C" u8 CustomForms_VoiceOverride'):],'CustomForms_VoiceOverride'))
    actor=(ROOT/'mm/src/code/z_actor.c').read_text()
    (tmp/'player_sfx.inc').write_text(function(actor,'Player_PlaySfx'))
    interactor=(ROOT/'mm/2s2h/GameInteractor/GameInteractor.cpp').read_text()
    (tmp/'interactor.inc').write_text(function(interactor,'GameInteractor_Should'))
    pitch=(ROOT/'mm/2s2h/Enhancements/Audio/Sfx/LinksVoicePitchMultiplier.cpp').read_text()
    (tmp/'pitch_hook.inc').write_text(pitch[pitch.index('static f32 freqMultiplier'):pitch.index('static RegisterShipInitFunc')])
    sfx=(ROOT/'mm/src/audio/sfx.c').read_text()
    (tmp/'sfx_queue.inc').write_text(sfx[sfx.index('typedef struct {'):sfx.index('void AudioSfx_MuteBanks')]+function(sfx,'AudioSfx_PlaySfx'))
    thread=(ROOT/'mm/src/audio/lib/thread.c').read_text()
    (tmp/'audio_mix.inc').write_text(function(thread,'AudioMgr_CreateNextAudioBuffer'))
    helper=(ROOT/'mm/mods/items/helpers/item_voice.c').read_text()
    (tmp/'item_voice.inc').write_text(function(helper,'ItemVoice_Play')+function(helper,'ItemVoice_PlayId'))
    for combo in (False,True):
        flags=[f for f in compiler_flags('mm',combo) if not f.startswith('-std=') and not f.startswith('-Wno-') and f!='-Werror=implicit-function-declaration']
        flags+=['-O0','-fno-sanitize=shift-base','-I'+str(tmp)]
        binary=tmp/('combo' if combo else 'solo')
        subprocess.run(['c++','-std=c++20',*flags,str(ROOT/'tests/link_voice/runtime_test.cpp'),'-o',str(binary)],check=True)
        for folder, donor in (('missing','unavailable'),(str(resources),'available'),('missing','available'),('malformed','available')):
            subprocess.run([str(binary),folder,donor],cwd=tmp,check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        print('PASS real player/pitch-hook/SFX-queue dispatch + optional/native/failure voice routes',combo,flush=True)
