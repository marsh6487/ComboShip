#!/usr/bin/env python3
"""Execute the real successful MM dump serialization with native saved options."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
s=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
a=s.index('extern "C" COMBO_EXPORT const char* MM_DumpRandoStaticData(void)')
b=s.index('// ComboShip: MM reachability oracle',a)
function=s[a:b]
tail=function[function.rindex('    cached = nlohmann::json{'):]
# Execute the actual native option expression, not a hand-written policy copy.
metadata=function[function.index('    nlohmann::json checkMetadata ='):function.index('    // ComboShip: the two hint options')]
options=function[function.rindex('    nlohmann::json startingSouls ='):function.rindex('    cached = nlohmann::json{')]
def body(source, signature):
 a=source.index(signature);b=source.index('{',a);depth=1;e=b+1
 while depth:
  depth+=(source[e]=='{')-(source[e]=='}');e+=1
 return source[a:e]
starts=(ROOT/'mm/2s2h/Rando/StartingItems.cpp').read_text()
getter=body(starts,'std::vector<RandoItemId> GetStartingItemsFromSave(')+'\n'+body(starts,'void SetStartingItemsInSave(')
oracle=s[s.index('extern "C" COMBO_EXPORT void Combo_MM_Rando_Reset(void)'):]
oracle=body(oracle,'    {\n        auto startingItems = Rando::GetStartingItemsFromConfig();')
checks_source=(ROOT/'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
a=checks_source.index('#define RC(');b=checks_source.index('// clang-format off',a)
check_ids=('RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART','RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2')
check_rows='\n'.join(next(line for line in checks_source.splitlines() if re.match(r'\s*RC\('+key+r'\s*,',line)) for key in check_ids)
native_checks=checks_source[a:b]+'\nstd::map<RandoCheckId,RandoStaticCheck> Checks = {\n'+check_rows+'\n};\n#undef RC\n'
cpp='''#include <nlohmann/json.hpp>
#include "Rando/Rando.h"
#include "Rando/SoulShuffle.h"
#include "rando/SharedSouls.h"
#include <cassert>
extern "C" { SaveContext gSaveContext{}; }
std::vector<RandoItemId> configured,granted;
static void GiveItemForOracle(RandoItemId item){granted.push_back(item);}
namespace Rando::StaticData {
'''+native_checks+'''
// Display/hint naming services are explicit data-context boundaries; metadata
// fields and identity rows above come directly from the native source.
const std::string& GetCheckDisplayName(RandoCheckId id){static std::map<RandoCheckId,std::string> names;return names.try_emplace(id,Checks.at(id).name).first->second;}
std::string GetLocationNameForHint(RandoCheckId id,bool){return std::string("Fixture hint for ")+Checks.at(id).name;}

}
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromConfig(){return configured;}
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo&){return {};}
RandoItemId ConvertItem(RandoItemId item,RandoCheckId){return item;}
'''+getter+'''
}
static void OracleStarts(){
'''+oracle+'''
}
static const char* Success(){
static std::string cached;
auto& saveInfo=gSaveContext.save.shipSaveInfo.rando;
nlohmann::json checks,pool,fixed,items,prices,locationHints,options,junkPool;
'''+metadata+options+'\n'+tail+'''
int main(){
 const RandoOptionId ids[]={RO_SHUFFLE_ENEMY_SOULS,RO_SHUFFLE_NPC_SOULS,RO_SHUFFLE_ANIMAL_SOULS,RO_SHUFFLE_MISC_SOULS};
 for(unsigned enabled=0;enabled<16;++enabled){
  gSaveContext={};
  uint32_t expected=0;
  for(unsigned i=0;i<4;++i){
   gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[ids[i]]=(enabled>>i)&1;
   if((enabled>>i)&1)expected|=ComboRando::SharedSoulCategories[i].bit;
  }
  auto& nativeStarts=gSaveContext.save.shipSaveInfo.rando.randoStartingItems;
  nativeStarts[0]=RI_SOUL_NPC_BANKER;nativeStarts[1]=RI_SOUL_ENEMY_WALLMASTER;
  nativeStarts[2]=RI_SOUL_ANIMAL_CUCCO;nativeStarts[3]=RI_SOUL_MISC_GS;
  nativeStarts[4]=RI_SOUL_ENEMY_ALIEN; // no native OoT counterpart
  auto result=nlohmann::json::parse(Success());
  assert(result["startingSouls"].size()==4);
  assert(result.at("checkMetadata").size()==2);
  for(auto id:{RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2}){
   const auto& native=Rando::StaticData::Checks.at(id);const auto& name=Rando::StaticData::GetCheckDisplayName(id);
   const auto& metadata=result.at("checkMetadata").at(name);
   assert(metadata.is_object()&&metadata.at("scene").is_number_integer()&&metadata.at("id").is_number_integer());
   assert(metadata.at("sourceName").is_string());
   assert(metadata.at("scene")==native.sceneId&&metadata.at("id")==static_cast<int>(id));
   assert(metadata.at("sourceName")==native.name);
   assert(result.at("locationHints").at(name)==Rando::StaticData::GetLocationNameForHint(id,false));
  }
  for(int i=0;i<4;++i){
   const char* name=Rando::SoulShuffle::SharedName(static_cast<RandoItemId>(nativeStarts[i]));
   assert(std::find(result["startingSouls"].begin(),result["startingSouls"].end(),name)!=result["startingSouls"].end());
  }
  assert(result.contains("soulOptions"));
  assert(!result.contains("error"));
  assert(ComboRando::EffectiveSharedSoulMask(ComboRando::SS_ALL,result,result)==expected);
 }
 gSaveContext={};assert(nlohmann::json::parse(Success())["startingSouls"].empty());
 const size_t capacity=ARRAY_COUNT(gSaveContext.save.shipSaveInfo.rando.randoStartingItems);
 configured.assign(capacity,RI_RUPEE_GREEN);configured.back()=RI_SOUL_ANIMAL_CUCCO;
 configured.push_back(RI_SOUL_NPC_BANKER);granted.clear();OracleStarts();
 assert(granted.size()==capacity);
 assert(std::find(granted.begin(),granted.end(),RI_SOUL_NPC_BANKER)==granted.end());
 assert(std::find(granted.begin(),granted.end(),RI_SOUL_ANIMAL_CUCCO)!=granted.end());
 configured={RI_SOUL_NPC_BANKER};granted.clear();OracleStarts();assert(granted==configured);
 assert(nlohmann::json::parse(Success())["startingSouls"]==nlohmann::json::array({"Banker NPC Soul"}));
 puts("PASS actual normal-success MM dump schema and native saved soul options in all16 combinations");
}
'''
with tempfile.TemporaryDirectory(prefix='soul-dump-') as tmp:
 p=Path(tmp)/'dump.cpp';p.write_text(cpp)
 for mode in ('original','combo'):
  exe=Path(tmp)/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-fmax-errors=5','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',*(['-DCOMBO_BUILD'] if mode=='combo' else []),*['-I'+str(ROOT/f) for f in ('mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo')],str(p),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
# The OoT exporter must refresh native starting inventory before serializing it.
oot=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
a=oot.index('extern "C" COMBO_EXPORT const char* SOH_DumpRandoStaticData(void)')
a=oot.index('        GenerateStartingInventory();',a)
b=oot.index('        soulOptions =',a)
cpp='''#include <nlohmann/json.hpp>
#include "soh/Enhancements/randomizer/SoulShuffle.h"
#include <cassert>
#include <vector>
std::vector<RandomizerGet> configured,StartingInventory;
void GenerateStartingInventory(){StartingInventory=configured;}
static nlohmann::json Export(){
 nlohmann::json startingSouls=nlohmann::json::array();
'''+oot[a:b]+'''
 return startingSouls;
}
int main(){
 configured={RG_SOUL_ENEMY_WALLMASTER,RG_SOUL_NPC_BANKER,RG_SOUL_ANIMAL_CUCCO,RG_SOUL_MISC_GS,RG_MM_SOUL_WALLMASTER,RG_NONE};
 auto names=Export();assert(names.size()==5);
 for(size_t i=0;i<5;++i)assert(names[i]==Rando::SoulShuffle::SharedName(configured[i]));
 configured.clear();assert(Export().empty());
 puts("PASS actual OoT starting-inventory export block and native/legacy identities");
}
'''
with tempfile.TemporaryDirectory(prefix='oot-soul-start-dump-') as tmp:
 p=Path(tmp)/'dump.cpp';p.write_text(cpp)
 for mode in ('original','combo'):
  exe=Path(tmp)/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*(['-DCOMBO_BUILD'] if mode=='combo' else []),*['-I'+str(ROOT/f) for f in ('soh','soh/include','soh/src','soh/assets','libultraship/include','combo')],str(p),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
