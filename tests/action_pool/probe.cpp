#include "rando/CrossWorldRando.h"
#include <fstream>
#include <map>
#include <stdexcept>
#define COMBO_BUILD
#define REQUIRE(condition) do { if (!(condition)) throw std::runtime_error("FAIL: " #condition); } while (0)
namespace Oot {
#define RANDO_ENUM_BEGIN(name) typedef enum name {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(name) } name;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerOptions.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
struct Option { int value; int Get() const { return value; } bool Is(int v) const {return value==v;} operator bool() const {return value!=0;} };
struct Context {std::map<int,int> options;std::set<RandomizerGet> possibleIceTrapModels;Option GetOption(int id){return {options[id]};}} context;
auto* ctx=&context;
std::vector<RandomizerGet> itemPool,lesserPool,plentifulPool,junkPool;
#include "oot_pool_map.inc"
#include "oot_add.inc"
std::vector<RandomizerGet> Generate(int climb,int chest,int abundance) {
    itemPool.clear();plentifulPool.clear();context.possibleIceTrapModels.clear();
    context.options[RSK_SHUFFLE_CLIMB]=climb;context.options[RSK_SHUFFLE_CRAWL]=0;
    context.options[RSK_SHUFFLE_OPEN_CHEST]=chest;context.options[RSK_ITEM_POOL]=abundance;
#include "oot_actions.inc"
    return itemPool;
}
}
namespace Mm {
#include "mm/2s2h/Rando/Types.h"
#include "mm_rusty_visual.inc"
std::map<std::string,int> cvars;
int CVarGetInteger(const char* name,int fallback){auto it=cvars.find(name);return it==cvars.end()?fallback:it->second;}
void CVarSetInteger(const char* name,int value){cvars[name]=value;}
namespace Ship {struct Context {static Context* GetRawInstance(){static Context c;return &c;} Context* GetWindow(){return this;}Context* GetGui(){return this;}void SaveConsoleVariablesNextFrame(){}};}
namespace Rando::StaticData {
struct RandoStaticOption {RandoOptionId id;const char* name;const char* cvar;int defaultValue;};
#include "mm_options.inc"
struct Item {RandoItemId randoItemId;const char* spoilerName;const char* name;RandoItemType randoItemType;};
std::map<RandoItemId,Item> Items={
#include "mm_item_rows.inc"
};
#include "mm_names.inc"
}
#include "mm_menu.inc"
#include "mm_active.inc"
bool combined=true;
bool Combo_IsCombinedGame(){return combined;}
struct RandoSaveInfo {std::map<int,uint32_t> randoSaveOptions;};
std::vector<RandoItemId> Generate(bool gear,bool useButton,bool inCombo){
    combined=inCombo;cvars.clear();
    if(useButton)NeiRando_SetAll(true);
    else CVarSetInteger(Rando::StaticData::Options[RO_SHUFFLE_OOT_GEAR].cvar,gear);
    RandoSaveInfo saveInfo;
#include "mm_settings.inc"
    std::vector<RandoItemId> itemPool;
#include "mm_has_oot.inc"
#include "mm_gear.inc"
    return itemPool;
}
nlohmann::json Dump(const std::vector<RandoItemId>& itemPool){
    using namespace Rando::StaticData;
    nlohmann::json pool=nlohmann::json::array();
#include "mm_category.inc"
#include "mm_pool_emit.inc"
    return pool;
}
}
using J=nlohmann::json;
static std::string ootReach,mmReach;
void Reset(){} void Owned(const char*){}void Place(const char*,const char*){}
const char* OotReach(){return ootReach.c_str();}const char* MmReach(){return mmReach.c_str();}uint8_t Portal(){return 1;}
J Dump(J pool,const char* prefix){J d={{"checks",J::array()},{"pool",pool},{"items",J::array()},{"fixed",J::array()},{"junkPool",J::array({"Blue Rupee"})}};for(int i=0;i<20;i++)d["checks"].push_back({{"name",std::string(prefix)+std::to_string(i)}});while(d["pool"].size()<20)d["pool"].push_back({{"name","Blue Rupee"},{"category","junk"},{"advancement",false}});for(const auto& [id,item]:Mm::Rando::StaticData::Items)d["items"].push_back({{"token",item.spoilerName},{"name",Mm::Rando::StaticData::GetItemDisplayName(id)}});return d;}
J OotDump(const std::vector<Oot::RandomizerGet>& items){J pool=J::array();for(auto id:items){std::string name=id==Oot::RG_OPEN_CHEST?"Open Chests":id==Oot::RG_CLIMB?"Climb":"Crawl";pool.push_back({{"name",name},{"category","major"},{"advancement",true}});}return Dump(pool,"OOT test check ");}
int Count(const J& spoiler,const std::string& name){int n=0;for(const char* game:{"oot","mm"})for(const auto& v:spoiler[game])n+=v==name;return n;}
int main(int argc,char** argv)try {
    REQUIRE(argc>=2);std::string output=argv[1];bool observe=argc==3;
    REQUIRE(Mm::FleetCombo_UnifiedPoolActive()==0);
    REQUIRE(Mm::Rando::StaticData::GetItemDisplayName(Mm::RI_OOT_ABILITY_CHESTS)=="Open Chests");
    // Real default snapshot and Add-all menu function, never touching the OoT option context.
    REQUIRE(Mm::Generate(false,false,true).empty());
    auto button=Mm::Generate(false,true,true);
    REQUIRE(Mm::CVarGetInteger("gRando.Options.RO_SHUFFLE_OOT_GEAR",0)==1);
    REQUIRE(Oot::context.options[Oot::RSK_SHUFFLE_CLIMB]==0);
    REQUIRE(Oot::context.options[Oot::RSK_SHUFFLE_OPEN_CHEST]==0);
    auto mmPool=Mm::Dump(button);
    REQUIRE(std::count(button.begin(),button.end(),Mm::RI_OOT_PROGRESSIVE_MASTER_SWORD)==2);
    REQUIRE(std::count(button.begin(),button.end(),Mm::RI_OOT_PROGRESSIVE_BGS)==2);
    REQUIRE(std::count(button.begin(),button.end(),Mm::RI_OOT_PROGRESSIVE_STRENGTH)==3);
    REQUIRE(std::count(button.begin(),button.end(),Mm::RI_OOT_PROGRESSIVE_STICK_CAPACITY)==2);
    REQUIRE(std::count(button.begin(),button.end(),Mm::RI_OOT_PROGRESSIVE_NUT_CAPACITY)==2);
    REQUIRE(std::count(button.begin(),button.end(),Mm::RI_OOT_ROCS_FEATHER)==1);
    auto solo=Mm::Generate(true,false,false);REQUIRE(std::count(solo.begin(),solo.end(),Mm::RI_OOT_ABILITY_CHESTS)==0);
    int poolCases=0;
    for(int abundance:{Oot::RO_ITEM_POOL_PLENTIFUL,Oot::RO_ITEM_POOL_BALANCED,Oot::RO_ITEM_POOL_SCARCE,Oot::RO_ITEM_POOL_MINIMAL})for(int climb:{0,1})for(int chest:{Oot::RO_OPEN_CHEST_OFF,Oot::RO_OPEN_CHEST_ON,Oot::RO_OPEN_CHEST_PROGRESSIVE}){
        auto pool=Oot::Generate(climb,chest,abundance);
        REQUIRE(std::count(pool.begin(),pool.end(),Oot::RG_CLIMB)==climb);
        REQUIRE(std::count(pool.begin(),pool.end(),Oot::RG_OPEN_CHEST)==(chest==Oot::RO_OPEN_CHEST_OFF?0:chest==Oot::RO_OPEN_CHEST_ON?1:2));
        REQUIRE(std::count(Oot::plentifulPool.begin(),Oot::plentifulPool.end(),Oot::RG_CLIMB)==(abundance==Oot::RO_ITEM_POOL_PLENTIFUL?climb:0));
        REQUIRE(std::count(Oot::plentifulPool.begin(),Oot::plentifulPool.end(),Oot::RG_OPEN_CHEST)==(abundance==Oot::RO_ITEM_POOL_PLENTIFUL&&chest!=Oot::RO_OPEN_CHEST_OFF?1:0));++poolCases;
    }
    ComboRando::OracleFns oo{Reset,Owned,OotReach,Place,Portal},mo{Reset,Owned,MmReach,Place};
    J results;int failures=0;
    for(int route:{0,1,2})for(int chest:{0,1,2})for(int climb:{0,1})for(bool sharing:{false,true}){
        auto od=OotDump(Oot::Generate(climb,chest,Oot::RO_ITEM_POOL_BALANCED));
        auto native=Mm::Generate(route==1,route==2,true);auto md=Dump(Mm::Dump(native),"MM test check ");
        J orc=J::array(),mrc=J::array();for(auto& c:od["checks"])orc.push_back(c["name"]);for(auto& c:md["checks"])mrc.push_back(c["name"]);orc.push_back("Ganon");mrc.push_back("Moon Majora Pot 01");ootReach=orc.dump();mmReach=mrc.dump();
        const std::string pairs=sharing?J::array({{{"oot","Open Chests"},{"mm","RI_OOT_ABILITY_CHESTS"}}}).dump():"";
        int chestPlacements=0,climbPlacements=0,foreign=0;
        for(unsigned seed=1;seed<=20;++seed){
            auto fill=ComboRando::CrossWorldCombinedFill(od.dump(),md.dump(),seed,oo,mo,nullptr,"",ComboRando::OotAccess::ALL_REACHABLE,{},ComboRando::GAME_OOT,pairs);
            REQUIRE(fill.success);auto spoiler=J::parse(fill.spoilerJson);int got=Count(spoiler,"Open Chests"),want=chest==0?0:chest==1?1:2;
            chestPlacements+=got;climbPlacements+=Count(spoiler,"Climb");
            for(auto& f:spoiler["foreign"])foreign+=f["itemName"]=="Open Chests";
            if(got!=want)++failures;
            REQUIRE(Count(spoiler,"Climb")==climb);
            if(route==2&&chest==0&&climb==0&&!sharing&&seed==1)std::ofstream(output+"/sample-test-fill.json")<<spoiler.dump(2);
        }
        results.push_back({{"route",route==0?"default":route==1?"manual_gear":"add_all_nei"},{"chest_setting",chest},{"climb_setting",climb},{"sharing",sharing},{"seeds",20},{"chest_placements",chestPlacements},{"climb_placements",climbPlacements},{"foreign_chest_placements",foreign}});
    }
    J report={{"native_pool_cases",poolCases},{"seeded_fills",results.size()*20},{"chest_count_contract_failures",failures},{"cases",results},{"scope","Production menu/options/pool branches/name mapping/pool emission/AddItemToPool and complete combined fill; synthetic checks/all-reachable oracle boundaries. Not a full native seed run."}};
    std::ofstream(output+"/results.json")<<report.dump(2);std::cerr<<"Pool cases: "<<poolCases<<"; seeded fills: "<<results.size()*20<<"; chest count contract failures: "<<failures<<"\n";
    if(!observe)REQUIRE(failures==0);
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
