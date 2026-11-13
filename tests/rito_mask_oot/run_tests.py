#!/usr/bin/env python3
"""Run production Rito ownership, existing-wheel, and migration with real IDs."""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    match = re.search(r'^(?:(?:extern "C"|static|inline)\s+)*(?:const CustomFormDef\*|uint16_t|uint8_t|u8|s32|int|bool|void)\s+' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise AssertionError('Missing production function: ' + name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

p = argparse.ArgumentParser()
p.add_argument('--sanitize', action='store_true')
args = p.parse_args()
forms = (ROOT / 'soh/mods/transformation_masks/custom_forms.cpp').read_text()
inv = (ROOT / 'soh/mods/extended_inventory.c').read_text()
controller = (ROOT / 'soh/mods/transformation_masks/mm_player_form.cpp').read_text()
trade = (ROOT / 'soh/mods/items/logic/trade_items.c').read_text()
equipment = (ROOT / 'soh/mods/extended_equipment.h').read_text()
# Migration now clears consumed trade copies before normalizing the mask cell.
# Use the actual dependency closure against this fixture's existing inventory
# and all three equipment banks, without introducing another trade model.
blocks = [next(line for line in equipment.splitlines() if line.startswith('#define ITEM_EXT_BOOTS_2 '))]
blocks += [re.search(r'^#define ' + name + r'\s+.*$', trade, re.M).group()
           for name in ('TRADE_ADULT_COUNT', 'TRADE_ADULT_PENDANT')]
start = trade.index('static const u8 sTradeAdultItems[')
blocks.append(trade[start:trade.index('\n};', start) + len('\n};')])
blocks += [function(trade, name) for name in
           ('TradeAdult_IndexOfItem', 'TradeAdult_IsConsumedIndex', 'TradeAdult_ReconcileConsumed')]
blocks += [function(controller, 'MmForm_OnRitoMaskRemoved')]
blocks += [function(forms, n) for n in (
    'RitoMask_IsOwned', 'RitoItem_NoteCellItem', 'OotMaskWheel_OwnsItem', 'OotMaskWheel_Neighbor',
    'RitoMask_MigrateLegacy', 'RitoMask_Grant', 'RitoMask_Remove', 'RitoItem_SyncCell',
    'RitoItem_FormEnabled', 'GateOn')]
blocks += [function(controller, 'MmForm_DiscardUnownedRitoReload')]
blocks += [function(inv, 'ExtInv_GetItemSlot')]
editor = (ROOT / 'soh/soh/Enhancements/debugger/NeiEditorItems.h').read_text()
blocks += [function(editor, 'SetRitoMaskOwned')]
# Exact icon routing must precede all generic fallback and array accesses.
icon = inv[inv.index('void* ExtInv_GetItemIcon('):]
assert re.search(r'^void\* ExtInv_GetItemIcon\(uint16_t itemId\)\s*\{\s*(?://[^\n]*\n\s*)*if \(itemId == ITEM_RITO_MASK\)\s*\{\s*return \(void\*\)"__OTR__textures/icon_item_custom/gItemIconRitoMaskTex";', icon)
# The actual pause handler and renderer use the same existing corner slot, with no Farore cycler.
pause = (ROOT / 'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c').read_text()
handle = function(pause, 'KaleidoScope_HandleItemCycles')
draw = function(pause, 'KaleidoScope_DrawItemCycles')
for source in (handle, draw):
    assert 'SLOT_TRADE_CHILD' in source and 'OotMaskWheel_Neighbor(-1, legacyMasks)' in source
    assert 'RitoItem_OtherItem' not in source and 'SLOT_FARORES_WIND' not in source
assert handle.index('TradeAdult_FoldCurrent(ExtInv_GetSlotItem(SLOT_TRADE_CHILD))') < handle.index('OotMaskWheel_Neighbor')
load = (ROOT / 'soh/mods/nei_save.cpp').read_text()
assert 'RitoMask_MigrateLegacy();' in function(load, 'NeiSave_Load')
assert 'RitoMask_MigrateLegacy();' not in function(load, 'NeiSave_Init')
with tempfile.TemporaryDirectory(prefix='rito-oot-') as td:
    t = Path(td)
    (t / 'production.inc').write_text('\n'.join(blocks))
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-variable',
             '-I'+str(ROOT), '-I'+str(ROOT/'soh'), '-I'+str(t)]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
    subprocess.run(['c++', *flags, str(ROOT/'tests/rito_mask_oot/test.cpp'), '-o', str(t/'test')], check=True)
    subprocess.run([str(t/'test')], check=True)
print('PASS exact Rito icon resolver, corner input/draw routing and load-only migration')
