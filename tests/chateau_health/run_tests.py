#!/usr/bin/env python3
"""Execute each game's native Chateau drink action, magic policy and bottle core."""
from pathlib import Path
import os,re,subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    m=re.search(r'^(?:extern "C" )?(?:static )?(?:void|s32|u8|int) '+name+r'\([^;{}]*\)\s*\{',source,re.M)
    assert m,name
    end=m.end();depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[m.start():end]
for host in ('soh','mm'):
    fixture=(ROOT/'tests/chateau_health/drink_test.cpp').read_text()
    header=(ROOT/host/'include/z64player.h').read_text()
    start=header.index('typedef enum PlayerItemAction {');end=header.index('} PlayerItemAction;',start)+len('} PlayerItemAction;')
    fixture=fixture.replace('/* NATIVE_ACTIONS */',header[start:end])
    player=(ROOT/host/'src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    name='Player_Action_67' if host=='mm' else 'Player_Action_8084EAC0'
    drink=function(player,name)
    if host=='mm':
        start=player.index('u8 D_8085D790[] =');end=player.index('};',start)+2
        drink=player[start:end]+'\n'+drink
    fixture=fixture.replace('/* NATIVE_DRINK */',re.sub(r'\bthis\b','player',drink))
    if host=='soh':
        # Execute the actual cutscene-item input eligibility body. Its action
        # value is the production Chateau -> blue-potion mapping already
        # checked by run_combo_reward_delivery_tests.py.
        start=player.index('// Handle "cutscene items"')
        # Select the inner branch with balanced braces, preserving its native
        # hostile-target and bottle-range predicate exactly.
        begin=player.index('if (!Player_CheckHostileLockOn',start)
        opening=player.index('{',begin);end=opening+1;depth=1
        while depth:
            depth+=(player[end]=='{')-(player[end]=='}');end+=1
        entry=player[begin:end]
        fixture=fixture.replace('/* NATIVE_ENTRY */','void NativeDrinkEntry(Player* player,PlayState* play,int itemAction){'+re.sub(r'\bthis\b','player',entry)+'}')
        fixture=fixture.replace('/* NATIVE_GRANT */','')
    else:
        give=(ROOT/'mm/2s2h/Rando/GiveItem.cpp').read_text()
        start=give.index('case RI_BOTTLE_CHATEAU_ROMANI:')+len('case RI_BOTTLE_CHATEAU_ROMANI:')
        arm=give[start:give.index('break;',start)]
        fixture=fixture.replace('/* NATIVE_GRANT */','void GrantFilledChateau(){'+arm+'}')
        fixture=fixture.replace('/* NATIVE_ENTRY */','')
    fixture=fixture.replace('/* RUN_NATIVE_DRINK */',name+'(&player,&play);')
    magic=(ROOT/host/'mods/items/helpers/equip_helper.c').read_text()
    fixture=fixture.replace('/* NATIVE_MAGIC */','\n'.join(function(magic,n) for n in ['ItemMagic_IsInfinite','ItemMagic_Consume','ItemMagic_HasEnough']))
    if host=='soh':
        wear=(ROOT/'soh/mods/transformation_masks/mm_mask_wear.cpp').read_text()
        fixture=fixture.replace('/* NATIVE_BUFF */','static s32 sChateauRomaniActive=0,sChateauDidSetFlag=0;\n'+'\n'.join(function(wear,n) for n in ['MmMaskWear_ActivateChateauRomani','MmMaskWear_DeactivateChateauRomani']))
    else:fixture=fixture.replace('/* NATIVE_BUFF */','')
    with tempfile.TemporaryDirectory(prefix='chateau-health-') as directory:
        p=Path(directory);(p/'test.cpp').write_text(fixture)
        flags=['-DHOST_MM'] if host=='mm' else []
        cmd=['c++','-std=c++20','-g','-fsanitize=address,undefined,float-cast-overflow','-fno-sanitize-recover=all','-fno-pie','-no-pie','-I'+str(ROOT/host),'-I'+str(ROOT),*flags,str(p/'test.cpp'),str(ROOT/host/'mods/items/custom_bottles.cpp'),'-o',str(p/'test')]
        if '--admission-only' not in sys.argv:
            subprocess.run(cmd,check=True)
            subprocess.run([str(p/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        source=(ROOT/host/'src/code/z_parameter.c').read_text()
        name='Magic_Consume' if host=='mm' else 'Magic_RequestChange'
        native=function(source,name)
        enum_source=(ROOT/host/'include/z64save.h').read_text()
        enums=[]
        for match in re.finditer(r'typedef\s+enum[^\{]*\{[\s\S]*?\}\s*\w+\s*;',enum_source):
            if 'MAGIC_STATE_IDLE' in match[0] or 'MAGIC_CONSUME_NOW' in match[0]:enums.append(match[0])
        admission=(ROOT/'tests/chateau_health/admission_test.cpp').read_text().replace('/* NATIVE_ENUMS */','\n'.join(enums)).replace('/* NATIVE_REQUEST */',native)
        types=list(dict.fromkeys(re.findall(r'case (MAGIC_[A-Z_]+):',native)))
        types=[v for v in types if v!='MAGIC_ADD']
        admission=admission.replace('/* REQUEST_TYPES */',','.join(types))
        (p/'admission.cpp').write_text(admission)
        command=[v if v!=str(p/'test.cpp') else str(p/'admission.cpp') for v in cmd]
        command=[v for v in command if v!=str(ROOT/host/'mods/items/custom_bottles.cpp')]
        subprocess.run(command,check=True)
        subprocess.run([str(p/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
