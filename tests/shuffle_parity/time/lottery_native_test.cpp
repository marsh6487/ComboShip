// Production pool/cycle/catalogue services and actual native save types.
#define main KeyRingFoundationMain
#include "../key_rings/native_test.cpp"
#undef main
#include "Rando/Lottery.h"
#include "src/overlays/actors/ovl_En_Kujiya/z_en_kujiya.h"
#include <cstdarg>
#include <functional>
#include <map>
#include "lottery_json.inc"

namespace Rando::ActorBehavior { void InitEnKujiyaBehavior(); }
static std::function<void(bool*,va_list)> winHook;
static std::map<int,std::function<void(u16*,bool*)>> messageHooks;
#undef IS_RANDO
#undef COND_VB_SHOULD
#undef COND_ID_HOOK
#define IS_RANDO true
#define COND_VB_SHOULD(id, cond, ...) winHook = [](bool* should, va_list args) __VA_ARGS__
#define COND_ID_HOOK(kind, id, cond, ...) messageHooks[id] = __VA_ARGS__
static std::string message;
static RandoCheckId namedCheck;
namespace CustomMessage {
struct Entry { std::string msg; };
Entry LoadVanillaMessageTableEntry(u16) { return {}; }
void Replace(std::string* text,const char* from,const std::string& to) {
 auto pos=text->find(from); if(pos!=std::string::npos) text->replace(pos,strlen(from),to);
}
void LoadCustomMessageIntoFont(const Entry& entry) { message=entry.msg; }
}
namespace Rando::StaticData {
std::string LotteryFixtureItemName(RandoItemId item,bool,RandoCheckId check) {
 namedCheck=check; return std::to_string(item);
}
}
extern "C" void EnKujiya_Wait(EnKujiya*,PlayState*) {}
#define GetItemName LotteryFixtureItemName
#include "lottery_callbacks.inc"
#undef GetItemName
#include "lottery_tables.inc"
static bool win(int first,...) { bool should=true;va_list args;va_start(args,first);winHook(&should,args);va_end(args);return should; }
namespace Rando::Logic { uint64_t gCurrentRegionTime; }
#include "lottery_rules_test.inc"

int main() {
 using namespace Rando;
#include "../legacy_ids.inc"
 REQUIRE(StaticData::Options.at(RO_SHUFFLE_LOTTERY_NIGHTS).defaultValue==0);
 auto& region=Logic::Regions[static_cast<RandoRegionId>(0)];
 for(const auto&[id,row]:StaticData::Checks) if(id!=RC_UNKNOWN) region.checks[id]={[]{return true;},"catalogue membership"};
 const RandoCheckId ids[]={RC_CLOCK_TOWN_WEST_LOTTERY,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3};
 int baselinePurple=-1;
 for(bool on:{false,true}) for(int excludedMask=0;excludedMask<8;++excludedMask) {
  excluded.clear();for(int i=0;i<3;++i) if(excludedMask&(1<<i)) excluded.push_back(ids[i]);
  std::sort(excluded.begin(),excluded.end());
  auto save=std::make_unique<RandoSaveInfo>();
  for(const auto&[id,row]:StaticData::Options) save->randoSaveOptions[id]=row.defaultValue;
  save->randoSaveOptions[RO_SHUFFLE_LOTTERY_NIGHTS]=on;
  std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;
  Logic::GeneratePools(*save,checks,items);
  if(!on&&excludedMask==0) baselinePurple=std::count(items.begin(),items.end(),RI_RUPEE_PURPLE);
  const int excludedActive=(excludedMask&1?1:0)+(on&&bool(excludedMask&2)?1:0)+(on&&bool(excludedMask&4)?1:0);
  REQUIRE(std::count(items.begin(),items.end(),RI_RUPEE_PURPLE)==baselinePurple+2*int(on)-excludedActive);
  for(int i=0;i<3;++i) {
   const bool active=on||i==0;
   REQUIRE(std::count(checks.begin(),checks.end(),ids[i])==int(active&&!(excludedMask&(1<<i))));
   if(!active) REQUIRE(!save->randoSaveChecks[ids[i]].shuffled);
   REQUIRE(Lottery::IsEnabled(ids[i],on)==active);
  }
 }
 excluded.clear();
 // Exact native callback body, with registration and message-font services as boundaries.
 ActorBehavior::InitEnKujiyaBehavior();EnKujiya owner{};
 for(bool on:{false,true}) for(int day=1;day<=3;++day) {
  gSaveContext={};RANDO_SAVE_OPTIONS[RO_SHUFFLE_LOTTERY_NIGHTS]=on;gSaveContext.save.day=day;
  auto expected=on?ids[day-1]:ids[0];
  for(int i=0;i<3;++i) RANDO_SAVE_CHECKS[ids[i]].randoItemId=RandoItemId(RI_RUPEE_GREEN+i);
  for(int text:{0x2b5c,0x2b66}) {u16 id=text;bool load=true;messageHooks[text](&id,&load);REQUIRE(!load);REQUIRE(namedCheck==expected);REQUIRE(message.find(std::to_string(RANDO_SAVE_CHECKS[expected].randoItemId))!=std::string::npos);}
  REQUIRE(!win(0,&owner));REQUIRE(owner.actionFunc==EnKujiya_Wait);
  REQUIRE(RANDO_SAVE_CHECKS[expected].eligible);
  for(auto id:ids) if(id!=expected) REQUIRE(!RANDO_SAVE_CHECKS[id].eligible);
  auto& check=RANDO_SAVE_CHECKS[expected];check.eligible=false;check.obtained=true;check.cycleObtained=true;
  REQUIRE(!win(0,&owner));REQUIRE(check.eligible==!on);check.eligible=false;
  // Actual save JSON round-trip, then native Song-of-Time hooks clear cycle state.
  nlohmann::json j=check;check=j.get<RandoSaveCheck>();REQUIRE(check.obtained);
  static PlayState play{};gPlayState=&play;
  MiscBehavior::BeforeEndOfCycleSave();MiscBehavior::AfterEndOfCycleSave();
  REQUIRE(check.obtained);REQUIRE(!check.cycleObtained);
  gSaveContext.save.day=day;REQUIRE(!win(0,&owner));REQUIRE(check.eligible==!on);
 }
 TestLotteryRules();
 puts("PASS lottery native pools/exclusions, selected-night callbacks/messages, permanent JSON/cycle state and actual CHECK rules");
}
