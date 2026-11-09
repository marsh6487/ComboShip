#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,tempfile,re
ROOT=Path(__file__).resolve().parents[3]
def function(source,name):
    a=source.index(name);b=source.index('{',a);e=b+1;depth=1
    while depth:depth+=(source[e]=='{')-(source[e]=='}');e+=1
    return source[a:e]
adapter=(ROOT/'soh/soh/Enhancements/randomizer/SongEvents.cpp').read_text()
actors=(ROOT/'soh/src/code/z_actor.c').read_text()
with tempfile.TemporaryDirectory(prefix='song-event-oot-') as td:
    names=['void Rando::Context::GenerateSongEvents','void Rando::Context::ImportSongEvents','nlohmann::json Rando::Context::ExportSongEvents','bool Rando::Logic::CanPlaySongEvent','static int EventForScene','extern "C" int SongEvents_OotAction','extern "C" int SongEvents_OotClockLocked','extern "C" void SongEvents_OotApplyClockLock','bool Rando::Logic::ClockAllowsTimePass','extern "C" int SongEvents_OotIsEventAction','extern "C" int SongEvents_OotShouldCapture','extern "C" int SongEvents_OotSunSongTimeLocked']
    code='\n'.join(function(actors,n) for n in ('s32 Flags_GetRandomizerInf','void Flags_SetRandomizerInf'))+'\n'+'\n'.join(re.findall(r'^static int sArmedOot.*;',adapter,re.M))+'\n'+'\n'.join(function(adapter,n) for n in names)
    rd=(ROOT/'soh/src/overlays/actors/ovl_En_Rd/z_en_rd.c').read_text()
    code+='\n'+re.sub(r'\bthis\b','self',function(rd,'void EnRd_SetupAction(EnRd* this, EnRdActionFunc actionFunc) {'))
    code+='\n'+re.sub(r'\bthis\b','self',function(rd,'void EnRd_SetupStunned(EnRd* this) {'))
    message=(ROOT/'soh/src/code/z_message_PAL.c').read_text()
    a=message.index('    static u16 sOcarinaSongFlagsMap[] = {');b=message.index('    };',a)+len('    };')
    owned=message[a:b]+'\ns16 i;\n'+function(message,'for (i = sOcarinaSongBitFlags = 0;')
    a=message.index('            if (msgCtx->ocarinaAction == OCARINA_ACTION_UNK_0',message.index('// "Ocarina（'))
    end='osSyncPrintf("222222222222  -> ");'
    b=message.index(end,a)+len(end);b=message.index('}',b)+1
    choose=message[a:b]
    a=message.index('                if (msgCtx->msgMode == MSGMODE_OCARINA_STARTING)')
    b=message.index('                if (msgCtx->ocarinaAction != OCARINA_ACTION_FREE_PLAY',a)
    dispatch=message[a:b]
    code+='\nu32 gBitFlags[32]{};static u32 sOcarinaSongBitFlags=0,observedRecognitionFlags=0;\n'
    code+='void AudioOcarina_Start(u32 flags){observedRecognitionFlags=flags;}\n'
    code+='#undef osSyncPrintf\n#define osSyncPrintf(...) ((void)0)\n'
    code+='void DispatchNativeEventCheck(PlayState*play){auto*msgCtx=&play->msgCtx;'+owned+'\n'+choose+'\n'+dispatch+'}\n'
    access=(ROOT/'soh/soh/Enhancements/randomizer/location_access.cpp').read_text()
    code+='struct Region {bool timePass=true,childDay=false,childNight=false,adultDay=false,adultNight=false;bool TimePass();};\n'
    code+='static Rando::Logic trackerLogic;static auto*logic=&trackerLogic;\n'+function(access,'bool Region::TimePass(')+'\n'
    fill=(ROOT/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text()
    a=fill.index('        if (startingAgeTime == RAT_CHILD_DAY)');b=fill.index('\n    }\n    if (calculatingAvailableChecks)',a)
    code+='void NativeTrackerSeedTime(Region*region,RandoAgeTime startingAgeTime){'+fill[a:b]+'}\n'
    (Path(td)/'oot_native_functions.inc').write_text(code)
    flags=['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-I'+td]
    if os.environ.get('SANITIZE'): flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    for p in ('soh','soh/include','soh/src','soh/assets','combo','libultraship/include'):flags+=['-isystem',str(ROOT/p)]
    for combo in (False,True):
        binary=Path(td)/('combo' if combo else 'original')
        subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),str(Path(__file__).with_name('oot_native.cpp')),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True);print('PASS native OoT200x18 actual request/solver/clock flags/save',combo,flush=True)
