"""The Mirror is a standalone, append-only native randomizer identity."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[2]
# Public PR66 has the exact pre-shuffle identities; CI fetches this immutable pin.
BASE = '0cd35e4d25f853dc68995dd88e8b1c177840e301'
# Preserve every previous serialized identity, including RI_MAX_TRAP (a real draw id).
for path, end, added in [
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h', 'RG_MAX', 'RG_MAGIC_MIRROR'),
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerCheck.h', 'RC_MAX', 'RC_LOST_WOODS_MAGIC_MIRROR_STUMP'),
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h', 'RSK_MAX', 'RSK_SHUFFLE_MAGIC_MIRROR'),
    ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerInf.h', 'RAND_INF_MAX', 'RAND_INF_MAGIC_MIRROR_STUMP'),
]:
    current = (ROOT / path).read_text()
    assert f'RANDO_ENUM_ITEM({added})' in current, added
    original = subprocess.check_output(['git', 'show', f'{BASE}:{path}'], cwd=ROOT, text=True)
    before = re.findall(r'RANDO_ENUM_ITEM\((\w+)\)', original)
    after = re.findall(r'RANDO_ENUM_ITEM\((\w+)\)', current)
    assert after[:before.index(end)] == before[:before.index(end)], path
    assert after[before.index(end)] == added, path
mm = (ROOT / 'mm/2s2h/Rando/Types.h').read_text()
old_mm = subprocess.check_output(['git', 'show', BASE + ':mm/2s2h/Rando/Types.h'], cwd=ROOT, text=True)
for prefix, end, addition in [('RC', 'RC_MAX', 'RC_LOST_WOODS_MAGIC_MIRROR_STUMP'),
                              ('RI', 'RI_MAX', 'RI_OOT_NEI_MAGIC_MIRROR'),
                              ('RO', 'RO_MAX', 'RO_SHUFFLE_MAGIC_MIRROR')]:
    before = re.findall(r'^\s*(' + prefix + r'_\w+)\s*,', old_mm, re.M)
    after = re.findall(r'^\s*(' + prefix + r'_\w+)\s*,', mm, re.M)
    idx = before.index(end)
    assert after[:idx] == before[:idx], prefix + ' ordinal changed'
    assert after[idx] == addition, prefix + ' must be appended, including after RI_MAX_TRAP'
old_fc = subprocess.check_output(['git', 'show', BASE + ':soh/soh/FleetShipCombo/FleetComboItems.h'], cwd=ROOT, text=True)
new_fc = (ROOT / 'soh/soh/FleetShipCombo/FleetComboItems.h').read_text()
ids = lambda text: re.findall(r'X\((FCI_\w+),', text)
# Mirror retains its original append ordinal. Later authorized features also
# append identities, so validate their entire tail against independent native
# catalogues rather than requiring the Mirror to remain the last item forever.
rusty = re.findall(r'^RUSTY_KEY\((OOT|MM),\s*(\w+),',
                   (ROOT / 'combo/rando/RustyDoorKeyCatalog.inc').read_text(), re.M)
assert sum(game == 'OOT' for game, _ in rusty) == 27
assert sum(game == 'MM' for game, _ in rusty) == 39
extensions = [f'FCI_MM_KEY_RING_{dungeon}' for dungeon in
              ('WOODFALL', 'SNOWHEAD', 'GREAT_BAY', 'STONE_TOWER')]
extensions += ['FCI_OOT_CLOCK']
extensions += [f'FCI_{game}_RUSTY_KEY_{key}' for game, key in rusty]
extensions += ['FCI_RITO_MASK']

def check_combo_registry(current):
    assert len(current) == len(set(current)), 'duplicate serialized combo identity'
    assert current == old_ids + ['FCI_MAGIC_MIRROR'] + extensions, 'combo ordinal or authorized tail changed'

old_ids = ids(old_fc)
assert len(old_ids) == 393, 'pinned pre-Mirror registry changed'
current_ids = ids(new_fc)
check_combo_registry(current_ids)
# The broader append contract still rejects shifts, removal, excess identities,
# duplicate identities and incorrect game ownership in the appended catalogue.
for mutation in (
    current_ids[1:],
    current_ids[1:2] + current_ids[:1] + current_ids[2:],
    current_ids[:394] + [current_ids[395], current_ids[394]] + current_ids[396:],
    current_ids + ['FCI_UNAUTHORIZED'],
    current_ids + [current_ids[-1]],
    current_ids[:len(old_ids)] + extensions + ['FCI_MAGIC_MIRROR'],
    current_ids[:-1],
    current_ids[:len(old_ids) + 6] + ['FCI_MM_RUSTY_KEY_TREASURE_CHEST_GAME'] +
        current_ids[len(old_ids) + 7:],
):
    try:
        check_combo_registry(mutation)
    except AssertionError:
        pass
    else:
        raise AssertionError('combo registry mutation was accepted')

assert 'RI_OOT_NEI_MAGIC_MIRROR,' in mm
assert 'RO_SHUFFLE_MAGIC_MIRROR,' in mm
assert 'RC_LOST_WOODS_MAGIC_MIRROR_STUMP,' in mm
for host in ['soh/soh', 'mm/2s2h']:
    table = (ROOT / host / 'FleetShipCombo/FleetComboItems.h').read_text()
    assert 'X(FCI_MAGIC_MIRROR, 1, FCI_F_NONE, RG_MAGIC_MIRROR, RI_OOT_NEI_MAGIC_MIRROR,' in table
assert (ROOT / 'soh/soh/FleetShipCombo/FleetComboItems.h').read_bytes() == (ROOT / 'mm/2s2h/FleetShipCombo/FleetComboItems.h').read_bytes()
print('PASS append-only Mirror item/check/option/flag and cross-game identity registry')
