#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tests/shuffle_parity/boss_keys'))
from run_tests import function
# Execute the actual name-keyed exclusion writer/reader against an in-memory
# Config boundary. The native file adapter and GUI are intentionally outside it.
source=(ROOT/'mm/2s2h/Rando/ExcludedChecks.cpp').read_text()
checks=(ROOT/'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
cpp=r'''
#include "mm/2s2h/Rando/Types.h"

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
struct Check {const char* name; RandoCheckType randoCheckType=RCTYPE_ICICLE;};
std::map<RandoCheckId,Check> Checks={
#define MM_ICICLE(id, scene, room, index, region) {id,{#id}},
#include "mm/2s2h/Rando/MMEnvironmentIcicles.inc"
#undef MM_ICICLE
};
'''+function(checks,'GetCheckIdFromName')+'\n}\n'+function(source,'GetExcludedChecksFromConfig')+'\n'+function(source,'SetExcludedChecksInConfig')+r'''
}
int main(){
 using namespace Rando;
 for(int mask=0;mask<128;++mask){
  std::vector<RandoCheckId> ids;
  const RandoCheckId all[]={
#define MM_ICICLE(id, scene, room, index, region) id,
#include "mm/2s2h/Rando/MMEnvironmentIcicles.inc"
#undef MM_ICICLE
};
  for(int i=0;i<7;++i)if(mask&(1<<i))ids.push_back(all[i]);
  SetExcludedChecksInConfig(ids);REQUIRE(Ship::Context::GetRawInstance()->config.saved);
  auto serialized=Ship::Context::GetRawInstance()->config.value.dump();
  Ship::Context::GetRawInstance()->config.value=json::parse(serialized);
  REQUIRE(GetExcludedChecksFromConfig()==ids);

 }
 puts("PASS actual environmental exclusion names serialize/reload for all 128 subsets");
}
'''
menu=(ROOT/'mm/2s2h/Rando/Menu.cpp').read_text()
cpp=cpp.replace('int main(){', 'int optionOn;\nint CVarGetInteger(const char*,int){return optionOn;}\nnamespace Rando::StaticData {struct Option{const char* cvar;};std::map<RandoOptionId,Option> Options={{RO_SHUFFLE_ICICLE_DROPS,{"test option service"}}};}\n'+function(menu,'EnvironmentCheckEnabled')+'\nint main(){')
cpp=cpp.replace('  REQUIRE(GetExcludedChecksFromConfig()==ids);','  REQUIRE(GetExcludedChecksFromConfig()==ids);\n  for(optionOn=0;optionOn<2;++optionOn)for(auto id:ids)REQUIRE(EnvironmentCheckEnabled(id)==bool(optionOn));')
with tempfile.TemporaryDirectory(prefix='environment-exclusions-') as td:
 td=Path(td);src=td/'test.cpp';src.write_text(cpp)
 for mode in ('original','combo'):
  exe=td/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(src),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)

menu=(ROOT/'mm/2s2h/Rando/Menu.cpp').read_text()
for start,end in [('static bool IsCheckShownAsExcluded','static RandoCheckId DrawCheckList'),('static const std::vector<std::string>& GetCheckFilterGroupLabels','static void DrawCheckGroupFilter')]:
 assert 'EnvironmentCheckEnabled' in menu[menu.index(start):menu.index(end)]
print('PASS disabled environmental exclusion display and metric filtering wired in actual menu')
