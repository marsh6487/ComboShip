#include "soh/Enhancements/randomizer/LockOverworldDoors.cpp"
#include "tests/test_require.h"
#include "soh/FleetShipCombo/FleetComboItems.h"
#include "soh/FleetShipCombo/FleetComboIds.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <memory>
#include <set>
#include <map>
#include <string>
extern "C" {
SaveContext gSaveContext{};
u8 options[RSK_MAX]{};
static std::map<std::string,int> cvars;
int32_t CVarGetInteger(const char* key,int32_t fallback) {auto it=cvars.find(key);return it==cvars.end()?fallback:it->second;}
void CVarSetInteger(const char* key,int32_t value) {cvars[key]=value;}
u8 (*settingProvider)(RandomizerSettingKey)=nullptr;
u8 Randomizer_GetSettingValue(RandomizerSettingKey k) { return settingProvider ? settingProvider(k) : options[k]; }
s32 Flags_GetRandomizerInf(RandomizerInf f) { return (gSaveContext.ship.randomizerInf[f/16]>>(f%16))&1; }
}
static void Own(RandomizerInf f) { gSaveContext.ship.randomizerInf[f/16]|=1u<<(f%16); }
int main() {
    using namespace Rando;
    REQUIRE(std::size(RustyDoorKeys::Catalog)==27);
    options[RSK_RUSTY_DOOR_KEY_VERSION]=1;
    static_assert(FCI_MAX<=FC_COMBO_OBTAINED_FC_SIZE);
    const int native[]={
#define X(id,chain,flags,rg,ri,comboName,ootName,mmName) rg,
    FC_COMBO_ITEM_LIST(X)
#undef X
    };
    struct Alias { int fc; RandomizerGet item; const char* name; };
    const Alias aliases[]={
#define RUSTY_KEY(game,key,rg,ri,flag,name) {FCI_##game##_RUSTY_KEY_##key,rg,name},
#include "rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
    };
    REQUIRE(std::size(aliases)==66);
    for(const auto& a:aliases) {
        REQUIRE(native[a.fc]==a.item);
        REQUIRE(std::count(std::begin(native),std::end(native),a.item)==1);
        REQUIRE(!std::strcmp(gFcComboItems[a.fc].ootName,a.name));
    }
    PlayState play{};Actor door{};std::set<int> seen;unsigned cases=0;
    for(int scene=0;scene<SCENE_ID_MAX;++scene) for(int index=0;index<64;++index) {
        int item=RustyDoorKeys::SourceItem(scene,index);if(item<0)continue;
        ++cases;seen.insert(item);
        auto* key=RustyDoorKeys::Find(static_cast<RandomizerGet>(item));REQUIRE(key);
        auto& wrong=RustyDoorKeys::Catalog[(key-RustyDoorKeys::Catalog+1)%27];
        for(int age:{LINK_AGE_CHILD,LINK_AGE_ADULT}) {
            gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;gSaveContext.linkAge=age;
            options[RSK_LOCK_OVERWORLD_DOORS]=1;play.sceneNum=scene;door.params=index<<10;
            REQUIRE(Randomizer_RustyDoorLocked(&play,&door));
            Own(wrong.flag);REQUIRE(Randomizer_RustyDoorLocked(&play,&door));
            Own(key->flag);REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
            auto saved=std::make_unique<SaveContext>(gSaveContext);gSaveContext={};gSaveContext=*saved;
            REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
            gSaveContext.ship.randomizerInf[key->flag/16]&=~(1u<<(key->flag%16));
            options[RSK_LOCK_OVERWORLD_DOORS]=0;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
            options[RSK_LOCK_OVERWORLD_DOORS]=1;Own(RAND_INF_HAS_SKELETON_KEY);
            REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
            gSaveContext={};gSaveContext.ship.quest.id=QUEST_NORMAL;
            REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
        }
    }
    REQUIRE(seen.size()==26);REQUIRE(!seen.contains(RG_RUSTY_KEY_HYRULE_CASTLE));
    gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;options[RSK_LOCK_OVERWORLD_DOORS]=1;
    play.sceneNum=SCENE_MARKET_ENTRANCE_DAY;door.params=447;Own(RAND_INF_GUARD_HOUSE_UNLOCKED);
    REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    play.sceneNum=SCENE_KAKARIKO_VILLAGE;door.params=8<<10;Own(RAND_INF_KAK_POTION_SHOP_KEY_OBTAINED);
    REQUIRE(Randomizer_RustyDoorLocked(&play,&door));
    Own(RAND_INF_RUSTY_KEY_ADULT_POTION_SHOP_BACK);REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    // Coupled/decoupled shuffle destinations never change the physical forward owner.
    // Guard House -> Richard's House needs Guard key to enter, then its return is free.
    for(int age:{LINK_AGE_CHILD,LINK_AGE_ADULT}) for(int destination:{SCENE_DOG_LADY_HOUSE,SCENE_MARKET_GUARD_HOUSE}) {
        gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;gSaveContext.linkAge=age;
        play.sceneNum=SCENE_MARKET_ENTRANCE_DAY;door.params=3<<7;
        REQUIRE(Randomizer_RustyDoorLocked(&play,&door));
        Own(RAND_INF_RICHARDS_HOUSE_KEY_OBTAINED);REQUIRE(Randomizer_RustyDoorLocked(&play,&door));
        gSaveContext.ship.randomizerInf[RAND_INF_RICHARDS_HOUSE_KEY_OBTAINED/16]=0;
        Own(RAND_INF_GUARD_HOUSE_KEY_OBTAINED);REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
        play.sceneNum=destination;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    }
    for(int scene:{SCENE_SHOOTING_GALLERY,SCENE_DOG_LADY_HOUSE,SCENE_LAKESIDE_LABORATORY,SCENE_STABLE}) {
        gSaveContext={};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;play.sceneNum=scene;door.params=3<<7;
        REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    }
    play.sceneNum=SCENE_TREASURE_BOX_SHOP;door.params=3<<7;REQUIRE(!Randomizer_RustyDoorLocked(&play,&door));
    door.params=1<<7;REQUIRE(Randomizer_RustyDoorLocked(&play,&door));
    std::printf("PASS native OoT26 En_Door families (Castle uses native gate): %u source scene/index cases x both ages, own/wrong/none/skeleton, disabled/vanilla, save/legacy/shuffled-return escape and66 aliases\n",cases);
    return 0;
}
