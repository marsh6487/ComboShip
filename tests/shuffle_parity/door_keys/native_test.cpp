#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "Rando/StaticData/StaticData.h"
#include "tests/test_require.h"
#include "Rando/RustyDoorKeys.h"
#include "FleetShipCombo/FleetComboItems.h"
#include "FleetShipCombo/FleetComboIds.h"
#include <spdlog/spdlog.h>
extern "C" int Rando_RustyDoorLocked(PlayState*,Actor*);
#ifdef COMBO_BUILD
#include "rando/CrossEntranceMmCatalogue.h"
extern "C" int Combo_MM_CrossDungeonEntranceFromRegion(int index,int) {return index;}
#endif
#if __has_include("Rando/KeyRings.h")
#include "Rando/KeyRings.h"
#endif
#include <cstdio>
#include <cstring>
#include <memory>

extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState;
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
s32 Ship_Random(s32 first, s32) { return first; }
void Ship_Random_Seed(uint32_t) {}
int FleetShipCombo_GetActiveGame() { return -1; }
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
}
#include "unified_pool.inc"
SaveContext saveContextCopy;
namespace Rando::MiscBehavior {
void BeforeEndOfCycleSave();
void AfterEndOfCycleSave();
}
#include "cycle.inc"
static std::vector<RandoCheckId> excluded;
namespace Rando {
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return excluded; }
#include "starting.inc"
namespace Logic {
std::map<RandoRegionId, RandoRegion> Regions;
uint64_t gCurrentRegionTime=0;
#include "traversal.inc"
int DungeonItemToDungeon(RandoItemId);
}
namespace StaticData {
#include "checks.inc"
}
}

int main() {
    using namespace Rando;
#include "../legacy_ids.inc"
    REQUIRE(std::size(RustyDoorKeys::Catalog)==39);
    REQUIRE(StaticData::Options.at(RO_SHUFFLE_RUSTY_KEYS).defaultValue==0);
    static_assert(FCI_MAX<=FC_COMBO_OBTAINED_FC_SIZE);
    const int native[]={
#define X(id,chain,flags,rg,ri,comboName,ootName,mmName) ri,
    FC_COMBO_ITEM_LIST(X)
#undef X
    };
    struct Alias {int fc;RandoItemId item;const char* token;const char* name;};
    const Alias aliases[]={
#define RUSTY_KEY(game,key,rg,ri,flag,name) {FCI_##game##_RUSTY_KEY_##key,ri,#ri,name},
#include "rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
    };
    REQUIRE(std::size(aliases)==66);
    for(const auto& a:aliases) {
        REQUIRE(native[a.fc]==a.item);
        REQUIRE(std::count(std::begin(native),std::end(native),a.item)==1);
        REQUIRE(!strcmp(StaticData::Items.at(a.item).spoilerName,a.token));
        REQUIRE(!strcmp(StaticData::Items.at(a.item).name,a.name));
        REQUIRE(!strcmp(gFcComboItems[a.fc].mmName,a.name));
        REQUIRE(!strcmp(gFcComboItems[a.fc].ootName,a.name));
    }
    PlayState play{};Actor door{};gPlayState=&play;
    auto& region=Logic::Regions[static_cast<RandoRegionId>(0)];
    for(const auto& [id,row]:StaticData::Checks) if(id!=RC_UNKNOWN)region.checks[id]={[]{return true;},"native checks"};
    for(bool enabled:{false,true})for(bool plentiful:{false,true}) {
        auto save=std::make_unique<RandoSaveInfo>();
        for(const auto& [id,row]:StaticData::Options)save->randoSaveOptions[id]=row.defaultValue;
        save->randoSaveOptions[RO_SHUFFLE_RUSTY_KEYS]=enabled;save->randoSaveOptions[RO_PLENTIFUL_ITEMS]=plentiful;
        save->randoSaveOptions[RO_LOGIC]=RO_LOGIC_GLITCHLESS;
        excluded={RC_BENEATH_THE_GRAVEYARD_DAMPE_CHEST};
        std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;
        Logic::GeneratePools(*save,checks,items);
        REQUIRE(std::find(checks.begin(),checks.end(),excluded[0])==checks.end());
        REQUIRE(save->randoSaveChecks[excluded[0]].skipped);
        REQUIRE(save->randoSaveChecks[excluded[0]].randoItemId==RI_JUNK);
        for(const auto& key:RustyDoorKeys::Catalog) {
            REQUIRE(std::count(items.begin(),items.end(),key.item)==(enabled?1:0));
            REQUIRE(StaticData::Items.at(key.item).randoItemType==RITYPE_MAJOR);
            REQUIRE(StaticData::Items.at(key.item).getItemId==GI_KEY_SMALL);
            REQUIRE(StaticData::GetItemIdFromName(StaticData::Items.at(key.item).spoilerName)==key.item);
        }
    }
    unsigned sources=0;std::set<int> seen;
    for(int scene=0;scene<SCENE_MAX;++scene)for(int index=0;index<64;++index) {
        int item=RustyDoorKeys::SourceItem(scene,index);if(item<0)continue;
        ++sources;seen.insert(item);
        auto* key=RustyDoorKeys::Find(static_cast<RandoItemId>(item));REQUIRE(key);
        auto& wrong=RustyDoorKeys::Catalog[(key-RustyDoorKeys::Catalog+1)%39];
        for(int form=0;form<5;++form) {
            gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
            gSaveContext.save.playerForm=form;RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;
            play.sceneId=scene;door.params=index<<10;
            REQUIRE(Rando_RustyDoorLocked(&play,&door));
            RustyDoorKeys::Give(wrong.item);REQUIRE(Rando_RustyDoorLocked(&play,&door));
            REQUIRE(RustyDoorKeys::Give(static_cast<RandoItemId>(item)));
            REQUIRE(!Rando_RustyDoorLocked(&play,&door));
            auto saved=std::make_unique<SaveContext>(gSaveContext);gSaveContext={};gSaveContext=*saved;
            REQUIRE(!Rando_RustyDoorLocked(&play,&door));
            MiscBehavior::BeforeEndOfCycleSave();MiscBehavior::AfterEndOfCycleSave();
            REQUIRE(!Rando_RustyDoorLocked(&play,&door));
            REQUIRE(RustyDoorKeys::Remove(static_cast<RandoItemId>(item)));
            REQUIRE(Rando_RustyDoorLocked(&play,&door));
            RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=0;REQUIRE(!Rando_RustyDoorLocked(&play,&door));
            RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_VANILLA;
            REQUIRE(!Rando_RustyDoorLocked(&play,&door));
        }
    }
    REQUIRE(seen.size()==39);REQUIRE(sources>39);REQUIRE(RustyDoorKeys::SourceItem(SCENE_CLOCKTOWER,0)<0);
    // A source-locked entry may arrive at another shuffled family's interior.
    // Interior scene exits permit escape with that source key alone, or without keys.
    for(int form=0;form<5;++form) {
        gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
        gSaveContext.save.playerForm=form;RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;
        play.sceneId=SCENE_20SICHITAI;door.params=3<<10;
        REQUIRE(Rando_RustyDoorLocked(&play,&door));
        RustyDoorKeys::Give(RI_RUSTY_KEY_BOMB_SHOP);REQUIRE(Rando_RustyDoorLocked(&play,&door));
        RustyDoorKeys::Remove(RI_RUSTY_KEY_BOMB_SHOP);RustyDoorKeys::Give(RI_RUSTY_KEY_TOURIST_INFORMATION);
        REQUIRE(!Rando_RustyDoorLocked(&play,&door));
        for(int interior:{SCENE_BOMYA,SCENE_MAP_SHOP,SCENE_BANDROOM,SCENE_DANPEI2TEST,SCENE_SYATEKI_MIZU}) {
            play.sceneId=interior;door.params=0;REQUIRE(!Rando_RustyDoorLocked(&play,&door));
        }
    }
    using namespace Rando::Logic;
    struct Route {RandoItemId item;int source,inside;};
    const Route routes[]={
#define RUSTY_ROUTE(key,source,inside) {RI_RUSTY_KEY_##key,source,inside},
#include "Rando/RustyDoorKeyRoutes.inc"
#undef RUSTY_ROUTE
    };
    std::map<int,RandoRegionExit> exits[]={
#define RUSTY_ROUTE(key,source,inside) {EXIT(0x7fff,source,true)},
#include "Rando/RustyDoorKeyRoutes.inc"
#undef RUSTY_ROUTE
    };
    std::map<int,RandoRegionExit> returns[]={
#define RUSTY_ROUTE(key,source,inside) {EXIT(source,inside,true)},
#include "Rando/RustyDoorKeyRoutes.inc"
#undef RUSTY_ROUTE
    };
    int routeIndex=0;
    for(const auto& route:routes) {
        gSaveContext={};RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;
        REQUIRE(RustyDoorKeys::ExitItem(route.source)==route.item);
        REQUIRE(RustyDoorKeys::ExitItem(route.inside)==RI_UNKNOWN);
        REQUIRE(!RustyDoorKeys::CanExit(route.source));REQUIRE(RustyDoorKeys::CanExit(route.inside));
        RustyDoorKeys::Give(route.item==routes[0].item?routes[1].item:routes[0].item);
        REQUIRE(!RustyDoorKeys::CanExit(route.source));
        RustyDoorKeys::Give(route.item);REQUIRE(RustyDoorKeys::CanExit(route.source));REQUIRE(RustyDoorKeys::CanExit(route.inside));
        REQUIRE(returns[routeIndex].at(route.source).condition());
        auto& e=exits[routeIndex++];REQUIRE(e.at(0x7fff).condition());
        RustyDoorKeys::Remove(route.item);REQUIRE(!e.at(0x7fff).condition());
        REQUIRE(returns[routeIndex-1].at(route.source).condition());
    }
    std::map<int,RandoRegionExit> zoraExits={
#include "zora_exit.inc"
    };
    auto zoraRoom=[&]{return zoraExits.begin()->second.condition();};
    for(int form=0;form<5;++form) {
        gSaveContext={};RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;gSaveContext.save.playerForm=form;
        INV_CONTENT(ITEM_MASK_ZORA)=ITEM_NONE;REQUIRE(!zoraRoom());
        RustyDoorKeys::Give(RI_RUSTY_KEY_ZORA_EVAN_ROOM);REQUIRE(!zoraRoom());
        RustyDoorKeys::Give(RI_RUSTY_KEY_ZORA_LULU_ROOM);REQUIRE(zoraRoom()==(form==PLAYER_FORM_ZORA));
        INV_CONTENT(ITEM_MASK_ZORA)=ITEM_MASK_ZORA;REQUIRE(zoraRoom());
        INV_CONTENT(ITEM_MASK_ZORA)=ITEM_NONE;RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=0;
        REQUIRE(zoraRoom()==(form==PLAYER_FORM_ZORA));
    }
    struct NativeConnection {RandoRegionId source;RandoItemId key;std::map<RandoRegionId,std::pair<std::function<bool()>,std::string>> connections;};
    const NativeConnection connections[]={
#include "connections.inc"
    };
    REQUIRE(std::size(connections)>=14);
    for(const auto& edge:connections) {
        gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;
        gSaveContext.save.saveInfo.playerData.isMagicAcquired=true;INV_CONTENT(ITEM_LENS_OF_TRUTH)=ITEM_LENS_OF_TRUTH;
        Regions.clear();auto target=edge.connections.begin()->first;Regions[edge.source].connections=edge.connections;
        Regions[edge.source].canStayOverTime=false;Regions[target].canStayOverTime=false;
        auto reaches=[&](int slice=TIME_NIGHT3_PM_08_00) {
            std::set<RandoRegionId> reachable={edge.source};std::unordered_map<RandoRegionId,RegionTimeState> times;
            times[edge.source]={1ULL<<slice,false};FindReachableRegions(edge.source,reachable,times);return reachable.contains(target);
        };
        REQUIRE(!reaches());RustyDoorKeys::Give(edge.key==RI_RUSTY_KEY_BOMB_SHOP?RI_RUSTY_KEY_POST_OFFICE:RI_RUSTY_KEY_BOMB_SHOP);
        REQUIRE(!reaches());RustyDoorKeys::Give(edge.key);REQUIRE(reaches());
        if(edge.source==RR_INN&&edge.key==RI_RUSTY_KEY_STOCK_POT_INN_STAFF_ROOM)REQUIRE(!reaches(TIME_DAY3_AM_08_00));
        if(edge.key==RI_RUSTY_KEY_BENEATH_GRAVEYARD) {
            INV_CONTENT(ITEM_LENS_OF_TRUTH)=ITEM_NONE;REQUIRE(!reaches());INV_CONTENT(ITEM_LENS_OF_TRUTH)=ITEM_LENS_OF_TRUTH;
            gSaveContext.save.saveInfo.playerData.isMagicAcquired=false;REQUIRE(!reaches());gSaveContext.save.saveInfo.playerData.isMagicAcquired=true;
        }
        RustyDoorKeys::Remove(edge.key);REQUIRE(!reaches());RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=0;REQUIRE(reaches());
    }
    Regions.clear();auto& dampe=Regions[RR_BENEATH_THE_GRAVEYARD_DAMPE];auto& graveyard=Regions[RR_IKANA_GRAVEYARD_LOWER];
    dampe.canStayOverTime=false;graveyard.canStayOverTime=false;
    dampe.exits={
#include "dampe_exit.inc"
    };
    graveyard.oneWayEntrances={ENTRANCE(IKANA_GRAVEYARD,4)};
    gSaveContext={};RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS]=1;
    auto escaped=[&]{std::set<RandoRegionId> reachable={RR_BENEATH_THE_GRAVEYARD_DAMPE};std::unordered_map<RandoRegionId,RegionTimeState> times;
        times[RR_BENEATH_THE_GRAVEYARD_DAMPE]={1ULL<<TIME_NIGHT3_PM_08_00,false};FindReachableRegions(RR_BENEATH_THE_GRAVEYARD_DAMPE,reachable,times);return reachable.contains(RR_IKANA_GRAVEYARD_LOWER);};
    REQUIRE(escaped());RustyDoorKeys::Give(RI_RUSTY_KEY_BENEATH_GRAVEYARD);REQUIRE(escaped());
    std::printf("PASS native MM39 rusty keys: %u source cases x5forms, %zu paired routes, %zu actual native traversal edges, Zora/lens/time/Dampe,66 aliases, saves/cycles/full pools/exclusions\n",sources,std::size(routes),std::size(connections));
}
