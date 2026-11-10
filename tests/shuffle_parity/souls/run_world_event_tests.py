#!/usr/bin/env python3
"""Run original source-qualified OoT quest predicates and MM saved-event soul adapter."""
from pathlib import Path
import json,os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
def body(s,signature):
 a=s.index(signature);e=s.index('{',a)+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
p=Path(__file__).parent
for game in ('oot','mm'):
 rows=json.loads((p/f'{game}_world_events.json').read_text())
 if game=='oot':
  expressions=[]
  for f in (ROOT/'soh/soh/Enhancements/randomizer/location_access').rglob('*.cpp'):
   if f.name=='root.cpp':continue
   s=f.read_text()
   for event,souls in rows.items():
    for m in re.finditer(r'EVENT_ACCESS\('+event+r'\s*,',s):
     a=m.end();e=a;d=1
     while d:d+=(s[e]=='(')-(s[e]==')');e+=1
     expressions.append((event,souls,s[a:e-1]))
  assert set(e for e,_,_ in expressions)==set(rows)
  methods=set(re.findall(r'logic->(\w+)\(','\n'.join(e for _,_,e in expressions)))-{'HasItem'}
  cpp='''#include "soh/soh/Enhancements/randomizer/SoulShuffle.h"
#include "soh/tests/test_require.h"
#include <set>
#include <functional>
struct Logic {bool IsAdult=true,IsChild=true,AtDay=true,enabled=true,capability=true;std::set<RandomizerGet> owned;
 bool HasItem(RandomizerGet i){return Rando::SoulShuffle::Find(i)?(!enabled||owned.count(i)):capability;}
'''+''.join(f'template<class...T>bool {m}(T...){{return capability;}}\n' for m in sorted(methods))+'''} state;auto*logic=&state;
struct Option{bool IsNot(int){return true;}};struct Ctx{Option GetOption(int){return {};}bool GetTrickOption(int){return true;}} context;auto*ctx=&context;
int GetCheckPrice(RandomizerCheck){return 0;}int GetWalletCapacity(){return 999;}
std::function<bool()> predicates[]={
'''+',\n'.join('[]{return '+e+';}' for _,_,e in expressions)+'};\nint main(){\n'
  for i,(_,souls,e) in enumerate(expressions):
   cpp+='state=Logic{};state.owned={'+','.join(souls)+f'}};REQUIRE(predicates[{i}]());\n'
   for soul in souls:cpp+=f'state.owned.erase({soul});REQUIRE(!predicates[{i}]());state.owned.insert({soul});\n'
   cpp+=f'state.enabled=false;state.owned.clear();REQUIRE(predicates[{i}]());\n'
   if e.strip().endswith('&& true') is False:cpp+=f'state.capability=false;REQUIRE(!predicates[{i}]());\n'
  # The native starting-setting shortcuts stay independent of encounter sources.
  root=(ROOT/'soh/soh/Enhancements/randomizer/location_access/overworld/root.cpp')
  if not root.exists():root=next((ROOT/'soh/soh/Enhancements/randomizer/location_access').rglob('root.cpp'))
  for line in root.read_text().splitlines():
   if any('EVENT_ACCESS('+event in line for event in rows):assert 'RG_SOUL_' not in line
 else:
  s=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
  cpp='''#include "mm/2s2h/Rando/SoulShuffle.h"
#include "soh/tests/test_require.h"
#include <set>
std::set<int> owned;unsigned options[RO_MAX]{};
#define RANDO_SAVE_OPTIONS options
bool Flags_GetRandoInf(RandoInf f){return owned.count(f);}
void Own(RandoItemId i,bool on=true){auto*s=Rando::SoulShuffle::Find(i);REQUIRE(s);if(on)owned.insert(s->flag);else owned.erase(s->flag);}
'''+body(s,'inline bool SoulEventRequirement(')+'\nint main(){\n'
  for event,souls in rows.items():
   cpp+='owned.clear();for(auto&o:options)o=1;'+''.join(f'Own({x});' for x in souls)+f'REQUIRE(SoulEventRequirement({event}));\n'
   for soul in souls:cpp+=f'Own({soul},false);REQUIRE(!SoulEventRequirement({event}));Own({soul});\n'
   cpp+=f'owned.clear();for(auto&o:options)o=0;REQUIRE(SoulEventRequirement({event}));\n'
 cpp+=f'puts("PASS {game} source/event requirements: {len(rows)} independent quest events, missing/owned/off and source shortcuts");}}\n'
 with tempfile.TemporaryDirectory(prefix='souls-world-events-') as t:
  t=Path(t);src=t/'events.cpp';src.write_text(cpp)
  for mode in ('original','combo'):
   exe=t/mode;subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),'-I'+str(ROOT/'soh/soh/Enhancements/randomizer'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
