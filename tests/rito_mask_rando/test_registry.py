"""Preserve R22 IDs and the cumulative Solo MM then Rito append-only identities."""
from pathlib import Path
import json, re
ROOT = Path(__file__).resolve().parents[2]
# The held R22 commit is not guaranteed to exist in CI or source-only recovery.
# This immutable prefix fixture records its exact source blob provenance.
BASELINE = json.loads((Path(__file__).with_name('baseline_ids_r22.json')).read_text())
assert BASELINE['baseline_commit'] == '191794e8c7534c30a55ee4ab4554e49356c6ca75'
# The cumulative candidate keeps Solo MM's existing donor ordinals, then Rito.
# These literals are a save/seed compatibility contract, not the current source's order.
MM_OPTION_APPEND_IDS = [
 (100, 'RO_SHUFFLE_DUNGEON_ENTRANCES'),
 (101, 'RO_SHUFFLE_EXPANDED_DUNGEON_ENTRANCES'),
 (102, 'RO_SHUFFLE_BOSS_ENTRANCES'),
 (103, 'RO_SHUFFLE_INTERIOR_ENTRANCES'),
 (104, 'RO_SHUFFLE_SHOP_ENTRANCES'),
 (105, 'RO_SHUFFLE_DOOR_INTERIORS'),
 (106, 'RO_SHUFFLE_GROTTO_ENTRANCES'),
 (107, 'RO_SHUFFLE_OVERWORLD_ENTRANCES'),
 (108, 'RO_DECOUPLED_ENTRANCES'),
 (109, 'RO_MIX_ENTRANCE_POOLS'),
 (110, 'RO_MIX_DUNGEON_ENTRANCES'),
 (111, 'RO_MIX_BOSS_ENTRANCES'),
 (112, 'RO_MIX_INTERIOR_ENTRANCES'),
 (113, 'RO_MIX_SHOP_ENTRANCES'),
 (114, 'RO_MIX_GROTTO_ENTRANCES'),
 (115, 'RO_MIX_OVERWORLD_ENTRANCES'),
 (116, 'RO_SHUFFLE_RITO_MASK'),
]
def read(p): return (ROOT/p).read_text()
for p, token, end in [
 ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h','RG_RITO_MASK','RG_MAX'),
 ('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h','RSK_SHUFFLE_RITO_MASK','RSK_MAX'),
 ('mm/2s2h/Rando/Types.h','RI_OOT_NEI_RITO_MASK','RI_MAX'),
 ('mm/2s2h/Rando/Types.h','RO_SHUFFLE_RITO_MASK','RO_MAX')]:
 current=read(p)
 assert token in current, f'missing appended {token}'
 prefix=end.split('_')[0]
 regex = r'RANDO_ENUM_ITEM\(('+prefix+r'_[A-Z0-9_]+)' if p.endswith('Get.h') or p.endswith('Key.h') else r'^\s*('+prefix+r'_[A-Z0-9_]+)\s*(?:,|=)'
 ids=lambda s:[x for x in re.findall(regex,s[:s.rfind("RANDO_ENUM_ITEM("+end+")") if "RANDO_ENUM_ITEM" in regex else s.index("    "+end+",")],re.M) if x!=end]
 current_ids, previous_ids = ids(current), BASELINE['identity_prefixes'][prefix]['names']
 appended = [token]
 if token == 'RO_SHUFFLE_RITO_MASK':
  assert len(previous_ids) == 100, 'unexpected R22 MM option boundary'
  appended = [name for _, name in MM_OPTION_APPEND_IDS]
  # Every entry in this enum is implicit. Reject an explicit override rather
  # than treating its source position as its serialized numeric identity.
  option_body = current[:current.index('    RO_MAX,')].rsplit('typedef enum {', 1)[1]
  assert not re.search(r'\bRO_\w+\s*=', option_body), 'MM option numeric override needs compatibility review'
  for value, name in MM_OPTION_APPEND_IDS:
   assert current_ids.index(name) == value, f'{name} must retain ID {value}'
 assert current_ids == previous_ids + appended, f'{prefix} old IDs or cumulative appended order moved'
a=read('soh/soh/FleetShipCombo/FleetComboItems.h'); b=read('mm/2s2h/FleetShipCombo/FleetComboItems.h')
assert a==b
ids=lambda s:re.findall(r'X\((FCI_\w+),',s)
assert ids(a)==BASELINE['identity_prefixes']['FCI']['names']+['FCI_RITO_MASK']
assert 'X(FCI_RITO_MASK, 1, FCI_F_NONE, RG_RITO_MASK, RI_OOT_NEI_RITO_MASK' in a
for p,tok in [('soh/soh/Enhancements/randomizer/randomizer.cpp','RG_RITO_MASK'),('mm/2s2h/Rando/GiveItem.cpp','RI_OOT_NEI_RITO_MASK')]:
 assert re.search(r'case '+tok+r':\s*RitoMask_Grant\(\);\s*break;',read(p)),p
print('PASS stable identities and native grant routing')
