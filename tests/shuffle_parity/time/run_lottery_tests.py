#!/usr/bin/env python3
"""Compile real lottery policy, callbacks and stored solver predicates; no game-runtime claim."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
types=(ROOT/'mm/2s2h/Rando/Types.h').read_text()
assert 'RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2' in types, 'Independent night-two check missing'
assert 'RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3' in types, 'Independent night-three check missing'
print('PASS lottery appended native identities')
import sys,re
sys.path.insert(0,str(ROOT/'tests/shuffle_parity/boss_keys'))
from run_tests import function
with tempfile.TemporaryDirectory(prefix='lottery-fixtures-') as td:
 build=Path(td)
 inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
 (build/'lottery_tables.inc').write_text('extern "C" {\n'+'\n'.join(re.search(r'^(?:u32|u8) '+name+r'\[\] = \{.*?^\};',inventory,re.M|re.S)[0] for name in ('gUpgradeMasks','gUpgradeShifts'))+'\n}')
 actor=(ROOT/'mm/2s2h/Rando/ActorBehavior/EnKujiya.cpp').read_text()
 (build/'lottery_callbacks.inc').write_text(actor[actor.index('static RandoCheckId CurrentLotteryCheck'):])
 json=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text();a=json.index('inline void to_json(json& j, const RandoSaveCheck&');b=json.index('inline void to_json(json& j, const Vec3f&',a)
 (build/'lottery_json.inc').write_text('using json=nlohmann::json;\n'+json[a:b])
 central=(ROOT/'mm/2s2h/Rando/Logic/Regions/Central.cpp').read_text();a=central.index('    Regions[RR_LOTTERY_SHOP]');b=central.index('    Regions[RR_MAYOR_RESIDENCE]',a)
 (build/'lottery_region.inc').write_text(central[a:b])
 # Reuse the production pool runner, adding only generated fixture includes.
 env={**os.environ,'CPATH':str(build)+os.pathsep+os.environ.get('CPATH','')}
 subprocess.run([sys.executable,str(ROOT/'tests/shuffle_parity/key_rings/run_tests.py'),'--fixture',str(Path(__file__).with_name('lottery_native_test.cpp')),*sys.argv[1:]],check=True,env=env)
# Execute the actual name-keyed exclusion writer/reader against an in-memory
# Config boundary. The native file adapter and GUI are intentionally outside it.
source=(ROOT/'mm/2s2h/Rando/ExcludedChecks.cpp').read_text()
checks=(ROOT/'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
cpp=r'''
#include "mm/2s2h/Rando/Types.h"
#include "mm/2s2h/Rando/Lottery.h"
#include "soh/tests/test_require.h"
#include <nlohmann/json.hpp>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
using json=nlohmann::json;
struct Config {json value;bool saved=false;
 json GetNestedJson(){return value;}
 void SetBlock(const char*,json data){value["CVars"]["gRando"]["ExcludedChecks"]=data;}
 void Save(){saved=true;}
};
struct Gui {void SaveConsoleVariablesNextFrame(){}};
struct Window {Gui* GetGui(){return nullptr;}};
namespace Ship {struct Context {
 Config config; static Context* GetRawInstance(){static Context c;return &c;}
 Config* GetConfig(){return &config;} Window* GetWindow(){return nullptr;}
};}
void CVarClear(const char*){}
namespace Rando {namespace StaticData {
struct Check {const char* name;};
std::map<RandoCheckId,Check> Checks={
{RC_CLOCK_TOWN_WEST_LOTTERY,{"RC_CLOCK_TOWN_WEST_LOTTERY"}},
{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,{"RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2"}},
{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3,{"RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3"}}};
'''+function(checks,'GetCheckIdFromName')+'\n}\n'+function(source,'GetExcludedChecksFromConfig')+'\n'+function(source,'SetExcludedChecksInConfig')+r'''
}
int main(){
 using namespace Rando;
 for(int mask=0;mask<8;++mask){
  std::vector<RandoCheckId> ids;
  const RandoCheckId all[]={RC_CLOCK_TOWN_WEST_LOTTERY,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3};
  for(int i=0;i<3;++i)if(mask&(1<<i))ids.push_back(all[i]);
  SetExcludedChecksInConfig(ids);REQUIRE(Ship::Context::GetRawInstance()->config.saved);
  auto serialized=Ship::Context::GetRawInstance()->config.value.dump();
  Ship::Context::GetRawInstance()->config.value=json::parse(serialized);
  REQUIRE(GetExcludedChecksFromConfig()==ids);
  for(bool on:{false,true})for(auto id:ids)REQUIRE(Lottery::IsEnabled(id,on)==(on||id==all[0]));
 }
 puts("PASS actual lottery exclusion names serialize/reload; disabled nights retained without display activation");
}
'''
with tempfile.TemporaryDirectory(prefix='lottery-exclusions-') as td:
 td=Path(td);src=td/'test.cpp';src.write_text(cpp)
 for mode in ('original','combo'):
  exe=td/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(src),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
menu=(ROOT/'mm/2s2h/Rando/Menu.cpp').read_text()
assert 'ItemPoolCheckbox("Separate Lottery Nights", RO_SHUFFLE_LOTTERY_NIGHTS,' in menu
assert '"gRando.Options.RO_SHUFFLE_LOTTERY_NIGHTS"' in menu
for start,end in [('static bool IsCheckShownAsExcluded','static RandoCheckId DrawCheckList'),('static const std::vector<std::string>& GetCheckFilterGroupLabels','static void DrawCheckGroupFilter')]:
 assert 'Lottery::IsEnabled' in menu[menu.index(start):menu.index(end)]
print('PASS lottery real UI control/metrics and disabled exclusion display source integration')
