#!/usr/bin/env python3
"""Actual native reset/types/start-kit/Item_Give health; no graph substitute."""
import os,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
def block(source,signature):
 a=source.index(signature);b=source.index('{',a);e=b+1;depth=1
 while depth:depth+=(source[e]=='{')-(source[e]=='}');e+=1
 return source[a:e]
source=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
reset=block(source,'extern "C" COMBO_EXPORT void Combo_MM_Rando_Reset(void)')
a=source.index('        case RI_HEART_CONTAINER:',source.index('static void GiveItemForOracle'));b=source.index('        // Bottle',a);health=source[a:b]
parameter=(ROOT/'mm/src/code/z_parameter.c').read_text();a=parameter.index('    } else if ((item == ITEM_HEART_PIECE_2)')+11;b=parameter.index('    } else if ((item >= ITEM_SONG_SONATA)',a)+5;native_health=parameter[a:b]
nei=block((ROOT/'mm/mods/nei_save.cpp').read_text(),'extern "C" void Nei_InitNewSave(void)')
inventory=(ROOT/'mm/src/code/z_inventory.c').read_text()
tables='\n'.join(re.search(r'^(?:u32|u16|u8) '+name+r'\[[^\n=]* = \{.*?^\};',inventory,re.M|re.S)[0] for name in ('gEquipMasks','gEquipShifts','gItemSlots','gUpgradeMasks','gUpgradeShifts','gUpgradeCapacities'))
starting=(ROOT/'mm/2s2h/Rando/StartingItems.cpp').read_text()
starting_storage='\n'.join(block(starting,name) for name in ('std::vector<RandoItemId> GetStartingItemsFromSave(', 'void SetStartingItemsInSave('))
native_grant=block(starting,'void GrantStartingItems()')
config_reader=block(starting,'std::vector<RandoItemId> GetStartingItemsFromConfig()')
item_source=(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text();a=item_source.index('#define RI(');b=item_source.index('// clang-format off',a)
health_catalog=item_source[a:b]+'std::map<RandoItemId,RandoStaticItem> Items={\n'+'\n'.join(re.findall(r'^    RI\(RI_HEART_(?:CONTAINER|PIECE),[^\n]+',item_source,re.M))+'\n};\n#undef RI\n'+block(item_source,'RandoItemId GetItemIdFromName(')
restore=block(source,'extern "C" COMBO_EXPORT void Combo_MM_Rando_Restore(void)')
set_seed=block(source,'extern "C" COMBO_EXPORT void MM_SetComboRandoSeed(')
apply_prices=block(source,'static void Combo_MM_ApplyCheckPrices() {')
sram=(ROOT/'mm/src/code/z_sram_NES.c').read_text();native_player=re.search(r'^SavePlayerData sSaveDefaultPlayerData = \{.*?^\};',sram,re.M|re.S)[0]
song=(ROOT/'mm/2s2h/Rando/SongEvents.cpp').read_text();song_prefix=song[song.index('namespace SE ='):song.index('void ClearImportedMapping')]
song_bodies=song_prefix+block(song,'void RememberSavedMapping(')+'\n'+block(song,'void Generate(')+'\n'+block(song,'void RestoreForOracle(')+'\n}\n'
program='''
#include "Rando/Logic/Logic.h"
#include "Rando/Wallets.h"
#include "Rando/SongEvents.h"
#include "Rando/SoloEntrances.h"
#include "rando_capacity.h"
#include "combo/menu/ItemGrantAuditBridge.h"
#include <cassert>
#include <cstring>
#include <unordered_map>
#define COMBO_EXPORT
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
using namespace Rando::Logic;
int configuredHearts=3;
extern "C" { SaveContext gSaveContext{};PlayState* gPlayState=nullptr;int gComboGameMode=COMBO_GAME_BOTH;int gFcCombo_SuppressRecord=0;
void CVarClear(const char*){assert(false);}
unsigned char ExtEquip_HasItem(short,unsigned char){return 0;}
int32_t CVarGetInteger(const char* name,int32_t fallback){return !strcmp(name,"gRando.Options.RO_STARTING_HEALTH")?configuredHearts:fallback;}
'''+tables+'''
}
'''+native_player+'\n'+song_bodies+'''
namespace Rando::Logic {uint64_t gCurrentRegionTime=0;}
namespace Rando::StaticData {
'''+health_catalog+'''
std::map<RandoCheckId,RandoStaticCheck> Checks;
const std::string& GetCheckDisplayName(RandoCheckId){assert(false);static const std::string s;return s;}
const std::string& GetItemDisplayName(RandoItemId){assert(false);static const std::string s;return s;}
}
// Config input/computed non-health kit and audit output are service boundaries.
std::vector<RandoItemId> configuredKit;
bool malformedKit=false;
// Configuration/GUI transport is a boundary; the JSON reader/type error is native.
namespace Ship {
struct GuiBoundary{void SaveConsoleVariablesNextFrame(){assert(false);}};
struct WindowBoundary{GuiBoundary*GetGui(){assert(false);return nullptr;}};
struct ConfigBoundary{
 nlohmann::json GetNestedJson(){
  auto items=nlohmann::json::array();
  for(auto item:configuredKit)items.push_back(Rando::StaticData::Items.at(item).spoilerName);
  if(malformedKit)items=nlohmann::json::array({17});
  return nlohmann::json{{"CVars",{{"gRando",{{"StartingItems",items}}}}}};
 }
};
struct Context{
 static Context*GetRawInstance(){static Context value;return &value;}
 ConfigBoundary*GetConfig(){static ConfigBoundary value;return &value;}
 WindowBoundary*GetWindow(){assert(false);return nullptr;}
};
}
namespace Rando {
'''+config_reader+'''
'''+starting_storage+'''
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo&){return {};}
RandoItemId ConvertItem(RandoItemId item,RandoCheckId){return item;}
}
int auditSuspensions=0;
extern "C" void ItemGrantAudit_Suspend(){++auditSuspensions;}
extern "C" void ItemGrantAudit_Resume(){assert(auditSuspensions>0);--auditSuspensions;}
extern "C" void ItemGrantAudit_Begin(const char*,int,int,int){}
extern "C" void ItemGrantAudit_End(){}
std::unordered_map<uint32_t,uint16_t> sMMComboCheckPrices;
'''+apply_prices+'''
void GiveItemForOracle(RandoItemId);
bool sMM_OracleActive=false,sMM_OracleInventorySweepDone=false,sMMComboRandoSeedSet=true;
SaveContext sMM_OracleSavedContext{};
uint64_t sMM_OracleSavedRegionTime=0,sMMComboRandoSeed=1;
int gMMComboGoalHunt=0,gMMComboGoalRequired=0,gMMComboGoalPieces=-1;
'''+nei+'\n'+reset+'\n'+restore+'\n'+set_seed+'''
void GiveOracleHealth(RandoItemId ri){switch(ri){
'''+health+'''
default:assert(false);}}
u8 GiveNativeHealth(u8 item){
'''+native_health+'''
return ITEM_NONE;}
void GiveItemForOracle(RandoItemId item){GiveOracleHealth(item);}
namespace Rando {
void GiveItem(RandoItemId item,RandoCheckId){
 assert(item==RI_HEART_CONTAINER || item==RI_HEART_PIECE);
 GiveNativeHealth(item==RI_HEART_CONTAINER?ITEM_HEART_CONTAINER:ITEM_HEART_PIECE);
}
'''+native_grant+'''
}
struct OTRGlobals{static void*Instance;};void*OTRGlobals::Instance=nullptr;
namespace Rando{
 bool gComboDormantGive=false;
 void PrepareNewRandomizedSave(){}
 std::string GenerateSeedInSaveContext(std::string seed){gSaveContext.save.shipSaveInfo.rando.finalSeed=1234;return seed;}
}
extern "C" void Sram_InitNewSave(){gSaveContext.save.saveInfo.playerData=sSaveDefaultPlayerData;}
const char* MM_DumpRandoSettings(){return "{}";}
#include "mm/2s2h/Rando/Logic/SoloRandoHeadless.inc"
void SameHealth(const SaveContext& expected){
 assert(gSaveContext.save.saveInfo.playerData.healthCapacity==expected.save.saveInfo.playerData.healthCapacity);
 assert(gSaveContext.save.saveInfo.playerData.health==expected.save.saveInfo.playerData.health);
 assert(gSaveContext.save.saveInfo.inventory.questItems==expected.save.saveInfo.inventory.questItems);
}
int main(){
 for(int hearts:{1,3,20}){
 configuredHearts=hearts;gSaveContext.save.playerForm=PLAYER_FORM_FIERCE_DEITY;Combo_MM_Rando_Reset();
 assert(gSaveContext.save.playerForm==PLAYER_FORM_HUMAN && CAN_BE_HUMAN);
 assert(!CAN_BE_DEITY && !CAN_BE_GORON && !CAN_BE_ZORA && !CAN_BE_DEKU && !CAN_USE_SWORD);
 for(auto item:gSaveContext.save.shipSaveInfo.nei.ownedItems)assert(item==ITEM_NONE);
 for(auto item:gSaveContext.save.shipSaveInfo.nei.bottleSlots)assert(item==ITEM_NONE);
 assert(gSaveContext.save.shipSaveInfo.nei.bottomlessContent==ITEM_NONE);
 assert(gSaveContext.save.saveInfo.playerData.healthCapacity==hearts*0x10 && gSaveContext.save.saveInfo.playerData.health==hearts*0x10);
 }
 const std::vector<std::vector<RandoItemId>> kits={
  {},{RI_HEART_CONTAINER},{RI_HEART_PIECE},{RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE},
  {RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE},
  {RI_HEART_CONTAINER,RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE}};
 for(int hearts:{1,3,6,20})for(const auto&kit:kits){
  configuredHearts=hearts;configuredKit=kit;Combo_MM_Rando_Reset();auto oracle=gSaveContext;
  // Native new-save player data and complete native GrantStartingItems body.
  // GiveItem dispatches to the actual native Item_Give health block above.
  gSaveContext={};gSaveContext.save.saveInfo.playerData=sSaveDefaultPlayerData;
  gSaveContext.save.shipSaveInfo.rando=oracle.save.shipSaveInfo.rando;
  Rando::GrantStartingItems();auto native=gSaveContext;
  gSaveContext=oracle;SameHealth(native);
  for(auto item:{RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_PIECE,RI_HEART_CONTAINER}){
   auto before=gSaveContext;GiveNativeHealth(item==RI_HEART_CONTAINER?ITEM_HEART_CONTAINER:ITEM_HEART_PIECE);
   native=gSaveContext;gSaveContext=before;GiveItemForOracle(item);SameHealth(native);
  }
 }
 configuredKit.clear();configuredHearts=3;Combo_MM_Rando_Reset();gSaveContext.save.saveInfo.inventory.questItems=0x12345;
 for(int count=0;count<17;count++){
 auto before=gSaveContext;GiveNativeHealth(count==16?ITEM_HEART_CONTAINER:ITEM_HEART_PIECE);auto native=gSaveContext;gSaveContext=before;
 GiveOracleHealth(count==16?RI_HEART_CONTAINER:RI_HEART_PIECE);SameHealth(native);
 }
 Combo_MM_Rando_Restore();assert(auditSuspensions==0);
 RANDO_SAVE_CHECKS[RC_CURIOSITY_SHOP_SPECIAL_ITEM].price=123;
 RANDO_SAVE_CHECKS[RC_BOMB_SHOP_ITEM_03].price=456;
 auto beforePrices=gSaveContext;
 sMMComboCheckPrices={{RC_CURIOSITY_SHOP_SPECIAL_ITEM,199},{RC_BOMB_SHOP_ITEM_03,88}};
 const auto originalPrices=sMMComboCheckPrices;
 sMMComboCheckPrices={{RC_CURIOSITY_SHOP_SPECIAL_ITEM,17},{RC_BOMB_SHOP_ITEM_03,0}};
 Combo_MM_Rando_Reset();
 assert(RANDO_SAVE_CHECKS[RC_CURIOSITY_SHOP_SPECIAL_ITEM].price==17);
 assert(RANDO_SAVE_CHECKS[RC_BOMB_SHOP_ITEM_03].price==0);
 Combo_MM_Rando_Restore();assert(!memcmp(&beforePrices,&gSaveContext,sizeof(gSaveContext))&&auditSuspensions==0);
 sMMComboCheckPrices=originalPrices;
 memset(&gSaveContext,0x5a,sizeof(gSaveContext));auto live=gSaveContext;
 gComboGameMode=COMBO_GAME_MM;sMMComboRandoSeed=77;sMMComboRandoSeedSet=true;
 gCurrentRegionTime=91;auto*play=reinterpret_cast<PlayState*>(uintptr_t(0x1234));gPlayState=play;
 gFcCombo_SuppressRecord=9;malformedKit=true;
 auto failed=nlohmann::json::parse(MM_FillSoloRandoSeed(2027));
 assert(!failed["success"].get<bool>() && !sMM_OracleActive && auditSuspensions==0 && sMMComboCheckPrices==originalPrices);
 assert(!memcmp(&gSaveContext,&live,sizeof(live)) && gPlayState==play && gCurrentRegionTime==91);
 assert(sMMComboRandoSeed==77 && sMMComboRandoSeedSet && !Rando::gComboDormantGive && gFcCombo_SuppressRecord==9);
 malformedKit=false;configuredKit.clear();
 auto second=nlohmann::json::parse(MM_FillSoloRandoSeed(2028));
 assert(second["success"].get<bool>() && !sMM_OracleActive && auditSuspensions==0 && sMMComboCheckPrices==originalPrices);
 assert(!memcmp(&gSaveContext,&live,sizeof(live)) && gPlayState==play && sMMComboRandoSeed==77);
 // A malformed imported event mapping throws even before inventory clearing.
 gSaveContext.save.shipSaveInfo.rando.finalSeed=2029;
 gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]=1;
 memset(gSaveContext.save.shipSaveInfo.rando.songEvents,255,sizeof(gSaveContext.save.shipSaveInfo.rando.songEvents));
 live=gSaveContext;
 auto early=nlohmann::json::parse(MM_FillSoloRandoSeed(2029));
 assert(!early["success"].get<bool>() && !sMM_OracleActive && auditSuspensions==0);
 assert(!memcmp(&gSaveContext,&live,sizeof(live)) && gPlayState==play && sMMComboRandoSeed==77);
}
'''
with tempfile.TemporaryDirectory(prefix='mm-oracle-baseline-') as td:
 p=Path(td);cpp=p/'test.cpp';cpp.write_text(program);binary=p/'test';flags=['-std=c++20','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT/'tests/dungeon_rewards/headless.h')]
 for name in ('.','combo','mm/include','mm/include/PR','mm','mm/2s2h','mm/src','mm/assets','libultraship/include'):flags+=['-I'+str(ROOT/name)]
 subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),str(ROOT/'mm/2s2h/Rando/StaticData/Options.cpp'),str(ROOT/'mm/2s2h/Rando/SoloEntrances.cpp'),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
print('PASS actual native empty oracle/forms/custom slots + native GrantStartingItems/Item_Give health order for starts1/3/6/20 and pieces/containers; transactional native Reset failures and second-call recovery')
