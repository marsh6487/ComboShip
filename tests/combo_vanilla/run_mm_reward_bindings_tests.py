#!/usr/bin/env python3
"""Compile real MM reward adapter/native grant functions against native save headers."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]


def function(path, signature):
    source=(ROOT/path).read_text(encoding='utf-8-sig')
    start=source.index(signature);brace=source.index('{',start);end=brace+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


adapter='mm/2s2h/Enhancements/Gameplay/ComboVanillaRewards.cpp'
inventory='mm/mods/extended_inventory.c'
equipment='mm/mods/extended_equipment.c'
nei='mm/mods/nei_save.cpp'
cane='mm/mods/items/logic/item_cane_of_somaria.c'
bottles='mm/mods/items/custom_bottles.cpp'
twilight='mm/mods/items/logic/twilight_upgrade.c'
source=(ROOT/adapter).read_text()
binding=source[source.index('struct Binding {'):source.index('bool NativePrerequisite')]
production=binding+'\n'
for signature in ('bool NativePrerequisite','RandoItemId DrawIdentity','bool HasPresentation','int ItemSlot',
                  'bool Owns','bool Grant','bool Active'):
    production+=function(adapter,signature)+'\n'
native='''#define NEI_CANE_SKILL_MAX 6
#define NEI_CANE_TYPE_MAX 4
#define CANE_SKILL_MAX 6
#define CANE_SKILL_TYPE(skill) ((skill)/3)
#define CANE_SKILL_SLOT(skill) ((skill)%3)
#define CANE_SKILL_PACCI_STONE 4
#define BC_ITEM_BOTTLE 0x12
'''
for signature in ('extern "C" NeiSaveData* Nei_Save', 'extern "C" uint16_t Nei_GetOwnedItem',
                  'extern "C" void Nei_SetOwnedItem', 'extern "C" uint8_t Nei_CaneHasSkill',
                  'extern "C" void Nei_CaneGrantSkill', 'extern "C" uint8_t Nei_CaneOwned',
                  'extern "C" void Nei_CaneSetType', 'static uint8_t Nei_SomariaSummonOwned',
                  'extern "C" void Nei_CaneSetSkillSlot'):
    native+=function(nei,signature)+'\n'
native+='extern "C" '+function(cane,'u8 Cane_GiveSkill')+'\n'
for signature in ('uint8_t Wand_RandoMode','void Wand_GrantMode','void Slate_GrantRune',
                  'void Seasons_GrantSeason','void TravelTools_Heal','void TravelTools_Grant',
                  'void GraceHourglass_Heal','void GraceHourglass_Grant'):
    native+=function(inventory,signature)+'\n'
for signature in ('static u32 ExtEquip_GetBit','u8 ExtEquip_HasItem','void ExtEquip_GiveItem',
                  'u8 ExtEquip_CapeOwned','void ExtEquip_GiveCape'):
    native+=function(equipment,signature)+'\n'
for signature in ('extern "C" uint8_t Bottle_NetOwned','extern "C" void Bottle_SetNetOwned',
                  'extern "C" uint8_t Bottle_BottomlessOwned','extern "C" void Bottle_SetBottomlessOwned',
                  'extern "C" void Bottle_BottomlessEmpty'):
    native+=function(bottles,signature)+'\n'
for signature in ('u8 TwilightUpgrade_IsFullyObtained','void TwilightUpgrade_Grant'):
    native+=function(twilight,signature)+'\n'
inventory_source=(ROOT/'mm/src/code/z_inventory.c').read_text()
items=re.search(r'u8 gItemSlots\[77\] = \{.*?\n\};',inventory_source,re.S)[0]
# The real wallet prerequisite expands CUR_UPG_VALUE; keep its native masks and
# shifts alongside the already extracted native inventory-slot table.
for declaration in ('u32 gUpgradeMasks[]', 'u8 gUpgradeShifts[]'):
    items+='\n'+re.search(re.escape(declaration)+r' = \{.*?\n\};',inventory_source,re.S)[0]
fixture=(ROOT/'tests/combo_vanilla/mm_rewards_adapter_test.cpp').read_text()
fixture=fixture.replace('/* ITEM_SLOTS */',items).replace('/* NATIVE_GRANTS */',native)
fixture=fixture.replace('/* ADAPTER_FUNCTIONS */',production)
fixture=fixture.replace('/* TRANSACTION_FUNCTION */',function(adapter,'bool Transact'))
fixture=fixture.replace('/* MIRROR_CLAIM */',function(adapter,'extern "C" int MM_VanillaClaimMirror(void) {'))
fixture=fixture.replace('/* MIRROR_FINISH */',function('mm/2s2h/Enhancements/Gameplay/MagicMirror.cpp','bool FinishVanillaPickup(int token) {'))
with tempfile.TemporaryDirectory(prefix='vanilla-mm-bindings-') as tmp:
    cpp=Path(tmp)/'test.cpp';cpp.write_text(fixture)
    flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    for path in ('','mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
        flags+=['-I'+str(ROOT/path)]
    if '--sanitize' in sys.argv:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    binary=Path(tmp)/'test'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
