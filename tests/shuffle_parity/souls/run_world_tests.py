#!/usr/bin/env python3
"""Execute production world-check adapters against independently recorded dependencies."""
from pathlib import Path
import json, os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
def body(s,signature):
 a=s.index(signature);b=s.index('{',a)+1;e=b;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
for game in ('oot','mm'):
 rows=json.loads((Path(__file__).parent/f'{game}_world_checks.json').read_text())
 if game=='oot':
  folder='soh/soh/Enhancements/randomizer';s=(ROOT/folder/'logic.cpp').read_text()
  cpp='#include "'+folder+'/SoulShuffle.h"\n#include "soh/tests/test_require.h"\n#include <set>\n'
  cpp+='struct Logic{std::set<RandomizerGet> owned;bool enabled=true;bool HasItem(RandomizerGet i){return !enabled||owned.count(i);}bool CanAccessSoulCheck(RandomizerCheck); } state;\n'
  cpp+=body(s,'bool Logic::CanAccessSoulCheck(')+'\nint main(){\n'
  for check,souls in rows.items():
   cpp+='state.owned={'+','.join(souls)+'};state.enabled=true;REQUIRE(state.CanAccessSoulCheck('+check+'));\n'
   for soul in souls:cpp+=f'state.owned.erase({soul});REQUIRE(!state.CanAccessSoulCheck({check}));state.owned.insert({soul});\n'
   cpp+=f'state.owned.clear();state.enabled=false;REQUIRE(state.CanAccessSoulCheck({check}));\n'
 else:
  s=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
  cpp='''#include "mm/2s2h/Rando/SoulShuffle.h"
#include "soh/tests/test_require.h"
#include <set>
std::set<int> owned;unsigned options[RO_MAX]{};
#define RANDO_SAVE_OPTIONS options
bool Flags_GetRandoInf(RandoInf f){return owned.count(f);}
void Own(RandoItemId i,bool on=true){auto*s=Rando::SoulShuffle::Find(i);REQUIRE(s);if(on)owned.insert(s->flag);else owned.erase(s->flag);}
'''+body(s,'inline bool SoulCheckRequirement(')+'\nint main(){\n'
  for check,souls in rows.items():
   cpp+='owned.clear();for(auto&o:options)o=1;'+''.join(f'Own({x});' for x in souls)+f'REQUIRE(SoulCheckRequirement({check}));\n'
   for soul in souls:cpp+=f'Own({soul},false);REQUIRE(!SoulCheckRequirement({check}));Own({soul});\n'
   cpp+=f'owned.clear();for(auto&o:options)o=0;REQUIRE(SoulCheckRequirement({check}));\n'
 cpp+=f'puts("PASS {game} native world-check requirements: {len(rows)} independent cases with each missing/owned/off control");}}\n'
 with tempfile.TemporaryDirectory(prefix='souls-world-') as t:
  t=Path(t);src=t/'world.cpp';src.write_text(cpp)
  for mode in ('original','combo'):
   exe=t/mode;subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),'-I'+str(ROOT/'soh/soh/Enhancements/randomizer'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
