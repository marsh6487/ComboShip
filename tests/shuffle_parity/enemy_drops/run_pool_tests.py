#!/usr/bin/env python3
"""Run native location catalogue and native pool/exclusion algorithms.

The generator's context/dungeon services and UI-only exclusion-option constructor
are isolated boundaries; Location, ItemLocation, OptionValue and all algorithms
being checked are production code. This is not a full seeded solver fixture.
"""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3];r=ROOT/'soh/soh/Enhancements/randomizer'
def function(source,signature):
 start=source.index(signature);end=source.index('\n}',start)+2
 return source[start:end]
seed=(r/'SeedContext.cpp').read_text();pool=(r/'3drando/item_pool.cpp').read_text();itemloc=(r/'item_location.cpp').read_text();option=(r/'option.cpp').read_text()
source=r'''
#include "soh/Enhancements/randomizer/EnemyFirstDrops.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/fishsanity.h"
#include "soh/Enhancements/randomizer/3drando/pool_functions.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <cassert>
#include <spdlog/spdlog.h>
#include "soh/Enhancements/randomizer/randomizerEnumStrings.h"
#define REQUIRE(expr) do{if(!(expr)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#expr);abort();}}while(0)
bool combined=false;
#ifdef COMBO_BUILD
extern "C" {int gComboGameMode=COMBO_GAME_OOT;}
#endif
namespace Rando {
// No UI is instantiated: the native location still carries the native exclusion identity.
LocationOption::LocationOption(RandomizerCheck check){key=check;}
std::array<Location,RC_MAX> StaticData::locationTable;
std::unordered_map<std::string,RandomizerCheck>StaticData::locationNameToEnum,StaticData::excludedLocationToEnum;
std::multimap<std::tuple<s16,s16,s32>,RandomizerCheck>StaticData::CheckFromActorMultimap;
Location* StaticData::GetLocation(RandomizerCheck check){return &locationTable[check];}
std::array<Location,RC_MAX>& StaticData::GetLocationTable(){return locationTable;}
bool DungeonInfo::IsMQ() const {return false;}
namespace Fishsanity {bool GetFishLocationIncluded(Location*,FishsanityOptionsSource){return false;}}
'''
for sig in ('OptionValue::OptionValue(uint8_t', 'uint8_t OptionValue::Get(', 'void OptionValue::Set(', 'OptionValue::operator bool(', 'RandomizerSettingKey Option::GetKey('):source+=function(option,sig)+'\n'
for sig in ('ItemLocation::ItemLocation()', 'bool ItemLocation::IsExcluded(', 'void ItemLocation::SetExcludedOption(', 'void ItemLocation::MergeAreas(', 'std::set<RandomizerArea> ItemLocation::GetAreas('):source+=function(itemloc,sig)+'\n'
source+=function((r/'location_list.cpp').read_text(),'void Rando::StaticData::InitHashMaps(').replace('Rando::StaticData::','StaticData::')+'\n'
for sig in ('std::set<RandomizerCheck> StaticData::ParseExcludedLocations(', 'std::string StaticData::SerializeExcludedLocations('):source+=function((r/'static_data.cpp').read_text(),sig)+'\n'
source+='}\n'
source+=r'''
struct PoolDungeons{std::vector<Rando::DungeonInfo*> GetDungeonList(){return {};}};
struct PoolFixtureContext {
 std::vector<RandomizerCheck> allLocations,overworldLocations;
 std::array<Rando::OptionValue,RSK_MAX> mOptions;
 std::map<RandomizerCheck,Rando::ItemLocation> locations;
 std::map<RandomizerCheck,RandomizerGet> placements;
 std::set<RandomizerGet> possibleIceTrapModels;
 RandoOptionCheckTrigger mGBKCondition=RO_CHECK_TRIGGER_NONE,mGanonsSoulCondition=RO_CHECK_TRIGGER_NONE;
 PoolDungeons dungeons;
 PoolDungeons*GetDungeons(){return &dungeons;}
 Rando::DungeonInfo*GetDungeon(size_t){abort();}
 Rando::DungeonInfo*GetDungeonFromScene(SceneID){abort();}
 Rando::OptionValue&GetOption(RandomizerSettingKey key){return mOptions[key];}
 void AddLocation(RandomizerCheck check,std::vector<RandomizerCheck>* destination=nullptr){(destination?destination:&allLocations)->push_back(check);}
 Rando::ItemLocation*GetItemLocation(RandomizerCheck check){return &locations[check];}
 void PlaceItemInLocation(RandomizerCheck check,RandomizerGet item){placements[check]=item;}
 static PoolFixtureContext*GetInstance();
 static std::vector<RandomizerCheck>GetLocations(const std::vector<RandomizerCheck>&,RandomizerCheckType);
 void GenerateLocationPool();
} state;auto*ctx=&state;
PoolFixtureContext*PoolFixtureContext::GetInstance(){return &state;}
using namespace Rando;
'''
source+=function(seed,'void Context::GenerateLocationPool(').replace('Context::','PoolFixtureContext::')+'\n'
source+=function(seed,'std::vector<RandomizerCheck> Context::GetLocations(').replace('Context::','PoolFixtureContext::')+'\n'
source+='std::vector<RandomizerGet>itemPool,junkPool,lesserPool,plentifulPool;\n'
a=pool.index('const std::map<RandomizerGet, std::vector<RandomizerGet>*> poolForItem');b=pool.index('\n};',a)+3;source+=pool[a:b]+'\n'
source+=function(pool,'void AddFixedItemToPool(').replace('int count = 1','int count').replace('bool iceTrapModel = true','bool iceTrapModel')+'\n'
# Production PlaceItemsForType relies on the native default args of AddFixedItemToPool.
source=source.replace('int count, bool iceTrapModel)', 'int count = 1, bool iceTrapModel = true)')
source+=function(pool,'static void PlaceItemsForType(')+'\n'
start=pool.index('    // Each global first-type check contributes');end=pool.index('    // Shuffle Wonder Items',start)
source+='void AddFirstDropItems(){\n'+pool[start:end]+'}\n'
source+='RandomizerGet GetJunkItem(){return RG_BLUE_RUPEE;}\n'
source+=function((r/'3drando/fill.cpp').read_text(),'static void FillExcludedLocations(').replace('Rando::Context::','PoolFixtureContext::')+'\n'
fill=(r/'3drando/fill.cpp').read_text()
a=fill.index('[dungeon, ctx](const auto loc) {',fill.index('static void RandomizeOwnDungeon('));b=fill.index('});',a)
source+='struct PhysicalDungeon{RandomizerArea GetArea(){return RA_FOREST_TEMPLE;}};\n'
source+='bool OwnDungeonAllows(RandomizerCheck check){auto*ctx=::ctx;PhysicalDungeon physical;auto*dungeon=&physical;auto predicate='+fill[a:b+1]+';return predicate(check); }\n'
source+=r'''
int main(){
 EnemyFirstDrops::RegisterLocations();
 StaticData::InitHashMaps();
 std::set<RandomizerCheck>allDrops;
 for(auto&entry:EnemyFirstDrops::Catalog)allDrops.insert(entry.check);
 REQUIRE(StaticData::ParseExcludedLocations(StaticData::SerializeExcludedLocations(allDrops))==allDrops);
 for(bool mode:{false,true}) {
  combined=mode;
#ifdef COMBO_BUILD
 gComboGameMode=mode?COMBO_GAME_BOTH:COMBO_GAME_OOT;
#endif
 ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Set(0);ctx->GenerateLocationPool();
  REQUIRE(ctx->allLocations.empty());AddFirstDropItems();REQUIRE(junkPool.empty());
  ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Set(1);ctx->GenerateLocationPool();
  REQUIRE(ctx->allLocations.size()==43);REQUIRE(ctx->overworldLocations.size()==43);
  AddFirstDropItems();REQUIRE(junkPool.size()==43);junkPool.clear();
  for(auto&entry:EnemyFirstDrops::Catalog){
   auto*location=StaticData::GetLocation(entry.check);
   REQUIRE(!location->IsDungeon());REQUIRE(location->GetScene()==SCENE_ID_MAX);
   REQUIRE(location->GetRCType()==RCTYPE_ENEMY_DROP);
   REQUIRE(location->GetCollectionCheck().flag==entry.flag);
   REQUIRE(location->GetName().starts_with("Global Enemy Drop: "));
   REQUIRE((int)location->GetExcludedOption()->GetKey()==(int)entry.check);
   ctx->GetItemLocation(entry.check)->SetExcludedOption(RO_LOCATION_EXCLUDE);
  }
  // Source-native own-dungeon filter must reject a global reward even when
  // one of its logical witnesses has inherited a real dungeon hint area.
  for(auto&entry:EnemyFirstDrops::Catalog){
   ctx->GetItemLocation(entry.check)->MergeAreas({RA_FOREST_TEMPLE});
   REQUIRE(!OwnDungeonAllows(entry.check));
  }
  FillExcludedLocations();REQUIRE(ctx->placements.size()==43);
  for(auto&[check,item]:ctx->placements)REQUIRE(item==RG_BLUE_RUPEE);
  ctx->placements.clear();
 }
 *StaticData::GetLocation(RC_KF_KOKIRI_SWORD_CHEST)=Location::Base(
  RC_KF_KOKIRI_SWORD_CHEST,RCQUEST_BOTH,RCTYPE_STANDARD,RCAREA_DEKU_TREE,
  ACTOR_EN_BOX,SCENE_DEKU_TREE,0,std::string("Positive control"),std::string("Positive control"),RHT_NONE,RG_BLUE_RUPEE);
 ctx->GetItemLocation(RC_KF_KOKIRI_SWORD_CHEST)->MergeAreas({RA_FOREST_TEMPLE});
 REQUIRE(OwnDungeonAllows(RC_KF_KOKIRI_SWORD_CHEST));
 puts("PASS actual native catalogue/location-pool/item-pool/excluded-fill algorithms, solo and unified contexts");
}
'''
with tempfile.TemporaryDirectory(prefix='oot-first-drop-pool-') as t:
 t=Path(t);cpp=t/'test.cpp';cpp.write_text(source)
 flags=['-std=c++20','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DSOH_BUILD_DLL','-I'+str(ROOT)]
 for path in ('soh/include','soh/include/PR','soh/src','soh','soh/assets','soh/soh','libultraship/include','libultraship/src','combo','combo/menu'):
  flags+=['-isystem',str(ROOT/path)]
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),str(r/'EnemyFirstDrops.cpp'),str(r/'location.cpp'),str(r/'randomizerEnumStrings.cpp'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
