"""Independent expected availability from native region scene/layout ownership."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[3]

def expected_layout_masks():
 r=ROOT/'soh/soh/Enhancements/randomizer'
 regions={};scene_set=set()
 for path in (r/'location_access').rglob('*.cpp'):
  quest='RCQUEST_BOTH'
  for line in path.read_text().splitlines():
   if '#pragma region Vanilla' in line:quest='RCQUEST_VANILLA'
   elif '#pragma region MQ' in line:quest='RCQUEST_MQ'
   elif '#pragma endregion' in line:quest='RCQUEST_BOTH'
   match=re.search(r'areaTable\[(RR_\w+)\] = Region\("[^\"]*", (SCENE_\w+)',line)
   if match:
    regions[match[1]]=(match[2],quest)
    if quest!='RCQUEST_BOTH':scene_set.add(match[2])
 scenes=sorted(scene_set);assert len(scenes)==12
 keys=re.findall(r'^ENEMY_FIRST_DROP\((\w+),',(r/'EnemyFirstDropsCatalog.inc').read_text(),re.M)
 profiles=[]
 for file in ('EnemyFirstDropsWitnesses.inc','EnemyFirstDropsAdditionalWitnesses.inc'):
  for key,region,scene,quest in re.findall(r'^ENEMY_DROP_WITNESS\((\w+),\s*(RR_\w+),\s*(SCENE_\w+),\s*(RCQUEST_\w+),',(r/file).read_text(),re.M):
   assert regions[region]==(scene,quest),(key,region,scene,quest,regions[region])
   profiles.append((key,*regions[region]))
 expected=[]
 for mask in range(4096):
  active={key for key,scene,quest in profiles if quest=='RCQUEST_BOTH' or bool(mask&(1<<scenes.index(scene)))==(quest=='RCQUEST_MQ')}
  expected.append(sum(1<<keys.index(key) for key in active))
 assert expected[0]==(1<<43)-1,'the all-Vanilla game must cover every native enemy type'
 return scenes,expected
