#!/usr/bin/env python3
"""Exercise production OoT fixed reward grants against actual native save layouts."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--sanitize', action='store_true')
args = p.parse_args()
source = (ROOT / 'soh/soh/Enhancements/Gameplay/ComboVanillaRewards.cpp').read_text()
# The static-story dispatcher also owns OnOpenText: its IDs cannot be reused.
message_ids = [int(value,16) for value in re.search(r'constexpr u16 kOfferText = (0x[0-9A-Fa-f]+), kReceiptText = (0x[0-9A-Fa-f]+), kResultText = (0x[0-9A-Fa-f]+);', source).groups()]
assert len(set(message_ids)) == 3
adapter = ROOT / 'soh/soh/Enhancements/Gameplay/ComboVanillaRewards.cpp'
for path in (ROOT / 'soh').rglob('*'):
    if path == adapter or path.suffix not in {'.h','.c','.cpp','.inc'} or not path.is_file():
        continue
    text = path.read_text(errors='ignore')
    assert not set(message_ids).intersection(int(value,16) for value in re.findall(r'\b0x[0-9A-Fa-f]+\b', text)), f'Native message ID collision in {path}'
def function(signature):
    search=0
    while True:
        start=source.index(signature,search)
        first=source.index('{',start)
        declaration=source.find(';',start,first)
        if declaration<0:
            break
        search=declaration+1
    depth,end=1,first+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}')
        end+=1
    return source[start:end]+'\n'
begin = source.index('// BEGIN TESTED NATIVE REWARD HELPERS')
end = source.index('// END TESTED NATIVE REWARD HELPERS')
with tempfile.TemporaryDirectory(prefix='vanilla-oot-rewards-') as directory:
    temp = Path(directory)
    (temp / 'oot_rewards_helpers.inc').write_text(source[begin:end])
    (temp / 'oot_rewards_receipt.inc').write_text('\n'.join(function(signature) for signature in [
        'static int Host(RewardAction action', 'static bool AcknowledgeOwned(const Reward& reward)',
        'static bool BeginReceipt(const Reward& reward)',
        'static bool FixedReceiptGrant(PlayState* play, const GetItemEntry& entry)']))
    (temp / 'oot_rewards_geometry.inc').write_text('\n'.join(function(signature) for signature in [
        'static bool EntranceMatches(int entrance)',
        'static bool Anchor(', 'static bool SafeRewardPosition(']))
    inventory = (ROOT / 'soh/mods/extended_inventory.c').read_text()
    pause = (ROOT / 'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c').read_text()
    adapter_source = source
    source = inventory
    elements = []
    for table in ['static const uint8_t sSw97ElemQuest', 'static const int16_t sSw97ElemVanillaArrow']:
        start=source.index(table)
        elements.append(source[start:source.index('};',start)+2])
    elements += [function(signature) for signature in [
        'uint8_t BombArrows_RandoMode(void)', 'uint8_t Sw97_BombArrowsOwned(void)',
        'uint8_t Sw97_ElementWheelEnabled(void)', 'uint8_t Sw97_ElementOwned(uint8_t elem)',
        'static uint8_t Sw97_ElementAllowed(uint8_t isSling, uint8_t elem)',
        'uint8_t Sw97_ElementCount(uint8_t isSling)', 'uint8_t Sw97_ElementAt(uint8_t isSling, uint8_t index)',
        'uint8_t Sw97_GetElement(uint8_t isSling)', 'void Sw97_SetElement(uint8_t isSling, uint8_t elem)',
        'uint8_t Sw97_EffectiveElement(uint8_t isSling)',
        'uint8_t Sw97_IsBowItem(uint16_t item)', 'uint8_t Sw97_IsSlingItem(uint16_t item)']]
    source=pause
    elements += [function(signature) for signature in [
        'static s8 ArrowWheel_GetCurrentEntry(s32* outCBtn)',
        'static void ArrowWheel_ApplyEntry(PlayState* play, u8 entryIdx, s32 cBtn)',
        'static void ArrowWheel_Cycle(PlayState* play, s32 dir)',
        'static void ArrowWheel_Build(void)', 'static void ArrowWheel_HandlePressA(PlayState* play)']]
    source=adapter_source
    (temp/'oot_rewards_elements.inc').write_text('\n'.join(elements))
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-DCOMBO_BUILD', '-DF3DEX_GBI_2']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
    includes = [flag for path in (ROOT, ROOT/'combo', ROOT/'soh', ROOT/'soh/include',
                 ROOT/'soh/assets', ROOT/'libultraship/include', temp) for flag in ['-I',str(path)]]
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')), *flags, *includes,
                    str(ROOT/'tests/combo_vanilla/oot_rewards_test.cpp'), '-o', str(temp/'test')],check=True)
    subprocess.run([str(temp/'test')],check=True)
