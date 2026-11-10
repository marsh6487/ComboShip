#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,tempfile,re
ROOT=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='song-event-native-') as td:
    source=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text();a=source.index('inline void to_json(json& j, const RandoSaveCheck&');b=source.index('inline void to_json(json& j, const Vec3f&',a)
    (Path(td)/'mm_save_json.inc').write_text('using json=nlohmann::json;\n'+source[a:b])
    message=(ROOT/'mm/src/code/z_message.c').read_text()
    a=message.index('            if ((sSongEventReservedSong >= 0')
    b=message.index('{',a);e=b+1;depth=1
    while depth:depth+=(message[e]=='{')-(message[e]=='}');e+=1
    capture=message[a:e]
    reservation=re.search(r'sSongEventReservedSong\s*=\s*SongEvents_MmInterceptSong\([^;]+;',message)
    if not reservation:raise RuntimeError('Native recognized-song reservation assignment missing')
    reserve='sLastPlayedSong = msgCtx->ocarinaStaff->state;\n'+reservation[0]

    fixture='static s16 sLastPlayedSong=0xff;\nstatic s16 sSongEventReservedSong=-1;\n'
    fixture+='void RecognizeNativeSong(PlayState* play,int song){auto* msgCtx=&play->msgCtx;OcarinaStaff staff{};staff.state=song;msgCtx->ocarinaStaff=&staff;msgCtx->lastPlayedSong=song;'+reserve+'msgCtx->ocarinaStaff=nullptr;}\n'
    fixture+='void CloseNativeSong(PlayState* play){auto* msgCtx=&play->msgCtx;'+capture+'}\n'
    (Path(td)/'mm_message_reservation.inc').write_text(fixture)
    pools=(ROOT/'mm/2s2h/Rando/Logic/GeneratePools.cpp').read_text()
    a=pools.index('void GeneratePools(');a=pools.index('{',a)+1
    b=pools.index('    const bool hasOotGameplay',a)
    (Path(td)/'mm_pool_song_mapping.inc').write_text('namespace Rando { void NativePoolSongMapping(RandoSaveInfo& saveInfo, bool preserveSongMapping) {'+pools[a:b]+'}}\n')
    runtime=(ROOT/'mm/2s2h/Rando/Rando.cpp').read_text()
    a=runtime.index('void OnSaveLoadHandler(');a=runtime.index('{',a)+1
    b=runtime.index('    Rando::MiscBehavior::OnFileLoad();',a)
    (Path(td)/'mm_song_save_load.inc').write_text('void NativeSongSaveLoadHandler() {'+runtime[a:b]+'}\n')

    # Native quest ownership builds the recognition mask; the actual message
    # start branch independently selects the current form's instrument.
    a=message.index('u16 sOcarinaSongFlagsMap[] =');b=message.index(';',a)+1
    native=message[a:b]+'\n'
    a=message.index('u8 sPlayerFormOcarinaInstruments[] =');b=message.index(';',a)+1
    native+=message[a:b]+'\n'
    a=message.index('    msgCtx->ocarinaAvailableSongs = 0;',message.index('void Message_DisplayOcarinaStaffImpl'))
    b=message.index('    // Skijer',a)
    native+='void NativeOwnedSongMask(PlayState*play){auto*msgCtx=&play->msgCtx;u32 i;'+message[a:b]+'}\n'
    a=message.index('                if (msgCtx->msgMode == MSGMODE_OCARINA_STARTING)')
    b=message.index('                } else if (msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION_STARTING)',a)
    native+='void NativeFormInstrumentStart(PlayState*play){auto*msgCtx=&play->msgCtx;'+message[a:b]+'}}\n'
    (Path(td)/'mm_native_instruments.inc').write_text(native)
    player=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    a=player.index('                if ((this->itemAction >= PLAYER_IA_MASK_MIN)',player.index('s32 Player_ActionHandler_13('))
    b=player.index('{',a);e=b+1;depth=1
    while depth:depth+=(player[e]=='{')-(player[e]=='}');e+=1
    parameter=(ROOT/'mm/src/code/z_parameter.c').read_text()
    table=re.search(r'^u8 gPlayerFormItemRestrictions\[PLAYER_FORM_MAX\]\[114\] = \{.*?^};',parameter,re.M|re.S)[0]
    native='#include "global.h"\nvoid func_808388B8(PlayState*,Player*,PlayerTransformation);\nvoid func_808389BC(PlayState*,Player*);\nvoid func_80838A20(PlayState*,Player*);\n'
    native+=table+'\nbool NativeMaskTransition(Player*this,PlayState*play){'+player[a:e]+'return false;}\n'
    (Path(td)/'mm_native_mask_transition.c').write_text(native)
    worldRoot=Path(os.environ.get('SONG_WORLD_SOURCE_ROOT',str(ROOT)))
    east=(worldRoot/'mm/2s2h/Rando/Logic/Regions/East.cpp').read_text()
    rules=[]
    for number in range(1,6):
        match=re.search(r'CHECK\(RC_IKANA_GRAVEYARD_ROCK_WALL_'+str(number)+r',\s*(.*)\),',east)
        if match:rules.append(match[1])
    if rules and len(rules)!=5:raise RuntimeError('Expected all five real Keeta rock rules')
    if not rules:print('SKIP Keeta environmental lambdas: absent in this isolated source; set SONG_WORLD_SOURCE_ROOT to integrated tree',flush=True)
    generated='#define SONG_KEETA_ROCK_COUNT '+str(len(rules))+'\n'
    generated+='static std::vector<std::function<bool()>> NativeKeetaRockRules(){return {'+','.join('[]{return '+rule+';}' for rule in rules)+'};}\n'
    # Execute the actual new environmental predicates with the native form
    # transition/table matrix, rather than modeling their Human requirement.
    for filename,check,name in (
        ('Central.cpp','RC_TERMINA_FIELD_SOIL_OBSERVATORY_ITEM_1','NativeObservatorySoilRule'),
        ('West.cpp','RC_GREAT_BAY_COAST_ROCK_WATER_01','NativeWaterRockRule'),
        ('West.cpp','RC_GREAT_BAY_COAST_ROCK_SAND_1','NativeSandRockRule')):
        source=(worldRoot/'mm/2s2h/Rando/Logic/Regions'/filename).read_text()
        rule=re.search(r'CHECK\('+check+r',\s*(.*)\),',source)
        if not rule:raise RuntimeError('Missing native environmental CHECK '+check)
        generated+='static bool '+name+'(){return '+rule[1]+';}\n'
    inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
    generated+='extern "C" {\n'+'\n'.join(re.search(r'^(?:u16|u8) '+name+r'\[\] = \{.*?^\};',inventory,re.M|re.S)[0] for name in ('gEquipMasks','gEquipShifts'))+'\n}\n'
    (Path(td)/'keeta_native_rules.inc').write_text(generated)
    flags=['-std=c++20','-I'+td,'-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-include',str(ROOT/'tests/shuffle_parity/souls/headless.h'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    if os.environ.get('SANITIZE'): flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    for p in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):flags+=['-isystem',str(ROOT/p)]
    policy=Path(td)/'policy'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(Path(__file__).with_name('policy.cpp')),'-o',str(policy)],check=True)
    subprocess.run([str(policy)],check=True);print('PASS deterministic policy/defaults/clock',flush=True)
    for combo in (False,True):
        binary=Path(td)/('combo' if combo else 'original')
        native=Path(td)/('mask-combo.o' if combo else 'mask-original.o')
        cflags=[flag for flag in flags if flag not in ('-std=c++20','-include',str(ROOT/'tests/shuffle_parity/souls/headless.h'))]
        subprocess.run([os.environ.get('CC','cc'),'-std=c11',*cflags,*(['-DCOMBO_BUILD'] if combo else []),'-c',str(Path(td)/'mm_native_mask_transition.c'),'-o',str(native)],check=True)
        subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),str(Path(__file__).with_name('mm_native.cpp')),str(ROOT/'mm/2s2h/Rando/SongEvents.cpp'),str(ROOT/'mm/2s2h/Rando/SoloEntrances.cpp'),str(native),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True);print('PASS real MM runtime/solver/JSON/oracle, native message reservations and actor state/range/form eligibility',combo,flush=True)
        if rules: print('PASS five actual integrated Keeta CHECK lambdas: STORMS assignment, SONATA negative, prerequisites and disabled defaults',combo,flush=True)
