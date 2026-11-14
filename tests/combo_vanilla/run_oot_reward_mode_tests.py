#!/usr/bin/env python3
"""Compile real mode/ownership adapters; engine configuration and inventory are seams."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()

def function(path, signature):
    source = (ROOT / path).read_text()
    start = source.index(signature)
    begin = source.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

inventory = 'soh/mods/extended_inventory.c'
forms = 'soh/mods/transformation_masks/custom_forms.cpp'
blocks = [function(inventory, name) for name in [
    'bool ExtInv_IsCustomItemsEnabled(void)', 'bool ExtInv_IsMmMasksEnabled(void)',
    'bool ExtInv_IsOnlyTransformation(void)',
    'uint8_t Wand_RandoMode(void)', 'uint8_t BombArrows_RandoMode(void)']]
blocks += [function('soh/mods/extended_equipment.c', 'u8 ExtEquip_IsEnabled(void)')]
# Migration now folds the unified trade selection and chooses a real mask
# neighbor. Keep those dependencies verbatim; only engine mode/flag queries
# are supplied by the fixture, which exercises Vanilla and seeded modes.
trade = (ROOT / 'soh/mods/items/logic/trade_items.c').read_text()
equipment = (ROOT / 'soh/mods/extended_equipment.h').read_text()
trade = trade[trade.index('#define TRADE_ADULT_COUNT'):trade.index('extern void* MmAssets_LoadResource')]
trade_declarations = re.findall(r'^(?:s32|u8|void) \w+\([^{};]*\) (?=\{)', trade, re.M)
blocks += ['extern "C" {\n' + ';\n'.join(trade_declarations) + ';\n}']
# The reward fixture owns its inventory/equipment storage. Keep the unchanged
# reconciler in that TU while the remaining helpers keep native C enum rules.
reconcile = function('soh/mods/items/logic/trade_items.c', 'void TradeAdult_ReconcileConsumed(void)')
blocks += [re.search(r'^#define TRADE_ADULT_PENDANT\s+.*$', trade, re.M).group(),
           'extern "C" ' + reconcile]
trade_source = ('#include "soh/include/z64item.h"\n#include "soh/mods/nei_save.h"\n'
                'typedef uint8_t u8;\ntypedef int32_t s32;\n' +
                next(line for line in equipment.splitlines() if line.startswith('#define ITEM_EXT_BOOTS_2 ')) +
                '\nvoid TradeAdult_ReconcileConsumed(void);\n' + trade.replace(reconcile, '', 1))
blocks += [function(forms, name) for name in [
    'extern "C" uint8_t RitoMask_IsOwned(void)',
    'extern "C" u8 RitoItem_FormEnabled(void)',
    'static bool GateOn(const CustomFormDef* def)',
    'extern "C" void RitoItem_NoteCellItem(s32 item)',
    'static uint8_t OotMaskWheel_OwnsItem(s32 item, uint8_t allowVanillaMasks)',
    'extern "C" uint8_t OotMaskWheel_Neighbor(int32_t direction, uint8_t allowVanillaMasks)',
    'extern "C" void RitoMask_MigrateLegacy(void)',
    'extern "C" void RitoItem_SyncCell(void)']]
blocks += [function('soh/soh/Enhancements/randomizer/RocsFeatherCycle.c', name) for name in [
    'void VanillaRocsFeather_Grant(void)', 'static uint8_t VanillaRocsOtherItem(void)', 'uint8_t Enhancement_GetNextNayrusItem()',
    'uint8_t Enhancement_GetPrevNayrusItem()']]
blocks += [function('soh/mods/nei_save.cpp', name) for name in [
    'extern "C" uint8_t Nei_CaneHasSkill(uint8_t skill)',
    'extern "C" void Nei_CaneGrantSkill(uint8_t skill)', 'extern "C" uint8_t Nei_CaneOwned(void)',
    'static uint8_t Nei_CaneTypeGateBit(uint8_t type)', 'extern "C" uint8_t Nei_CaneTypeOwned(uint8_t type)',
    'extern "C" uint8_t Nei_CaneTypeVisible(uint8_t type)', 'extern "C" uint8_t Nei_CaneGetType(void)',
    'extern "C" void Nei_CaneSetType(uint8_t type)', 'static uint8_t Nei_SomariaSummonOwned(uint8_t slot)',
    'extern "C" uint8_t Nei_CaneGetSkillSlot(uint8_t type)',
    'extern "C" void Nei_CaneSetSkillSlot(uint8_t type, uint8_t slot)',
    'extern "C" uint8_t Nei_CaneActiveSkill(void)']]
blocks += [function('soh/mods/items/logic/item_cane_of_somaria.c', 'u8 Cane_GiveSkill(u8 skill)')]
blocks += [function('soh/mods/items/logic/item_cane_of_somaria.c', name) for name in [
    'u8 Cane_HasSkill(u8 skill)', 'u8 Cane_GetType(void)',
    'static u8 Cane_SummonSlotOwned(u8 slot)',
    'static void Cane_CycleSummon(Player* p, PlayState* play, s8 dir)']]
blocks += [function('soh/mods/transformation_masks/mm_player_form.cpp', 'u8 MmForm_IsEnabled(void)')]
with tempfile.TemporaryDirectory(prefix='vanilla-oot-modes-') as directory:
    temp = Path(directory)
    (temp / 'oot_reward_modes.inc').write_text('\n'.join(blocks))
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-DCOMBO_BUILD', '-I', str(ROOT),
             '-I', str(ROOT / 'soh'), '-I', str(temp)]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
    # These are C helpers in the native unity build. Compile them as C to
    # preserve their enum conversion rules while keeping -Wextra -Werror.
    (temp / 'trade.c').write_text(trade_source)
    cflags = [flag for flag in flags if not flag.startswith('-std=')]
    subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), '-std=gnu11', *cflags,
                    '-c', str(temp / 'trade.c'), '-o', str(temp / 'trade.o')], check=True)
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                    str(ROOT / 'tests/combo_vanilla/oot_rewards_mode_test.cpp'), str(temp / 'trade.o'),
                    '-o', str(temp / 'test')], check=True)
    subprocess.run([str(temp / 'test')], check=True)
