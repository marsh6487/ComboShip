#!/usr/bin/env python3
"""Independent OoT identity/ownership contract; compile actual producer in a fixture."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
ENEMIES='STALFOS OCTOROK WALLMASTER DODONGO KEESE TEKTITE LEEVER PEAHAT LIZALFOS_DINOLFOS GOHMA_LARVA SHABOM BABY_DODONGO DARK_LINK BIRI_BARI TAILPASARN PARASITE SKULLTULA TORCH_SLUG MOBLIN ARMOS DEKU_BABA DEKU_SCRUB BUBBLE BEAMOS FLOORMASTER POE REDEAD_GIBDO SKULLWALLTULA FLARE_DANCER DEAD_HAND SHELL_BLADE LIKE_LIKE SPIKE ANUBIS IRON_KNUCKLE SKULL_KID FLYING_POT FREEZARD STINGER WOLFOS STALCHILD GUAY THIEVES'.split()
NPCS='SARIA DARUNIA RUTO KING_ZORA ZELDA SHEIK ANJU CARPENTERS GURU_GURU MIDO KOKIRI KOKIRI_SHOPKEEPER HYLIAN_GUARD CITIZEN MALON TALON FISHING_POND_OWNER GORON MEDIGORON BIGGORON GORON_CHILD GORON_SHOPKEEPER ZORA ZORA_SHOPKEEPER BAZAAR_SHOPKEEPER BOMBCHU_SHOPKEEPER POTION_SHOPKEEPER BOMBCHU_BOWLING_LADY CHEST_GAME_OWNER SHOOTING_GALLERY_OWNER DAMPE BOMBERS POE_COLLECTOR COMPOSER_BROS HONEY_DARLING ASTRONOMER ROOFTOP_MAN BEAN_SALESMAN SCIENTIST GORMAN GROG DOG_LADY CARPET_MAN OLD_HAG BANKER THIEVES'.split()
assert len(ENEMIES)==43 and len(NPCS)==46
header=ROOT/'combo/menu/ComboSoulMiniOOT.h'
assert header.exists(), 'RED: OoT owner miniature catalog absent (43 enemies + 46 NPCs)'
source=header.read_text()
for family,keys in [('ENEMY',ENEMIES),('NPC',NPCS)]:
    for key in keys:
        assert f'RG_SOUL_{family}_{key}' in source, (family,key)
assert 'ComboSoulModels::SimplerEnemy()' in source
assert 'ComboSoulModels::SimplerNpc()' in source
print('PASS independent 43 enemy / 46 NPC coverage and category toggle contract')

import tempfile, subprocess, sys, json
with tempfile.TemporaryDirectory(prefix='oot-soul-catalog-') as directory:
    temp=Path(directory)
    bridge=temp/'libultraship/bridge/consolevariablebridge.h'
    bridge.parent.mkdir(parents=True)
    bridge.write_text('#pragma once\nint CVarGetInteger(const char*, int);\n')
    binary=temp/'catalog'
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-I'+str(temp),'-I'+str(ROOT),str(ROOT/'tests/soul_mini/oot_catalog_test.cpp'),
                    '-o',str(binary)],check=True)
    result=subprocess.check_output([str(binary),'--dump'],text=True)
    recipes=json.loads(result)
    expected=json.loads((ROOT/'tests/soul_mini/oot_expected_objects.json').read_text())
    for family,keys in [('ENEMY',ENEMIES),('NPC',NPCS)]:
        for key,obj in zip(keys,expected[family],strict=True):
            identity='RG_SOUL_'+family+'_'+key
            recipe=next(r for r in recipes if r['id']==identity)
            body=recipe['skelPath'] or recipe['parts'][0]['dlPath']
            assert body.startswith('__OTR__objects/'+obj+'/'), (identity,body,obj)
    print('PASS independent native object representatives for all 89 identities')
    if '--archive' in sys.argv:
        import zipfile, struct
        archive=zipfile.ZipFile(sys.argv[sys.argv.index('--archive')+1])
        checked=0
        for recipe in recipes:
            for path in recipe['paths']:
                if path.endswith('/gBossSoulFlameDL'):
                    continue # Combo-owned source resource generated outside stock OoT archive.
                assert path[7:] in archive.namelist(), (recipe['id'],path)
            if not recipe['skelPath']:
                continue
            skeleton=archive.read(recipe['skelPath'][7:])
            assert skeleton[4:8] == b'LKSO'
            count=struct.unpack_from('<I',skeleton,66)[0]
            assert count+1==recipe['limbCount'],recipe['id']
            assert (skeleton[64] == 0) == bool(recipe['nonFlexSkeleton']),recipe['id']
            if recipe['animPath']:
                anim=archive.read(recipe['animPath'][7:])
                assert anim[4:8] == b'MNAO' and struct.unpack_from('<I',anim,64)[0]==0
                frames=struct.unpack_from('<h',anim,68)[0]
                values=struct.unpack_from('<I',anim,70)[0]
                offset=74+values*2
                joints=struct.unpack_from('<I',anim,offset)[0]
                assert joints>=count+1,(recipe['id'],joints,count)
                static=struct.unpack_from('<H',anim,offset+4+joints*6)[0]
                assert frames>0
                for index in struct.unpack_from('<'+'H'*(joints*3),anim,offset+4):
                    assert index+(frames-1 if index>=static else 0)<values,recipe['id']
            elif recipe['posePath']:
                pose=archive.read(recipe['posePath'][7:])
                assert pose[4:8] == b'MAPO'
                entries=struct.unpack_from('<I',pose,64)[0]
                assert entries>=recipe['limbCount']*3
            checked+=1
        print('PASS actual OoT archive paths,',checked,'native rigs, matching skeleton kinds and animation index bounds')
    if '--dump' in sys.argv:
        destination=Path(sys.argv[sys.argv.index('--dump')+1])
        destination.write_text(result)
        print('Wrote',destination)
    else:
        subprocess.run([str(binary)],check=True)

producer=(ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
draw=(ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
assert 'ComboSoulMiniOOT::UsesDetailed((int32_t)rg)' in producer
assert 'return OOT_GetSoulMiniDrawInfo((int32_t)it->second, out)' in producer
assert 'ComboSoulMiniOOT::Describe(itemId, out)' in producer
assert 'out->stateDependent = 2; // Live enemy/NPC simpler preference' in producer
assert 'entry->getItemId == RG_ICE_TRAP ? entry->drawItemId : entry->getItemId' in draw
assert 'ComboForeignAnim_Draw(&info, "oot", play)' in draw
assert '#ifdef COMBO_BUILD\n    if (entry)' in draw
imported=draw[draw.index('void Randomizer_DrawMmSoul('):]
assert imported.index('ComboSoulModels::SimplerEnemy()') < imported.index('MmAssets_IsAvailable()')
assert 'if (!bossSoul && ComboSoulModels::SimplerEnemy())' in imported
print('PASS native/foreign descriptor routing, disguised soul identity and original-build guards')
