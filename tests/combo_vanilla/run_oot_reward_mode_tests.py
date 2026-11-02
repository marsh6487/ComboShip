#!/usr/bin/env python3
"""Compile real mode/ownership adapters; engine configuration and inventory are seams."""
from pathlib import Path
import argparse
import os
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
blocks += [function(forms, name) for name in [
    'extern "C" u8 RitoItem_FormEnabled(void)',
    'static bool GateOn(const CustomFormDef* def)', 'static const CustomFormDef* RitoRow()',
    'extern "C" void RitoItem_NoteCellItem(s32 item)', 'extern "C" void RitoItem_SyncCell(void)']]
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
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                    str(ROOT / 'tests/combo_vanilla/oot_rewards_mode_test.cpp'), '-o', str(temp / 'test')], check=True)
    subprocess.run([str(temp / 'test')], check=True)
