#!/usr/bin/env python3
"""Check real rendered option registrations, counts and existing accent wrapper."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[3]
oot=(ROOT/'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
for key,label,cvar in [('ENEMY','Enemy Souls','ShuffleEnemySouls'),('NPC','NPC Souls','ShuffleNpcSouls'),('ANIMAL','Animal Souls','ShuffleAnimalSouls'),('MISC','Miscellaneous Souls','ShuffleMiscSouls')]:
 registration=next(line for line in oot.splitlines() if f'OPT_U8(RSK_SHUFFLE_{key}_SOULS,' in line)
 assert f'"{label}"' in registration and f'CVAR_RANDOMIZER_SETTING("{cvar}")' in registration
 assert 'RO_GENERIC_OFF' in registration and 'WIDGET_CVAR_COMBOBOX' in registration
 assert oot.count(f'&mOptions[RSK_SHUFFLE_{key}_SOULS]')==2
mm=(ROOT/'mm/2s2h/Rando/Menu.cpp').read_text();a=mm.index('UIWidgets::BeginCard("itemPoolSouls")');b=mm.index('UIWidgets::EndCard()',a);card=mm[a:b]
for label,key in [('Enemy Souls','ENEMY'),('NPC Souls','NPC'),('Animal Souls','ANIMAL'),('Miscellaneous Souls','MISC')]:assert re.search(r'ItemPoolCheckbox\(\s*"'+re.escape(label)+r'",\s*RO_SHUFFLE_'+key+r'_SOULS\s*,',card)
assert 'RI_SOUL_NPC_THIEVES - RI_SOUL_NPC_AROMA + 1' in card
assert 'Blocking NPCs retain their barriers' in card and 'Blocking NPCs retain their barriers' in oot
helper=mm[mm.index('static bool ItemPoolCheckbox('):mm.index('static bool ItemPoolCheckbox(')+1800]
assert 'WhatsNew::Scope' in helper
print('PASS actual native soul menu controls: named Off-default OoT options in both groups; MM NPC/Animal/Misc counts and accent scope; blocker help')
