#!/usr/bin/env python3
"""Execute explicit native Cucco traversal/check lambda expressions with native IDs."""
from pathlib import Path
import json,re,subprocess,tempfile,os
ROOT=Path(__file__).resolve().parents[3]
from song_fixture import oot_support
rows=json.loads((Path(__file__).with_name('physical_dependencies.json')).read_text())
def body(src,sig):
 a=src.index(sig);b=src.index('{',a);d=1;e=b+1
 while d:d+=(src[e]=='{')-(src[e]=='}');e+=1
 return src[a:e]
def expression(row):
 s=(ROOT/row['file']).read_text();a=s.index('areaTable['+row['source']+']');b=s.find('areaTable[',a+1);s=s[a:b if b!=-1 else None]
 m=re.search(r'\b'+row['kind']+r'\('+row['target']+r',',s);assert m,row
 e=m.end();d=1
 while d:d+=(s[e]=='(')-(s[e]==')');e+=1
 return s[m.end():e-1].strip()
exprs=[expression(r) for r in rows]
logic=(ROOT/'soh/soh/Enhancements/randomizer/logic.cpp').read_text()
helper=body(logic,'bool Logic::CanUseCucco(') if 'bool Logic::CanUseCucco(' in logic else ''
methods=set(re.findall(r'logic->(\w+)\(', '\n'.join(exprs)))-{'HasItem','CanUse','CanUseCucco','CanPlaySongEvent'}
source='''#include "soh/soh/Enhancements/randomizer/randomizerEnums.h"
#include "combo/rando/SongEvents.h"
#include "tests/test_require.h"
#include <set>
#include <functional>
#include <string>
struct Logic {
 bool IsAdult=false,IsChild=true,AtDay=true,enabled=true,soul=false;
 std::set<int> items;std::set<std::string> abilities;
 bool HasItem(RandomizerGet i){return (i==RG_SOUL_ANIMAL_CUCCO||i==RG_SOUL_ANIMAL_COW||i==RG_SOUL_ENEMY_BABY_DODONGO)?(!enabled||soul):items.count(i);}
 bool CanUse(RandomizerGet i){return items.count(i);}
 bool CanUseCucco();
 bool CanPlaySongEvent(unsigned);
'''
for method in sorted(methods):source+='template<class...T> bool '+method+'(T...){return '+('items.count(RG_KOKIRI_SWORD)' if method=='CanAttack' else 'abilities.count("'+method+'")')+';}\n'
source+='''} state; auto*logic=&state;
struct Option {bool Is(int){return false;}};
struct Ctx {std::set<int> tricks;Option GetOption(int){return {};};bool GetTrickOption(int t){return tricks.count(t);}} context;auto*ctx=&context;
template<class F> bool AnyAgeTime(F f){return f();}
'''+oot_support()+helper+'\n'
source+='std::function<bool()> predicates[]={\n'+',\n'.join('[]{return '+e+';}' for e in exprs)+'};\nint main(){\n'
for i,row in enumerate(rows):
 tool='RG_KOKIRI_SWORD' if row['soul']=='RG_SOUL_ENEMY_BABY_DODONGO' else ('RG_FAIRY_SLINGSHOT' if row['soul']=='RG_SOUL_ANIMAL_COW' else 'RG_POWER_BRACELET')
 source+=f'state=Logic{{}};context=Ctx{{}};state.items={{{tool}}};\n'
 if row['target']=='RR_GV_FORTRESS_SIDE':source+='context.tricks.insert(RT_GV_CHILD_CUCCO_JUMP);state.abilities.insert("CanJumpslash");\n'
 if row['target']=='RR_ZR_BEHIND_WATERFALL':source+='context.tricks.insert(RT_ZR_CUCCO);\n'
 if row['target']=='RR_ZR_ATOP_LADDER':source+='state.abilities.insert("CanClimbLadder");\n'
 if row['target']=='RC_KAK_SILVER_BOULDER':source+='state.IsAdult=true;state.IsChild=false;state.items.insert(RG_LONGSHOT);state.abilities.insert("CanBreakSilverBoulder");\n'
 source+=f'REQUIRE(!predicates[{i}]());state.soul=true;REQUIRE(predicates[{i}]());state.items.erase({tool});REQUIRE(!predicates[{i}]());state.items.insert({tool});state.soul=false;state.enabled=false;REQUIRE(predicates[{i}]());\n'
 # Real alternate native routes must continue with no soul and no carrying ability.
 alternatives={'RR_GV_UPPER_STREAM':'state.abilities.insert("TakeDamage");','RR_GV_CRATE_LEDGE':'state.items.insert(RG_LONGSHOT);','RR_KAK_BACKYARD':'state.IsAdult=true;state.IsChild=false;','RR_ZR_PILLAR':'state.items.insert(RG_HOVER_BOOTS);','RC_ZR_NEAR_DOMAIN_FREESTANDING_POH':'state.items.insert(RG_BOOMERANG);','RR_ZR_FRONT':'state.items.insert(RG_BRONZE_SCALE);','RR_ZR_BEHIND_WATERFALL':'state.items.insert(RG_ZELDAS_LULLABY);','RR_DODONGOS_CAVERN_SE_ROOM':'state.abilities.insert("CanBreakMudWalls");'}
 if row['target'] in alternatives:
  source+='state.enabled=true;state.items.erase(RG_POWER_BRACELET);'+alternatives[row['target']]+f'REQUIRE(predicates[{i}]());\n'
source+='puts("PASS native physical/check predicates: missing/owned/off, strength and alternate-route controls");}\n'
with tempfile.TemporaryDirectory(prefix='soul-physical-') as t:
 t=Path(t);cpp=t/'native.cpp';cpp.write_text(source)
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'soh'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
