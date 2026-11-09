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
from layout_test_data import expected_layout_masks
scenes,expected=expected_layout_masks()
source=r'''
#include "soh/Enhancements/randomizer/EnemyFirstDrops.h"
#include "soh/Enhancements/randomizer/RustyDoorKeys.h"
#include "soh/Enhancements/randomizer/hint.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/fishsanity.h"
#include "soh/Enhancements/randomizer/3drando/pool_functions.hpp"
#include <cstdio>
#include "NeiGracePolicy.h"
#include "NeiSeasonsPolicy.h"
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
std::array<Item,RG_MAX>StaticData::itemTable;
std::array<HintText,RHT_MAX> StaticData::hintTextTable;
std::unordered_map<RandomizerHint,StaticHintInfo>StaticData::staticHintInfoMap;
std::unordered_map<uint32_t,RandomizerHintTextKey>StaticData::areaNames;
std::unordered_map<std::string,RandomizerSettingKey>StaticData::optionNameToEnum;
std::unordered_map<std::string,RandomizerCheck>StaticData::locationNameToEnum,StaticData::excludedLocationToEnum;
std::multimap<std::tuple<s16,s16,s32>,RandomizerCheck>StaticData::CheckFromActorMultimap;
Location* StaticData::GetLocation(RandomizerCheck check){return &locationTable[check];}
std::array<Location,RC_MAX>& StaticData::GetLocationTable(){return locationTable;}
DungeonInfo::~DungeonInfo() = default;
namespace Fishsanity {bool GetFishLocationIncluded(Location*,FishsanityOptionsSource){return false;}}
'''
for sig in ('OptionValue::OptionValue(uint8_t', 'uint8_t OptionValue::Get(', 'void OptionValue::Set(', 'OptionValue::operator bool(', 'RandomizerSettingKey Option::GetKey(', 'void Option::Hide(', 'void Option::Unhide(', 'bool Option::IsHidden('):source+=function(option,sig)+'\n'
for sig in ('ItemLocation::ItemLocation()', 'bool ItemLocation::IsExcluded(', 'void ItemLocation::SetExcludedOption(', 'void ItemLocation::SetAsHintable(', 'RandomizerGet ItemLocation::GetPlacedRandomizerGet(', 'void ItemLocation::SetPlacedItem(', 'void ItemLocation::SetPrice(', 'bool ItemLocation::IsHintable(', 'void ItemLocation::SetHidden(', 'bool ItemLocation::IsAHintAccessible(', 'void ItemLocation::SetHintAccesible(', 'void ItemLocation::MergeAreas(', 'std::set<RandomizerArea> ItemLocation::GetAreas('):source+=function(itemloc,sig)+'\n'
source+=function((r/'location_list.cpp').read_text(),'void Rando::StaticData::InitHashMaps(').replace('Rando::StaticData::','StaticData::')+'\n'
for sig in ('std::set<RandomizerCheck> StaticData::ParseExcludedLocations(', 'std::string StaticData::SerializeExcludedLocations('):source+=function((r/'static_data.cpp').read_text(),sig)+'\n'
dungeon=(r/'dungeon.cpp').read_text()
for sig in ('DungeonInfo::DungeonInfo()', 'void DungeonInfo::SetMQ(', 'void DungeonInfo::ClearMQ(', 'bool DungeonInfo::IsMQ('):source+=function(dungeon,sig)+'\n'
for sig in ('Item::Item()', 'uint16_t Item::GetPrice('):source+=function((r/'item.cpp').read_text(),sig)+'\n'
source+=function((r/'item_list.cpp').read_text(),'Item& Rando::StaticData::RetrieveItem(').replace('Rando::StaticData::','StaticData::')+'\n'
source+=function((r/'location_list.cpp').read_text(),'std::vector<RandomizerCheck> Rando::StaticData::GetGossipStoneLocations(').replace('Rando::StaticData::','StaticData::')+'\n'
source+='}\n'
# Native hint/message storage; only the unrelated formatting boundary is isolated.
source+=(ROOT/'soh/soh/Enhancements/custom-message/text.cpp').read_text().splitlines()[5]+'\n'
message=(ROOT/'soh/soh/Enhancements/custom-message/CustomMessageManager.cpp').read_text()
for sig in ('CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_, TextBoxType', 'CustomMessage::CustomMessage(std::string english_, TextBoxType', 'const std::vector<std::string> CustomMessage::GetAllMessages('):source+=function(message,sig)+'\n'
source+='using namespace std::literals::string_literals;\n'
for map_name in ('colorToPercent','percentColors'):
 source+=re.search(r'static const std::unordered_map<[^\n]+> '+map_name+r' = \{.*?\n\};',message,re.S)[0]+'\n'
for sig in ('CustomMessage::CustomMessage(std::string english_, std::string german_, std::string french_,\n',
            'void CustomMessage::EncodeColors(', 'void CustomMessage::ReplaceColors(', 'std::string CustomMessage::COLOR(',
            'void CustomMessage::Replace(std::string&& oldStr, CustomMessage newMessage)',
            'void CustomMessage::ProcessMessageFormat('):
 source+=function(message,sig)+'\n'
source+='void CustomMessage::FormatString(std::string&)const {abort();}\nvoid CustomMessage::CleanString(std::string&)const {abort();}\nvoid CustomMessage::AutoFormatString(std::string&)const {abort();}\n'

item_source=(r/'item.cpp').read_text()
constructors=list(re.finditer(r'Item::Item\(const RandomizerGet',item_source))
a=constructors[-1].start();b=item_source.index('\n}',a)+2
source+='namespace Rando {\n'+item_source[a:b]+'\n'+function(item_source,'const Text& Item::GetName(')+'\n}\n'
text_source=(ROOT/'soh/soh/Enhancements/custom-message/text.cpp').read_text()
source+=function(text_source,'Text::Text(std::string english_, std::string french_, std::string german_)')+'\n'
source+=function(text_source,'const std::string& Text::GetEnglish(')+'\n'
hints=(r/'3drando/hints.cpp').read_text()
for sig in ('HintText::HintText(CustomMessage', 'const CustomMessage& HintText::GetClear('):source+=function(hints,sig)+'\n'
source+='constexpr SceneID layoutScenes[]={'+','.join(scenes)+'};\n'
source+='constexpr uint64_t expectedMasks[]={'+','.join(str(mask)+'ULL' for mask in expected)+'};\n'
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
 std::array<Rando::DungeonInfo,12> layouts;
 struct TrialBoundary{bool IsRequired(){return true;}};
 TrialBoundary*GetTrial(TrialKey){static TrialBoundary trial;return &trial;}
 struct LogicBoundary{int resets=0;void Reset(){++resets;}} logic;
 LogicBoundary*GetLogic(){return &logic;}
 PoolDungeons*GetDungeons(){return &dungeons;}
 Rando::DungeonInfo*GetDungeon(size_t){abort();}
 Rando::DungeonInfo*GetDungeonFromScene(SceneID scene){for(size_t i=0;i<12;++i)if(layoutScenes[i]==scene)return &layouts[i];return nullptr;}
 Rando::OptionValue&GetOption(RandomizerSettingKey key){return mOptions[key];}
 void AddLocation(RandomizerCheck check,std::vector<RandomizerCheck>* destination=nullptr){(destination?destination:&allLocations)->push_back(check);}
 Rando::ItemLocation*GetItemLocation(RandomizerCheck check){return &locations[check];}
 void PlaceItemInLocation(RandomizerCheck check,RandomizerGet item){placements[check]=item;}
 static PoolFixtureContext*GetInstance();
 static std::vector<RandomizerCheck>GetLocations(const std::vector<RandomizerCheck>&,RandomizerCheckType);
 void SetSeedString(std::string){}
 void SetSeed(uint32_t){}
 void GenerateLocationPool();
 void UpdateEnemyDropExclusions();
} state;auto*ctx=&state;
PoolFixtureContext*PoolFixtureContext::GetInstance(){return &state;}
using namespace Rando;
'''
for sig in ('const CustomMessage& HintText::GetObscure(size_t', 'const CustomMessage& HintText::GetAmbiguous(size_t', 'const CustomMessage& HintText::GetHintMessage('):source+=function(hints,sig).replace('Rando::Context::GetInstance()','PoolFixtureContext::GetInstance()')+'\n'
hint=(r/'hint.cpp').read_text()
for sig in ('Hint::Hint()', 'void Hint::AddHintedLocation(', 'void Hint::SetHintType(', 'const HintText Hint::GetHintText(', 'const HintText Hint::GetAreaHintText(', 'const CustomMessage Hint::GetAreaName(', 'void Hint::SetForeignAreas('):source+='namespace Rando {\n'+function(hint,sig).replace('Rando::Context::GetInstance()','PoolFixtureContext::GetInstance()')+'\n}\n'
templates=(r/'3drando/hint_list.cpp').read_text()
source+='void InitSariaColorTemplates(){auto&hintTextTable=StaticData::hintTextTable;\n'
for key in ('RHT_SARIA_TALK_HINT','RHT_SARIA_SONG_HINT'):
 a=templates.index('hintTextTable['+key+'] =');b=templates.index('));',a)+3
 source+=templates[a:b]+'\n'
source+='}\n'
# Reachability itself is the explicit boundary: verify the native hint search
# removes its own reward, resets logic, and restores the saved placement.
source+='RandomizerCheck testedHint;bool stoneReachable=false;\nstd::vector<RandomizerCheck>ReachabilitySearch(std::vector<RandomizerCheck> stones){REQUIRE(ctx->GetItemLocation(testedHint)->GetPlacedRandomizerGet()==RG_NONE);return stoneReachable?stones:std::vector<RandomizerCheck>{};}\n'
for sig in ('static std::vector<RandomizerCheck> GetAccessibleGossipStones(', 'static std::vector<RandomizerCheck> FilterHintability('):source+=function(hints,sig).replace('Rando::Context::GetInstance()','PoolFixtureContext::GetInstance()').replace('= NoFilter','= [](RandomizerCheck){return true;}')+'\n'
source+=r'''
struct SettingTextBoundary {
 uint8_t GetValueFromText(const std::string& text){REQUIRE(text=="On"||text=="Off");return text=="On";}
 void SetContextIndex(uint8_t){}
};
struct PoolFixtureSettings {
 PoolFixtureContext*mContext=ctx;
 std::array<SettingTextBoundary,RSK_MAX>mOptions;
 std::unordered_map<std::string,RandomizerTrick>mTrickNameToEnum;
 SettingTextBoundary&GetTrickSetting(RandomizerTrick){return mOptions[0];}
 void ParseJson(const nlohmann::json&);
};
'''
source+=function((r/'settings.cpp').read_text(),'void Settings::ParseJson(').replace('Settings::','PoolFixtureSettings::').replace('Context::GetInstance()','PoolFixtureContext::GetInstance()')+'\n'
source+=function(seed,'void Context::UpdateEnemyDropExclusions(').replace('Context::','PoolFixtureContext::')+'\n'
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
# The actual native loader handles appended options/flag words by defaulting
# missing array entries. Exercise it with a previous-size seed/save document.
save_header=(ROOT/'soh/soh/SaveManager.h').read_text()
a=save_header.index('    template <typename T> void LoadData(');b=save_header.index('    void LoadCharArray',a)
source+='struct SavedArrayBoundary{\nusing LoadArrayFunc=std::function<void(size_t)>;\nnlohmann::json document; nlohmann::json*currentJsonContext=&document; nlohmann::json::iterator currentJsonArrayContext;\n'+save_header[a:b]+'void LoadArray(const std::string&,size_t,LoadArrayFunc);};\n'
source+=function((ROOT/'soh/soh/SaveManager.cpp').read_text(),'void SaveManager::LoadArray(').replace('SaveManager::','SavedArrayBoundary::')+'\n'
spoiler=(r/'3drando/spoiler_log.cpp').read_text()
source+=re.search(r'^std::map<RandomizerHintTextKey, Rando::ItemLocation\*> hintedLocations;',spoiler,re.M)[0]+'\n'
spoiler_insert=re.search(r'        if \(location->IsAHintAccessible\(\)\) \{\n            hintedLocations.emplace.*?\n        \}',spoiler,re.S)[0]
source+='void RecordSpoilerHint(RandomizerCheck key){auto*location=ctx->GetItemLocation(key);\n'+spoiler_insert+'\n}\n'
exporter=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
source+='static std::string sCachedHintText;static RandomizerCheck sensorCheck=RC_UNKNOWN_CHECK;\n'
source+='static RandomizerGet SensorGetDesire(s32 slot){return slot==0?RG_MM_SOUL_GOHT:RG_NONE;}\n'
source+='static RandomizerCheck FindOutstandingCheckFor(RandomizerGet){return sensorCheck;}\n'
source+='static const char*GetVagueLocationDesc(RandomizerCheckType){return "ordinary control";}\n'
source+='namespace RandomizerCheckObjects {std::string GetRCAreaName(RandomizerCheckArea){return "Hyrule Field";}}\n'
source+='#ifndef SENSOR_DESIRE_SLOTS\n#define SENSOR_DESIRE_SLOTS 5\n#endif\n'
source+=function(exporter,'extern "C" u8 Randomizer_SensorBuildHint(')+'\n'
source+=r'''
int main(){
 SavedArrayBoundary saved;
 std::vector<unsigned>oldOptions(RSK_SHUFFLE_ENEMY_DROPS,0);oldOptions.back()=1;
 saved.document["randoSettings"]=oldOptions;ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Set(1);
 saved.LoadArray("randoSettings",RSK_MAX,[&](size_t i){int value=0;saved.LoadData("",value);ctx->GetOption((RandomizerSettingKey)i).Set(value);});
 REQUIRE(ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Get()==0);
 REQUIRE(ctx->GetOption((RandomizerSettingKey)(RSK_SHUFFLE_ENEMY_DROPS-1)).Get()==1);
 std::vector<u16>oldFlags((RAND_INF_ENEMY_DROP_STALFOS+15)/16,0);
 auto oldLast=RAND_INF_ENEMY_DROP_STALFOS-1;oldFlags[oldLast>>4]|=1u<<(oldLast&15);
 saved.document["randomizerInf"]=oldFlags;
 std::array<u16,(RAND_INF_MAX+15)/16>loadedFlags;loadedFlags.fill(0xffff);
 saved.LoadArray("randomizerInf",loadedFlags.size(),[&](size_t i){saved.LoadData("",loadedFlags[i]);});
 REQUIRE(loadedFlags[oldLast>>4]&(1u<<(oldLast&15)));
 for(auto&entry:EnemyFirstDrops::Catalog){
  REQUIRE(!(loadedFlags[entry.flag>>4]&(1u<<(entry.flag&15))));
  REQUIRE(!(loadedFlags[entry.defeatedFlag>>4]&(1u<<(entry.defeatedFlag&15))));
 }
 REQUIRE(EnemyFirstDrops::SourceNames(*EnemyFirstDrops::Find(RC_ENEMY_DROP_OCTOROK)).de=="Oktorok");
 REQUIRE(EnemyFirstDrops::SourceNames(*EnemyFirstDrops::Find(RC_ENEMY_DROP_TEKTITE)).fr=="Araknon");
 REQUIRE(EnemyFirstDrops::SourceNames(*EnemyFirstDrops::Find(RC_ENEMY_DROP_FLARE_DANCER)).fr=="Danse-Flamme");
 EnemyFirstDrops::RegisterLocations();
 InitSariaColorTemplates();
 StaticData::RetrieveItem(RG_MM_SOUL_GOHT)=Item(RG_MM_SOUL_GOHT,Text("Soul of Goht","Âme de Goht","Seele von Goht"),ITEMTYPE_ITEM,0,true,LOGIC_NONE,RHT_ITEM_MM_SOUL_GOHT,ITEM_CATEGORY_MAJOR);
 sensorCheck=EnemyFirstDrops::Catalog[0].check;
 REQUIRE(Randomizer_SensorBuildHint());
 REQUIRE(sCachedHintText=="The slate senses %gSoul of Goht%w&in the first %rStalfos%w drop...");
 {CustomMessage message(sCachedHintText);auto bytes=sCachedHintText;message.ReplaceColors(bytes);
  REQUIRE(bytes.find(CustomMessage::COLOR(QM_GREEN)+"Soul of Goht"+CustomMessage::COLOR(QM_WHITE))!=std::string::npos);
  REQUIRE(bytes.find(CustomMessage::COLOR(QM_RED)+"Stalfos"+CustomMessage::COLOR(QM_WHITE))!=std::string::npos);}

 std::set<RandomizerHintTextKey>hintKeys;
 for(auto&entry:EnemyFirstDrops::Catalog){
  auto*location=StaticData::GetLocation(entry.check);
  REQUIRE(location->GetHintKey()!=RHT_NONE);
  for(auto type:{HINT_TYPE_ITEM,HINT_TYPE_ITEM_AREA}){
   Hint nativeHint;nativeHint.AddHintedLocation(entry.check);nativeHint.SetHintType(type);
   auto messages=nativeHint.GetHintText().GetClear().GetAllMessages(MF_RAW);
   REQUIRE(messages[0].find(entry.name)!=std::string::npos);
   REQUIRE(messages[0].find("[[1]]")!=std::string::npos);
   const auto localized=EnemyFirstDrops::SourceNames(entry);
   const std::array<std::string,3> enemyNames={localized.en,localized.de,localized.fr};
   const auto source=EnemyFirstDrops::HintSource(entry);
   const std::array<std::string,3> sourceNames={source.en,source.de,source.fr};
   for(auto key:{RHT_SARIA_TALK_HINT,RHT_SARIA_SONG_HINT}){
    auto saria=StaticData::hintTextTable[key].GetClear();
    saria.Replace("[[1]]",nativeHint.GetAreaName(0));
    auto encodedSaria=saria.GetAllMessages(MF_ENCODE);
    for(size_t language=0;language<encodedSaria.size();++language){
     REQUIRE(encodedSaria[language].find("%r"+sourceNames[language]+"%w")!=std::string::npos);
     REQUIRE(encodedSaria[language].find("%g")!=std::string::npos);
     REQUIRE(encodedSaria[language].find("%w%w")==std::string::npos);
    }
   }
   nativeHint.SetForeignAreas({"Woodfall Temple (MM)"});
   REQUIRE(nativeHint.GetAreaName(0).GetAllMessages(MF_RAW)[0]=="Woodfall Temple (MM)");
   nativeHint.SetForeignAreas({});
   auto colored=nativeHint.GetHintText().GetClear();
   colored.Replace("[[1]]",CustomMessage("Soul of Goht","Seele von Goht","Âme de Goht"));
   const auto encoded=colored.GetAllMessages(MF_ENCODE);
   const std::array<std::string,3> rewards={"Soul of Goht","Seele von Goht","Âme de Goht"};
   for(size_t language=0;language<encoded.size();++language){
    REQUIRE(encoded[language].find("%r"+enemyNames[language]+"%w")!=std::string::npos);
    REQUIRE(encoded[language].find("%g"+rewards[language]+"%w")!=std::string::npos);
    REQUIRE(encoded[language].find('#')==std::string::npos);
    REQUIRE(encoded[language].find("%w%w")==std::string::npos);
    auto bytes=encoded[language];colored.ReplaceColors(bytes);
    REQUIRE(bytes.find(CustomMessage::COLOR(QM_RED)+enemyNames[language]+CustomMessage::COLOR(QM_WHITE))!=std::string::npos);
    REQUIRE(bytes.find(CustomMessage::COLOR(QM_GREEN)+rewards[language]+CustomMessage::COLOR(QM_WHITE))!=std::string::npos);
   }
  }
  REQUIRE(hintKeys.insert(location->GetHintKey()).second);
  const auto rawMessages=location->GetHint()->GetClear().GetAllMessages(MF_RAW);
  const auto localized=EnemyFirstDrops::SourceNames(entry);
  const std::array<std::string,3> nativeNames={localized.en,localized.de,localized.fr};
  REQUIRE(rawMessages.size()==nativeNames.size());
  for(size_t language=0;language<rawMessages.size();++language){
   REQUIRE(rawMessages[language].find(nativeNames[language])!=std::string::npos);
   REQUIRE(rawMessages[language].find("[[1]]")!=std::string::npos);
  }
 }
 REQUIRE(hintKeys.size()==43);
 auto stone=RC_KF_GOSSIP_STONE;
 *StaticData::GetLocation(stone)=Location::Base(stone,RCQUEST_BOTH,RCTYPE_GOSSIP_STONE,RCAREA_KOKIRI_FOREST,ACTOR_ID_MAX,SCENE_ID_MAX,0,std::string("Stone"),std::string("Stone"),RHT_NONE,RG_HINT);
 for(auto&entry:EnemyFirstDrops::Catalog){
  testedHint=entry.check;auto*location=ctx->GetItemLocation(entry.check);
  location->SetPlacedItem(RG_KOKIRI_SWORD);
  stoneReachable=false;REQUIRE(GetAccessibleGossipStones(entry.check).empty());
  REQUIRE(location->GetPlacedRandomizerGet()==RG_KOKIRI_SWORD);
  stoneReachable=true;REQUIRE(GetAccessibleGossipStones(entry.check)==std::vector<RandomizerCheck>{stone});
  REQUIRE(location->GetPlacedRandomizerGet()==RG_KOKIRI_SWORD);
  std::vector<RandomizerCheck>candidates={entry.check};
  REQUIRE(FilterHintability(candidates).empty());
  location->SetAsHintable();REQUIRE(FilterHintability(candidates)==candidates);
  location->SetHidden(true);REQUIRE(FilterHintability(candidates).empty());location->SetHidden(false);
  REQUIRE(FilterHintability(candidates,[](RandomizerCheck){return false;}).empty());
  location->SetHintAccesible();REQUIRE(FilterHintability(candidates).empty());
  RecordSpoilerHint(entry.check);
 }
 REQUIRE(ctx->logic.resets==86);
 REQUIRE(hintedLocations.size()==43);
 StaticData::InitHashMaps();
 StaticData::optionNameToEnum["Shuffle Enemy First Drops"]=RSK_SHUFFLE_ENEMY_DROPS;
 PoolFixtureSettings settings;
 auto oldSeed=nlohmann::json{{"seed","historical"},{"finalSeed",1}};
 auto newSeed=oldSeed;newSeed["settings"]={{"Shuffle Enemy First Drops","On"}};
 ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(RustyDoorKeys::CurrentCatalog);
 ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Set(1);
 settings.ParseJson(newSeed);REQUIRE(ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Get()==1);
 REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==RustyDoorKeys::LegacyCatalog);
 REQUIRE(ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Get()==1);
 ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(RustyDoorKeys::CurrentCatalog);
 settings.ParseJson(oldSeed);REQUIRE(ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Get()==0);
 REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==RustyDoorKeys::LegacyCatalog);
 std::set<RandomizerCheck>allDrops;
 for(auto&entry:EnemyFirstDrops::Catalog)allDrops.insert(entry.check);
 REQUIRE(StaticData::ParseExcludedLocations(StaticData::SerializeExcludedLocations(allDrops))==allDrops);
 for(bool mode:{false,true})for(unsigned mask=0;mask<4096;++mask) {
  for(size_t i=0;i<12;++i){if(mask&(1u<<i))ctx->layouts[i].SetMQ();else ctx->layouts[i].ClearMQ();}
  size_t expectedCount=__builtin_popcountll(expectedMasks[mask]);
  combined=mode;
#ifdef COMBO_BUILD
 gComboGameMode=mode?COMBO_GAME_BOTH:COMBO_GAME_OOT;
#endif
 ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Set(0);ctx->GenerateLocationPool();
  REQUIRE(ctx->allLocations.empty());AddFirstDropItems();REQUIRE(junkPool.empty());
  ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS).Set(1);ctx->GenerateLocationPool();
  REQUIRE(ctx->allLocations.size()==expectedCount);REQUIRE(ctx->overworldLocations.size()==expectedCount);
  AddFirstDropItems();REQUIRE(junkPool.size()==expectedCount);junkPool.clear();
  size_t entryIndex=0;
  for(auto&entry:EnemyFirstDrops::Catalog){
   bool expected=(expectedMasks[mask]&(1ULL<<entryIndex++))!=0;
   bool active=EnemyFirstDrops::IsAvailable(entry.check,[](SceneID scene){auto*d=ctx->GetDungeonFromScene(scene);return d&&d->IsMQ();});
   REQUIRE(active==expected);
   REQUIRE((std::find(ctx->allLocations.begin(),ctx->allLocations.end(),entry.check)!=ctx->allLocations.end())==expected);
   auto*location=StaticData::GetLocation(entry.check);
   REQUIRE(location->GetExcludedOption()->IsHidden()==!expected);
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
  FillExcludedLocations();REQUIRE(ctx->placements.size()==expectedCount);
  for(auto&[check,item]:ctx->placements)REQUIRE(item==RG_BLUE_RUPEE);
  ctx->placements.clear();
 }
 *StaticData::GetLocation(RC_KF_KOKIRI_SWORD_CHEST)=Location::Base(
  RC_KF_KOKIRI_SWORD_CHEST,RCQUEST_BOTH,RCTYPE_STANDARD,RCAREA_DEKU_TREE,
  ACTOR_EN_BOX,SCENE_DEKU_TREE,0,std::string("Positive control"),std::string("Positive control"),RHT_NONE,RG_BLUE_RUPEE);
 ctx->GetItemLocation(RC_KF_KOKIRI_SWORD_CHEST)->MergeAreas({RA_FOREST_TEMPLE});
 REQUIRE(OwnDungeonAllows(RC_KF_KOKIRI_SWORD_CHEST));
 puts("PASS all4096 physical layout masks: native catalogue/location-pool/item-pool/exclusion/own-dungeon, solo and unified");
}
'''
with tempfile.TemporaryDirectory(prefix='oot-first-drop-pool-') as t:
 t=Path(t);cpp=t/'test.cpp';cpp.write_text(source)
 flags=['-O1','-std=c++20','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DSOH_BUILD_DLL','-I'+str(ROOT)]
 for path in ('soh/include','soh/include/PR','soh/src','soh','soh/assets','soh/soh','libultraship/include','libultraship/src','combo','combo/menu'):
  flags+=['-isystem',str(ROOT/path)]
 # Native option headers require the project's pinned ImGui. An explicit -I
 # from an older distro pkg-config wins over CPATH, even when CI already
 # provides the correct dependency. Prefer the actual supplied checkout.
 candidates=[os.environ.get('IMGUI_DIR',''),*os.environ.get('CPATH','').split(os.pathsep)]
 imgui=next((Path(p) for p in candidates if p and (Path(p)/'imgui.h').is_file()),None)
 if imgui is not None:
  flags+=['-I'+str(imgui)]
 else:
  pkg=subprocess.run(['pkg-config','--atleast-version=1.91','imgui'],capture_output=True,text=True)
  if pkg.returncode!=0:
   raise RuntimeError('Set IMGUI_DIR to the pinned ImGui v1.91.9b-docking checkout')
  import shlex
  flags+=shlex.split(subprocess.check_output(['pkg-config','--cflags','imgui'],text=True))
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),str(r/'EnemyFirstDrops.cpp'),str(r/'location.cpp'),str(r/'randomizerEnumStrings.cpp'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)

# Exercise the actual native settings identity body alongside pool policy.
import seed_identity_test
seed_identity_test.run()
