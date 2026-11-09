#define main CurrentRuntimeMatrixMain
#include "oot_runtime_test.cpp"
#undef main
#include "combo/NeiGracePolicy.h"
#include "combo/NeiSeasonsPolicy.h"
#include "combo/rando/MagicMirrorShuffle.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <sstream>
#include <unordered_map>
using json=nlohmann::json;
#include "macros.h"
#define COMBO_EXPORT
static std::map<std::string,std::string> stringCvars;
extern "C" {
const char* CVarGetString(const char* key,const char* fallback) {auto it=stringCvars.find(key);return it==stringCvars.end()?fallback:it->second.c_str();}
void CVarSetString(const char* key,const char* value){stringCvars[key]=value;}
void CVarClear(const char* key){cvars.erase(key);stringCvars.erase(key);}
}
// Option metadata/context/container allocation and JSON storage are the engine
// service boundary. The actual production parser, save callbacks, array walks,
// snapshot functions, pool branch and key-oracle branch are extracted unchanged.
namespace Rando {
struct OptionValue {
    uint8_t mVal=0;
    uint8_t Get();void Set(uint8_t);explicit operator bool() const;
    bool Is(uint32_t v)const{return mVal==v;}
};
struct Location {void SetExcludedOption(uint8_t){}};
struct Context {
    std::array<OptionValue,RSK_MAX> values{};std::set<RandomizerGet> possibleIceTrapModels;
    std::string seed;uint32_t finalSeed=0;
    static std::shared_ptr<Context> GetInstance(){static auto p=std::make_shared<Context>();return p;}
    OptionValue& GetOption(RandomizerSettingKey k){return values[k];}
    void SetSeedString(const std::string& v){seed=v;}void SetSeed(uint32_t v){finalSeed=v;}
    Location* GetItemLocation(RandomizerCheck){static Location l;return &l;}
};
struct Option {
    RandomizerSettingKey key=RSK_NONE;std::string cvarName,name;std::vector<std::string> options;
    std::unordered_map<std::string,uint8_t> optionsTextToVar;uint8_t defaultOption=0;
    const std::string& GetCVarName()const;uint8_t GetOptionIndex()const;
    RandomizerSettingKey GetKey()const{return key;}
    const std::string& GetName()const{return name;}
    size_t GetOptionCount()const{return options.size();}
    const std::string& GetOptionText(size_t i)const{return options.at(i);}
    uint8_t GetValueFromText(const std::string text);
};
struct Trick {void SetContextIndex(uint8_t){}};
struct Settings {
    static std::shared_ptr<Settings> mInstance;std::shared_ptr<Context> mContext=Context::GetInstance();
    std::array<Option,RSK_MAX> mOptions{};std::unordered_map<std::string,RandomizerTrick> mTrickNameToEnum;
    static std::shared_ptr<Settings> GetInstance();const std::array<Option,RSK_MAX>& GetAllOptions()const;
    void ParseJson(const json&);Trick& GetTrickSetting(RandomizerTrick){static Trick t;return t;}
};
std::shared_ptr<Settings> Settings::mInstance;
namespace StaticData {
std::unordered_map<std::string,RandomizerSettingKey> optionNameToEnum;
std::unordered_map<std::string,RandomizerCheck> locationNameToEnum;
}
}
static auto ctx=Rando::Context::GetInstance();
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#include "legacy_options.inc"
static json jsonData;
#include "legacy_snapshot.inc"
#include "legacy_spoiler.inc"
struct SaveManager {
    using SaveArrayFunc=std::function<void(size_t)>;using LoadArrayFunc=SaveArrayFunc;
    static SaveManager* Instance;json root;json* currentJsonContext=&root;json::iterator currentJsonArrayContext;
    void SaveArray(const std::string&,size_t,SaveArrayFunc);void LoadArray(const std::string&,size_t,LoadArrayFunc);
    template<class T>void SaveData(const std::string& name,const T& v){if(name.empty())currentJsonContext->push_back(v);else (*currentJsonContext)[name]=v;}
    template<class T>void LoadData(const std::string& name,T& v){
        if(name.empty()){if(currentJsonArrayContext!=currentJsonContext->end())currentJsonArrayContext->get_to(v);else v=T();}
        else if(currentJsonContext->contains(name))currentJsonContext->at(name).get_to(v);else v=T();
    }
};
SaveManager* SaveManager::Instance;
#include "legacy_save_arrays.inc"
static json SaveNative(){SaveManager m;SaveManager::Instance=&m;auto randoContext=ctx;auto& saveContext=gSaveContext;
#include "legacy_save_callbacks.inc"
    return m.root;
}
static void LoadNative(const json& j){SaveManager m;m.root=j;SaveManager::Instance=&m;auto randoContext=ctx;
#include "legacy_load_callbacks.inc"
}
static std::vector<RandomizerGet> itemPool,plentifulPool;
static std::map<RandomizerGet,std::vector<RandomizerGet>*> poolForItem;
#include "legacy_add_pool.inc"
static std::set<RandomizerGet> Pool(){itemPool.clear();plentifulPool.clear();
#include "legacy_pool.inc"
    return {itemPool.begin(),itemPool.end()};
}
namespace Rando {
struct LegacyKeyOracle {
    std::shared_ptr<Context> ctx=Context::GetInstance();
    std::set<RandomizerGet> baseInventory; // Non-key inventory service boundary for native edge predicates.
    bool CanUse(RandomizerGet item){return baseInventory.contains(item);}
    bool CheckRandoInf(RandomizerInf f){return Flags_GetRandomizerInf(f)!=0;}
    bool HasItem(RandomizerGet itemName){
#include "legacy_oracle.inc"
        return itemName==RG_SKELETON_KEY ? CheckRandoInf(RAND_INF_HAS_SKELETON_KEY) : baseInventory.contains(itemName);
    }
};
}
// Actual Entrance class and methods execute the native shuffle operation with
// Region node storage/allocation as a service boundary.
using ConditionFn=bool(*)();
class Region;
#include "legacy_entrance_decl.inc"
#include <list>
class Region {
public:
    std::list<Rando::Entrance*> entrances;std::list<Rando::Entrance> exits;std::string regionName="native graph boundary";
    void AddExit(RandomizerRegion,RandomizerRegion to,ConditionFn fn,std::string text){exits.emplace_back(to,fn,text);}
    Rando::Entrance* GetExit(RandomizerRegion to){for(auto& e:exits)if(e.GetConnectedRegionKey()==to)return &e;return nullptr;}
};
static std::map<RandomizerRegion,Region> nativeRegions;
static Region* RegionTable(RandomizerRegion id){return &nativeRegions[id];}
#ifdef COMBO_BUILD
#include "combo/ComboGameMode.h"
extern "C" {int gComboGameMode=COMBO_GAME_BOTH;int Combo_SOH_CrossDungeonRegion(int,int,int fallback){return fallback;}}
#endif
#define SPDLOG_DEBUG(...) ((void)0)
#include "legacy_entrance_methods.inc"
static auto logic=std::make_shared<Rando::LegacyKeyOracle>();
static std::string CleanConditionString(const char* text){return text;}
#define ENTRANCE(check, condition, ...) Rando::Entrance(RandomizerRegion::check, [] { return condition; }, CleanConditionString(#condition), ##__VA_ARGS__)
extern "C" {
#include "overlays/actors/ovl_Bg_Spot15_Saku/z_bg_spot15_saku.h"
#include "overlays/actors/ovl_En_Heishi2/z_en_heishi2.h"
void BgSpot15Saku_Init(Actor*,PlayState*);void BgSpot15Saku_Update(Actor*,PlayState*);
void func_80A5344C(EnHeishi2*,PlayState*);void func_80A531E4(EnHeishi2*,PlayState*);
void func_80A53538(EnHeishi2*,PlayState*);
extern int castleSoundCount,castleMessageClosed,castleMessageContinued;
}
static void CheckCastleNative() {
    PlayState play{};play.sceneNum=SCENE_HYRULE_CASTLE;
    for(int version:{0,1})for(int enabled:{0,1})for(bool staleOpen:{false,true})for(int owner:{0,1,2,3})for(bool randomized:{false,true}){
        ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(version);ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Set(enabled);
        gSaveContext={};gSaveContext.ship.quest.id=randomized?QUEST_RANDOMIZER:QUEST_NORMAL;
        if(staleOpen)gSaveContext.infTable[INFTABLE_71>>4]|=1u<<(INFTABLE_71&15);
        if(owner==1)Own(RAND_INF_RUSTY_KEY_HYRULE_CASTLE);
        if(owner==2)Own(RAND_INF_RUSTY_KEY_RANCH_HOUSE_ROOM);
        if(owner==3)Own(RAND_INF_HAS_SKELETON_KEY);
        auto originalInf=std::vector(std::begin(gSaveContext.infTable),std::end(gSaveContext.infTable));
        bool keyMode=version==1&&enabled&&randomized;bool blocked=keyMode&&owner!=1&&owner!=3;
        REQUIRE(bool(Randomizer_RustyCastleGateEnabled(&play))==keyMode);
        REQUIRE(bool(Randomizer_RustyCastleGateLocked(&play))==blocked);
        BgSpot15Saku gate{};gate.dyna.actor.world.pos.z=2800.0f;castleSoundCount=0;
        BgSpot15Saku_Init(&gate.dyna.actor,&play);REQUIRE(gate.dyna.bgId==7);
        REQUIRE(gate.dyna.actor.world.pos.z==(staleOpen&&!blocked?2659.0f:2800.0f));
        // Simulate a native guard opening already requested/paid before the snapshot.
        gate.unk_168=1;
        for(int frame=0;frame<120;++frame)BgSpot15Saku_Update(&gate.dyna.actor,&play);
        REQUIRE((gate.dyna.actor.world.pos.z<2660.0f)==!blocked);
        REQUIRE(std::equal(originalInf.begin(),originalInf.end(),std::begin(gSaveContext.infTable)));
        if(blocked){
            Own(RAND_INF_RUSTY_KEY_HYRULE_CASTLE);
            for(int frame=0;frame<120;++frame)BgSpot15Saku_Update(&gate.dyna.actor,&play);
            REQUIRE(gate.dyna.actor.world.pos.z<2660.0f);REQUIRE(castleSoundCount>0);
            REQUIRE(std::equal(originalInf.begin(),originalInf.end(),std::begin(gSaveContext.infTable)));
        }
        // A key opens the native gate without a bribe request or Hylian/wallet ownership.
        if(keyMode&&!blocked){
            BgSpot15Saku fresh{};fresh.dyna.actor.world.pos.z=2800.0f;
            gSaveContext.infTable[INFTABLE_71>>4]&=~(1u<<(INFTABLE_71&15));BgSpot15Saku_Init(&fresh.dyna.actor,&play);
            REQUIRE(fresh.unk_168==0);
            for(int frame=0;frame<120;++frame)BgSpot15Saku_Update(&fresh.dyna.actor,&play);
            REQUIRE(fresh.dyna.actor.world.pos.z<2660.0f);
        }
    }
    // Exact native guard payment function: key mode returns before native currency/stat updates.
    for(int version:{0,1})for(int enabled:{0,1})for(int scene:{SCENE_HYRULE_CASTLE,SCENE_KAKARIKO_VILLAGE})for(int role:{2,5})for(bool randomized:{false,true})for(bool owned:{false,true}){
        ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(version);ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Set(enabled);
        play.sceneNum=scene;play.msgCtx.choiceIndex=0;gSaveContext={};gSaveContext.ship.quest.id=randomized?QUEST_RANDOMIZER:QUEST_NORMAL;gSaveContext.rupees=50;
        if(owned)Own(RAND_INF_RUSTY_KEY_HYRULE_CASTLE);
        EnHeishi2 guard{};guard.type=role;castleMessageClosed=castleMessageContinued=0;
        func_80A5344C(&guard,&play);
        bool keyMode=version==1&&enabled&&randomized&&scene==SCENE_HYRULE_CASTLE&&role==2;
        REQUIRE(gSaveContext.rupees==(keyMode?50:40));
        REQUIRE(gSaveContext.ship.stats.count[COUNT_RUPEES_SPENT]==(keyMode?0:10));
        REQUIRE(guard.actionFunc==(keyMode?func_80A531E4:func_80A53538));
        REQUIRE(castleMessageClosed==(keyMode?1:0));
        REQUIRE(gSaveContext.infTable[INFTABLE_71>>4]==0);
    }
    play.sceneNum=SCENE_HYRULE_CASTLE;
    ctx->GetOption(RSK_NO_LOGIC).Set(0);
#include "legacy_castle_edges.inc"
    for(int version:{0,1})for(int enabled:{0,1}){
        ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(version);ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Set(enabled);
        gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;logic->baseInventory.clear();
        REQUIRE(!gateEdge.GetConditionsMet());Own(RAND_INF_RUSTY_KEY_RANCH_HOUSE_ROOM);REQUIRE(!gateEdge.GetConditionsMet());
        Own(RAND_INF_RUSTY_KEY_HYRULE_CASTLE);REQUIRE(gateEdge.GetConditionsMet()==(version==1&&enabled));
        gSaveContext.ship.randomizerInf[RAND_INF_RUSTY_KEY_HYRULE_CASTLE/16]=0;
        logic->baseInventory={RG_CHILD_WALLET,RG_SPEAK_HYLIAN};REQUIRE(gateEdge.GetConditionsMet()==!(version==1&&enabled));
        Own(RAND_INF_HAS_SKELETON_KEY);REQUIRE(gateEdge.GetConditionsMet());
        logic->baseInventory={RG_CLIMB};REQUIRE(vineEdge.GetConditionsMet());REQUIRE(backEdge.GetConditionsMet());
    }
    std::puts("PASS actual native Castle gate Init/Update/animation, saved-open/paid flags, key/wrong/Skeleton, precise guard no-charge, matching ground/vines/return predicates");
}
static void CheckShuffledEscape() {
    for(int version:{0,1})for(bool decoupled:{false,true}){
        nativeRegions.clear();ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Set(1);ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(version);ctx->GetOption(RSK_NO_LOGIC).Set(0);
        gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
#include "legacy_entrance_edges.inc"
        guardEntry.BindTwoWay(&guardReturn);richardEntry.BindTwoWay(&richardReturn);
        guardReturn.AssumeReachable();auto target=richardEntry.AssumeReachable();
        if(decoupled)guardEntry.SetDecoupled();
        Rando::ChangeConnections(&guardEntry,target);
        REQUIRE(guardEntry.GetConnectedRegionKey()==RR_MARKET_DOG_LADY_HOUSE);
        REQUIRE(!guardEntry.GetConditionsMet());
        Own(RAND_INF_RICHARDS_HOUSE_KEY_OBTAINED);REQUIRE(!guardEntry.GetConditionsMet());
        gSaveContext.ship.randomizerInf[RAND_INF_RICHARDS_HOUSE_KEY_OBTAINED/16]=0;
        Own(RAND_INF_GUARD_HOUSE_KEY_OBTAINED);REQUIRE(guardEntry.GetConditionsMet());
        REQUIRE(richardReturn.GetConditionsMet());
        REQUIRE(richardReturn.GetConnectedRegionKey()==(decoupled?RR_MARKET_BACK_ALLEY:RR_MARKET_ENTRANCE));
        PlayState play{};Actor door{};door.params=3<<7;play.sceneNum=SCENE_MARKET_ENTRANCE_DAY;
        REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));play.sceneNum=SCENE_DOG_LADY_HOUSE;
        REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    }
}
#undef ENTRANCE
static std::string VersionHashSuffix(){std::string settingsStr;
#include "legacy_hash.inc"
    return settingsStr;
}
static void CopyConfiguredSettings(){auto& mContext=Rando::Settings::GetInstance()->mContext;auto& mOptions=Rando::Settings::GetInstance()->mOptions;
#include "legacy_copy_options.inc"
}
static uint8_t ReadOption(RandomizerSettingKey k){return ctx->GetOption(k).Get();}
static void Setup(){
    settingProvider=ReadOption;auto s=Rando::Settings::GetInstance();
    auto row=[&](RandomizerSettingKey key,const char* cvar,const char* name,std::vector<std::string> opts){
        auto& o=s->mOptions[key];o.key=key;o.cvarName=cvar;o.name=name;o.options=opts;
        for(size_t i=0;i<opts.size();++i)o.optionsTextToVar[opts[i]]=i;
        Rando::StaticData::optionNameToEnum[name]=key;
    };
    row(RSK_LOCK_OVERWORLD_DOORS,"gRandoSettings.LockOverworldDoors","Lock Overworld Doors",{"Off","On"});
    row(RSK_RUSTY_DOOR_KEY_VERSION,Rando::RustyDoorKeys::CatalogVersionCVar,"Rusty Door Key Catalog",{"Legacy24","Rusty27"});
}
int main(){
    using namespace Rando;Setup();LegacyKeyOracle oracle;
    auto oldSnapshot=json{{"gRandoSettings.LockOverworldDoors",1}};
    cvars[RustyDoorKeys::CatalogVersionCVar]=1;ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(1);
    SOH_RestoreRandoSettings(oldSnapshot.dump().c_str());CopyConfiguredSettings();
    REQUIRE(ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Get()==1);
    REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==0);
    REQUIRE(!json::parse(SOH_DumpRandoSettings()).contains(RustyDoorKeys::CatalogVersionCVar));
    for(int density:{RO_ITEM_POOL_PLENTIFUL,RO_ITEM_POOL_BALANCED,RO_ITEM_POOL_SCARCE,RO_ITEM_POOL_MINIMAL}){
        ctx->GetOption(RSK_ITEM_POOL).Set(density);REQUIRE(Pool().size()==24);
        REQUIRE(!Pool().contains(RG_RUSTY_KEY_RANCH_HOUSE_ROOM));
    }
    REQUIRE(VersionHashSuffix().empty());
    PlayState play{};Actor door{};std::set<int> legacyItems;
    for(int scene=0;scene<SCENE_ID_MAX;++scene)for(int index=0;index<64;++index){
        auto item=RustyDoorKeys::LegacySourceItem(scene,index);if(item==RG_NONE)continue;
        legacyItems.insert(item);gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;play.sceneNum=scene;door.params=index<<10;
        REQUIRE(Randomizer_RustyDoorLocked(&play,&door));REQUIRE(!oracle.HasItem(item));
        Own(RAND_INF_RUSTY_KEY_RANCH_HOUSE_ROOM);REQUIRE(Randomizer_RustyDoorLocked(&play,&door));REQUIRE(!oracle.HasItem(item));
        Own(RustyDoorKeys::Find(item)->flag);REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));REQUIRE(oracle.HasItem(item));
    }
    REQUIRE(legacyItems.size()==24);
    gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
    play.sceneNum=SCENE_HYRULE_CASTLE;door.params=0;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));REQUIRE(oracle.HasItem(RG_RUSTY_KEY_HYRULE_CASTLE));
    play.sceneNum=SCENE_LON_LON_BUILDINGS;door.params=2<<10;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));REQUIRE(oracle.HasItem(RG_RUSTY_KEY_RANCH_HOUSE_ROOM));
    play.sceneNum=SCENE_DOG_LADY_HOUSE;door.params=0;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    REQUIRE(RustyDoorKeys::CanUseInterior(*ctx,oracle,RG_RICHARDS_HOUSE_KEY));
    play.sceneNum=SCENE_KAKARIKO_VILLAGE;door.params=8<<10;REQUIRE(Randomizer_RustyDoorLocked(&play,&door));REQUIRE(!oracle.HasItem(RG_RUSTY_KEY_ADULT_POTION_SHOP_BACK));
    Own(RAND_INF_RUSTY_KEY_ADULT_POTION_SHOP_BACK);REQUIRE(Randomizer_RustyDoorLocked(&play,&door));REQUIRE(!oracle.HasItem(RG_RUSTY_KEY_ADULT_POTION_SHOP_BACK));
    Own(RAND_INF_KAK_POTION_SHOP_KEY_OBTAINED);Own(RAND_INF_GUARD_HOUSE_KEY_OBTAINED);
    REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));REQUIRE(oracle.HasItem(RG_RUSTY_KEY_ADULT_POTION_SHOP_BACK));
    auto oldSave=SaveNative();oldSave["randoSettings"].erase(oldSave["randoSettings"].begin()+RSK_RUSTY_DOOR_KEY_VERSION,oldSave["randoSettings"].end());
    oldSave["randomizerInf"].erase(oldSave["randomizerInf"].begin()+((RAND_INF_RUSTY_KEY_HYRULE_CASTLE+15)/16),oldSave["randomizerInf"].end());
    // Padding in the legacy final word was unowned, even if a new build assigned bits there.
    auto& words=oldSave["randomizerInf"];if(!words.empty()&&(RAND_INF_RUSTY_KEY_HYRULE_CASTLE%16))words.back()=words.back().get<uint16_t>()&((1u<<(RAND_INF_RUSTY_KEY_HYRULE_CASTLE%16))-1);
    for(auto& v:ctx->values)v.Set(99);for(auto& word:gSaveContext.ship.randomizerInf)word=65535;
    LoadNative(oldSave);REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==0);REQUIRE(ctx->GetOption(RSK_LOCK_OVERWORLD_DOORS).Get()==1);
    REQUIRE(Flags_GetRandomizerInf(RAND_INF_GUARD_HOUSE_KEY_OBTAINED));REQUIRE(Flags_GetRandomizerInf(RAND_INF_KAK_POTION_SHOP_KEY_OBTAINED));
    REQUIRE(!Flags_GetRandomizerInf(RAND_INF_RUSTY_KEY_HYRULE_CASTLE));
    REQUIRE(Pool().size()==24);REQUIRE(oracle.HasItem(RG_RUSTY_KEY_RANCH_HOUSE_ROOM));
    play.sceneNum=SCENE_HYRULE_CASTLE;door.params=0;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    // Actual native spoiler parsing resets a dirty newer Context before old settings are applied.
    auto oldSpoiler=json{{"seed","old24"},{"finalSeed",12345},{"settings",{{"Lock Overworld Doors","On"}}}};
    ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(1);Rando::Settings::GetInstance()->ParseJson(oldSpoiler);
    REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==0);REQUIRE(Pool().size()==24);REQUIRE(VersionHashSuffix().empty());
    // Actual new coordinator request stamps before generic settings copy; old replay above never did.
    cvars["gRandoSettings.LockOverworldDoors"]=1;RustyDoorKeys::PrepareNewGeneration();CopyConfiguredSettings();
    REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==1);REQUIRE(Pool().size()==27);REQUIRE(!VersionHashSuffix().empty());
    auto currentSnapshot=json::parse(SOH_DumpRandoSettings());REQUIRE(currentSnapshot.at(RustyDoorKeys::CatalogVersionCVar)==1);
    gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;play.sceneNum=SCENE_HYRULE_CASTLE;door.params=0;
    REQUIRE(Randomizer_RustyCastleGateLocked(&play));REQUIRE(!oracle.HasItem(RG_RUSTY_KEY_HYRULE_CASTLE));
    Own(RAND_INF_RUSTY_KEY_RANCH_HOUSE_ROOM);REQUIRE(Randomizer_RustyCastleGateLocked(&play));
    Own(RAND_INF_RUSTY_KEY_HYRULE_CASTLE);REQUIRE(!Randomizer_RustyCastleGateLocked(&play));REQUIRE(oracle.HasItem(RG_RUSTY_KEY_HYRULE_CASTLE));
    auto currentSave=SaveNative();ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(0);LoadNative(currentSave);REQUIRE(ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()==1);REQUIRE(Pool().size()==27);
    jsonData=json::object();WriteSettings();jsonData["seed"]="current27";jsonData["finalSeed"]=34567;
    REQUIRE(jsonData["settings"]["Rusty Door Key Catalog"]=="Rusty27");ctx->GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(0);Rando::Settings::GetInstance()->ParseJson(jsonData);REQUIRE(Pool().size()==27);
    cvars.clear();SOH_RestoreRandoSettings(currentSnapshot.dump().c_str());CopyConfiguredSettings();REQUIRE(Pool().size()==27);
    SOH_RestoreRandoSettings(oldSnapshot.dump().c_str());CopyConfiguredSettings();REQUIRE(Pool().size()==24);REQUIRE(VersionHashSuffix().empty());
    cvars["gRandoSettings.LockOverworldDoors"]=0;RustyDoorKeys::PrepareNewGeneration();CopyConfiguredSettings();REQUIRE(Pool().empty());REQUIRE(VersionHashSuffix().empty());
    CheckShuffledEscape();
    CheckCastleNative();
    std::puts("PASS actual legacy24/new27: native pool branch,24 exterior runtime/oracle keys, free interiors/rear front-key, native array save+flags, native spoiler dirty reset, fleet snapshots, explicit stamp/hash and actual coupled+decoupled shuffle return");
}
