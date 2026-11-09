#!/usr/bin/env python3
"""Compile real MM save/logic macros, native ammo and contiguous refill branches.

The refill test includes Item_GiveImpl's entry guard and its six native bag-ammo
branches. Other Item_Give families and the configured player loop are outside
this fixture, and remain production-TU/runtime integration checks.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/shuffle_parity/boss_keys'))
from run_tests import function

parameter = (ROOT/'mm/src/code/z_parameter.c').read_text()
inventory = (ROOT/'mm/src/code/z_inventory.c').read_text()
actor = (ROOT/'mm/src/code/z_actor.c').read_text()
tables = '\n'.join(re.search(r'^(?:u32|u16|u8) '+name+r'\[[^]]*\](?:\[4\])? = \{.*?^\};',inventory,re.M|re.S)[0]
                   for name in ('gUpgradeMasks','gUpgradeNegMasks','gUpgradeShifts','gUpgradeCapacities','gItemSlots'))
give = function(parameter,'Item_GiveImpl')
prefix = give[give.index('{')+1:give.index('    slot = SLOT(item);')]
start = give.index('    } else if (item == ITEM_DEKU_STICK) {')
end = give.index('    } else if (item == ITEM_POWDER_KEG)', start)
refills = 'u8 NativeRefill(PlayState* play,u8 item) {'+prefix+'    if'+give[start+len('    } else if'):end]+'''    }
    INV_CONTENT(item) = item;
    return ITEM_NONE;
}
'''
counts = re.search(r'^s16 sAmmoRefillCounts\[\] = \{[^}]*\};',parameter,re.M)
assert counts, 'native ammo refill table missing'
services = tables+'\n'+counts[0]+'\n'+ '\n'.join(function(actor,n) for n in (
    'Flags_GetRandoInf','Flags_SetRandoInf','Flags_ClearRandoInf'))+'\n'+function(inventory,'Inventory_ChangeUpgrade')+'\n'+refills+'\n'+function(parameter,'Inventory_ChangeAmmo')+'\n'+function(parameter,'Rupees_ChangeBy')
# Flag notifications and the unrelated powder-keg cap are engine boundaries.
services = '''#include "global.h"
#include "variables.h"
#include "functions.h"
#include "2s2h/GameInteractor/GameInteractor.h"
void GameInteractor_ExecuteOnFlagSet(FlagType,u32) {}
void GameInteractor_ExecuteOnFlagUnset(FlagType,u32) {}
bool GameInteractor_Should(GIVanillaBehavior,uint32_t result,...) {return result;}
'''+services
with tempfile.TemporaryDirectory(prefix='mm-bag-runtime-') as directory:
    build = Path(directory)
    (build/'bag_services.c').write_text(services)
    conversions = (ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text()
    a = conversions.index('inline void to_json(json& j, const RandoSaveCheck&')
    b = conversions.index('inline void to_json(json& j, const Vec3f&', a)
    (build/'bag_json.inc').write_text(conversions[a:b])
    starting = (ROOT/'mm/2s2h/Rando/StartingItems.cpp').read_text()
    (build/'bag_starting.inc').write_text('\n'.join(function(starting,n) for n in (
        'GetComputedStartingItems','GetStartingItemsFromSave','SetStartingItemsInSave',
        'GetStartingItemsFromSpoiler','SetStartingItemsInSpoiler','GrantStartingItems')))
    bridge = (ROOT/'mm/2s2h/BenPort.cpp').read_text()
    (build/'wallet_flush.inc').write_text(function(bridge,'Combo_MM_FlushAccumulators'))
    (build/'bag_bridge.inc').write_text(function(bridge,'Combo_MM_SpoilerNameToItemId')+'\n'+
                                      function(bridge,'GrantMmItemByName'))
    conversion = (ROOT/'mm/2s2h/Rando/ConvertItem.cpp').read_text()
    obtainable = function(conversion[conversion.index('bool Rando::IsItemObtainable('):],'Rando::IsItemObtainable')
    a = obtainable.index('        case RI_PROGRESSIVE_WALLET:')
    b = obtainable.index('        case RI_WALLET_ADULT:',a)
    resolve = function(conversion[conversion.index('RandoItemId Rando::ConvertItem('):],'Rando::ConvertItem')
    c = resolve.index('            case RI_PROGRESSIVE_WALLET:')
    d = resolve.index('            case RI_PROGRESSIVE_SWORD:',c)
    (build/'wallet_convert.inc').write_text(
        'bool NativeWalletObtainable(bool hasObtainedCheck) { switch(RI_PROGRESSIVE_WALLET) {\n'+
        obtainable[a:b]+'\n default:return false; } }\n'+
        'RandoItemId NativeWalletResolve() { switch(RI_PROGRESSIVE_WALLET) {\n'+
        resolve[c:d]+'\n default:return RI_JUNK; } }\n')
    def call(text, signature):
        start = text.index(signature)
        end = text.index('(',start) + 1
        depth = 1
        while depth:
            depth += (text[end] == '(') - (text[end] == ')')
            end += 1
        return text[start:end]
    east = (ROOT/'mm/2s2h/Rando/Logic/Regions/East.cpp').read_text()
    central = (ROOT/'mm/2s2h/Rando/Logic/Regions/Central.cpp').read_text()
    rules = 'feeChecks.checks.insert({'+call(east,'CHECK(RC_ENEMY_DROP_POE_SISTER,')+'});\n'
    rules += 'std::map<RandoEvent,std::function<bool()>> playgroundEvents={\n'+',\n'.join(
        call(central,'EVENT(RE_DEKU_PLAYGROUND_'+str(i)+',') for i in range(1,4))+'\n};\n'
    (build/'wallet_fee_rules.inc').write_text(rules)
    flags = ['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
             '-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT/'tests/shuffle_parity/souls/headless.h'),
             '-I'+str(ROOT),'-I'+str(build),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
        flags += ['-isystem',str(ROOT/path)]
    if '--sanitize' in sys.argv:
        flags += ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    for combo in (False, True):
        binary = build/('combo' if combo else 'original')
        native = build/('combo.o' if combo else 'original.o')
        cflags = [f for f in flags if f not in ('-std=c++20','-include',str(ROOT/'tests/shuffle_parity/souls/headless.h'))]
        subprocess.run([os.environ.get('CC','cc'),'-std=c11',*cflags,*(['-DCOMBO_BUILD'] if combo else []),
                        '-c',str(build/'bag_services.c'),'-o',str(native)],check=True)
        fixture = 'wallet_runtime_test.cpp' if '--wallet-only' in sys.argv else 'runtime_test.cpp'
        catalogues = [] if '--wallet-only' in sys.argv else [
            str(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp'),str(ROOT/'mm/2s2h/Rando/StaticData/Options.cpp')]
        subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),
                        str(Path(__file__).with_name(fixture)),*catalogues,
                        str(native),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        print('PASS native bag runtime', 'Combo' if combo else 'original',flush=True)
