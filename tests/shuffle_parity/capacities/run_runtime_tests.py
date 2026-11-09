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
magic_globals = '\n'.join(re.search(r'^s16 '+name+r' = [^;]*;',parameter,re.M)[0] for name in (
    'sMagicMeterOutlinePrimRed','sMagicMeterOutlinePrimGreen','sMagicMeterOutlinePrimBlue','sMagicBorderRatio'))
icon_tables = '\n'.join(re.search(r'^'+declaration+r' = \{.*?^\};',source,re.M|re.S)[0] for source,declaration in (
    (inventory,r'TexturePtr gItemIcons\[\]'),
    ((ROOT/'mm/src/code/z_message.c').read_text(),r's16 D_801CFF94\[\]')))
icon_tables = re.search(r'^#define MESSAGE_ITEM_NONE .+$',(ROOT/'mm/src/code/z_message.c').read_text(),re.M)[0]+'\n'+icon_tables
icon_tables = ('#include "interface/parameter_static/parameter_static.h"\n#include "archives/icon_item_static/icon_item_static_yar.h"\n#include "archives/icon_item_24_static/icon_item_24_static_yar.h"\n'+icon_tables) if '--infinite-integration' in sys.argv else ''
services = icon_tables+'\n'+tables+'\n'+counts[0]+'\n'+magic_globals+'\n'+ '\n'.join(function(actor,n) for n in (
    'Flags_GetRandoInf','Flags_SetRandoInf','Flags_ClearRandoInf','Flags_SetWeekEventReg'))+'\n'+function(inventory,'Inventory_ChangeUpgrade')+'\n'+refills+'\n'+function(parameter,'Inventory_ChangeAmmo')+'\n'+function(parameter,'Rupees_ChangeBy')+'\n'+function(parameter,'Magic_Consume')+'\n'+function(parameter,'Magic_Update')
equipment = (ROOT/'mm/mods/items/helpers/equip_helper.c').read_text()
services += '\n#ifndef MAGIC_REQ\n#define MAGIC_REQ(amount) (amount)\n#endif\n'
services += '\n'.join(function(equipment,n) for n in ('ItemMagic_IsInfinite','ItemMagic_Consume','ItemMagic_HasEnough'))
# Actual player-arrow/spell affordability expressions, with their real cost tables.
player = (ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
spells = (ROOT/'mm/mods/items/logic/item_oot_spells.c').read_text()
for source,table,index,name in ((player,'sMagicArrowCosts','magicArrowType','NativeArrowAffordable'),
                               (spells,'sOotMagicSpellCosts','magicSpell','NativeSpellAffordable')):
    services += '\n'+re.search(r'(?:static )?u8 '+table+r'\[\] = \{.*?\};',source,re.S)[0]
    expression = re.search(r'RandoCapacity_CanSpendMagic\(MAGIC_REQ\('+table+r'\['+index+r'\]\)\)',source)[0]
    services += '\nint '+name+'(int '+index+') { return '+expression+'; }\n'

# Execute the exact production direct-deduction blocks. Actor spawning, collision,
# input timing and visual callbacks remain outside this bounded fixture.
remains = (ROOT/'mm/mods/boss_remains/boss_remains.cpp').read_text()
for name in ('kGohtThunderMagicCost','kOdolwaMothMagicCost'):
    services += '\n'+re.search(r'(?:static )?constexpr s16 '+name+r' = [^;]+;',remains)[0].replace('constexpr','const')
blocks = []
for path in ('mm/mods/boss_remains/boss_remains.cpp','mm/mods/equipment/behaviors/equip_pegasus.c','mm/mods/actors/z_en_partner.inc.c'):
    source = (ROOT/path).read_text()
    found = re.findall(r'if \(!RandoCapacity_InfiniteMagic\(\)\) \{\s*gSaveContext.save.saveInfo.playerData.magic(?:--| -= [^;]+);\s*\}',source)
    assert len(found) == (5 if 'boss_remains' in path else 1 if 'pegasus' in path else 2), path
    blocks += found
for i,block in enumerate(blocks):
    services += '\nvoid NativeDirectMagic'+str(i)+'(void) { '+block+' }\n'
trident = (ROOT/'mm/mods/equipment/behaviors/equip_trident.c').read_text()
services += '\n'+re.search(r'^#define TRI_BALL_MAGIC_COST .+$',trident,re.M)[0]+'\n'
services += function(trident,'Trident_PayBallMagic').replace('static void','void')

wallet_hud = (ROOT/'mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp').read_text()
hud_checks = re.findall(r'if \((CUR_CAPACITY\(UPG_WALLET\) >= 1000)\)',wallet_hud)
assert len(hud_checks) == 2
hud_registration = re.search(r'bool shouldRegister = ([^;]+);',wallet_hud)[1]
services += '\n'+re.search(r'^#define IS_RANDO .+$',(ROOT/'mm/2s2h/Rando/Rando.h').read_text(),re.M)[0]+'\nint NativeWalletHudRegistered(void) { return '+hud_registration+'; }\n'
assert 'if (shouldRegister && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES)' in wallet_hud

services += '\nint NativeWalletFourDigits(void) { return '+hud_checks[0]+'; }\n'

# Native accumulator paths and exact synchronous purchase charge guards.
a = parameter.index('    // Imported or pending debits cannot consume an owned Infinite Wallet.')
b = parameter.index('    // Update perfect letters',a)
services += '\nvoid NativeRupeeTick(void) { '+parameter[a:b]+' }\n'
faster = (ROOT/'mm/2s2h/Enhancements/Timesavers/FasterRupeeAccumulator.cpp').read_text()
a = faster.index('[]() {') + len('[]() {')
b = faster.index('    });',a)
services += '\nvoid NativeFasterRupeeTick(void) { '+faster[a:b]+' }\n'
for i,path in enumerate(('mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp','mm/2s2h/Enhancements/Gameplay/ComboVanillaRewards.cpp')):
    source = (ROOT/path).read_text()
    charge = re.search(r'if \(!RandoCapacity_InfiniteWallet\(\)\) \{\s*gSaveContext.save.saveInfo.playerData.rupees -= [^;]+;\s*\}',source)[0]
    services += '\nvoid NativePurchaseCharge'+str(i)+'(int price) { struct { int price; } sReceipt = {price}; '+charge+' }\n'

# Flag notifications and the unrelated powder-keg cap are engine boundaries.
services = '''#include "global.h"
#include "variables.h"
#include "functions.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
void GameInteractor_ExecuteOnFlagSet(FlagType,u32) {}
void GameInteractor_ExecuteOnFlagUnset(FlagType,u32) {}
bool GameInteractor_Should(GIVanillaBehavior,uint32_t result,...) {return result;}
void Audio_PlaySfx(u16 id) {}
bool Play_InCsMode(PlayState* play) {return false;}
s32 Player_GetEnvironmentalHazard(PlayState* play) {return 0;}
u8 ExtEquip_CapeOwned(void) {return 0;}
int32_t CVarGetInteger(const char* name,int32_t fallback) {return fallback;}
int16_t ComboRpg_MagicCapacity(int16_t native) {return native;}
void Magic_FlashMeterBorder(void) {}
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
    infinite = obtainable[obtainable.index('    if (const auto* upgrade = InfiniteUpgrades::Find'):obtainable.index('    if (randoItemId == RI_WALLET_CHILD)')]
    (build/'infinite_obtainable.inc').write_text('bool NativeInfiniteObtainable(RandoItemId randoItemId,bool hasObtainedCheck) {\n'+infinite+'\nreturn false; }\n')
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
        fixture = ('infinite_integration_test.cpp' if '--infinite-integration' in sys.argv else
                   'infinite_runtime_test.cpp' if '--infinite-only' in sys.argv else
                   'wallet_runtime_test.cpp' if '--wallet-only' in sys.argv else 'runtime_test.cpp')
        catalogues = [] if '--wallet-only' in sys.argv or '--infinite-only' in sys.argv else [
            str(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp'),str(ROOT/'mm/2s2h/Rando/StaticData/Options.cpp')]
        subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),
                        str(Path(__file__).with_name(fixture)),*catalogues,
                        str(native),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        print('PASS native bag runtime', 'Combo' if combo else 'original',flush=True)
