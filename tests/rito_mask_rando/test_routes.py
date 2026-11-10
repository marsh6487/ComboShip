from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
for p,identity in [('soh/soh/Enhancements/randomizer/3drando/item_pool.cpp','RG_RITO_MASK'),('mm/2s2h/Rando/Logic/GeneratePools.cpp','RI_OOT_NEI_RITO_MASK')]:
 text=(ROOT/p).read_text()
 assert identity in text, f'{p} has no enabled Rito pool'
for p in ['soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp','mm/2s2h/Rando/NeiGiPresentation.cpp']:
 assert 'rito_mask' in (ROOT/p).read_text(), f'{p} has no authored Rito GI'
for p,identity in [('soh/soh/Enhancements/randomizer/randomizer.cpp','RG_RITO_MASK'),('mm/2s2h/Rando/GiveItem.cpp','RI_OOT_NEI_RITO_MASK')]:
 text=(ROOT/p).read_text()
 assert re.search(r'case '+identity+r':\s*RitoMask_Grant\(\);',text), 'native grant missing'
print('PASS native pool/grant/authored-model routes')

assert 'RG_RITO_MASK, false, LOGIC_NONE' in (ROOT/'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
assert 'it.randoItemId != RI_OOT_NEI_RITO_MASK' in (ROOT/'mm/2s2h/BenPort.cpp').read_text()
print('PASS Rito has no solver advancement in either native dump')

assert 'case RI_OOT_NEI_RITO_MASK:' in (ROOT/'mm/2s2h/Rando/RemoveItem.cpp').read_text(), 'MM native removal route missing'
