#include <map>
#include <string>
#include <cstdint>
static std::map<std::string, int32_t> values;
#include "foundation.inc"
#include "Rando/Rando.h"
#include "BenJsonConversions.hpp"
extern "C" {
#include "overlays/actors/ovl_En_Sth/z_en_sth.h"
}
#include <spdlog/spdlog.h>
#include <stdexcept>
#if __has_include("Rando/VanillaCompatibility.h")
#include "Rando/VanillaCompatibility.h"
#endif
extern "C" void CVarClear(const char* key) { values.erase(key); }
#include "compatibility.inc"
uint32_t Ship_Hash(std::string) { return 1234; }
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromConfig() { return {}; }
std::vector<RandoItemId> GetSariaPriorityItemsFromConfig() { return {}; }
void SetSariaPriorityItemsInSave(RandoSaveInfo&, std::vector<RandoItemId>&) {}
void GrantStartingItems() {}
void GiveItem(RandoItemId, RandoCheckId) {}
namespace Logic {
static int applied;
void ApplyNoLogicToSaveContext(std::vector<RandoCheckId>&, std::vector<RandoItemId>&) { applied=RO_LOGIC_NO_LOGIC; }
void ApplyNearlyNoLogicToSaveContext(std::vector<RandoCheckId>&, std::vector<RandoItemId>&) { applied=RO_LOGIC_NEARLY_NO_LOGIC; }
void ApplyGlitchlessLogicToSaveContext(std::vector<RandoCheckId>&, std::vector<RandoItemId>&) { applied=RO_LOGIC_GLITCHLESS; }
}
#include "generation.inc"
}
static constexpr RandoOptionId newOptions[] = {
    RO_SHUFFLE_NPC_SOULS, RO_SHUFFLE_ANIMAL_SOULS, RO_SHUFFLE_MISC_SOULS,
    RO_SHUFFLE_DEKU_STICK_BAG, RO_SHUFFLE_DEKU_NUT_BAG, RO_SHUFFLE_CHILD_WALLET,
    RO_INFINITE_UPGRADES, RO_SHUFFLE_RUSTY_KEYS, RO_SHUFFLE_KEY_RINGS, RO_SHUFFLE_SONG_EVENTS,
    RO_SHUFFLE_ICICLE_DROPS, RO_SHUFFLE_ROCK_DROPS, RO_SHUFFLE_BOULDER_DROPS,
    RO_SHUFFLE_RED_BOULDER_DROPS, RO_SHUFFLE_SOIL_DROPS, RO_SHUFFLE_LOTTERY_NIGHTS,
};
static void Configure(int logic, bool enabled) {
    values.clear();
    values[Rando::StaticData::Options[RO_LOGIC].cvar]=logic;
    for(auto option:newOptions) values[Rando::StaticData::Options[option].cvar]=enabled;
}
static void CheckDynamicChecks(const RandoSaveInfo& save, bool shuffled, bool obtained) {
#define CHECK_ENV(id) REQUIRE(save.randoSaveChecks[id].shuffled==shuffled); REQUIRE(save.randoSaveChecks[id].obtained==obtained); if(!shuffled) REQUIRE(!save.randoSaveChecks[id].eligible);
#define MM_ICICLE(id,scene,room,index,region) CHECK_ENV(id)
#include "Rando/MMEnvironmentIcicles.inc"
#undef MM_ICICLE
#define MM_ENVIRONMENT_CHECK(id,type,scene,region) CHECK_ENV(id)
#include "Rando/MMEnvironmentChecks.inc"
#undef MM_ENVIRONMENT_CHECK
#undef CHECK_ENV
    for(auto id:{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3}) {
        REQUIRE(save.randoSaveChecks[id].shuffled==shuffled);
        REQUIRE(save.randoSaveChecks[id].obtained==obtained);
        if(!shuffled) REQUIRE(!save.randoSaveChecks[id].eligible);
    }
}
int main() {
    // Candidate membership is a boundary; generation, option normalization,
    // balancing, branch selection and JSON serialization execute native code.
    for(auto& [id,row]:Rando::StaticData::Checks) if(id!=RC_UNKNOWN)
        Rando::Logic::Regions[static_cast<RandoRegionId>(0)].checks[id]={[]{return true;},"candidate membership"};
    Configure(RO_LOGIC_VANILLA,true);
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions) {
        values[Rando::StaticData::Options[option].cvar]=1;
        REQUIRE(IncompatibleWithLogicSetting(option));
    }
    ClearIncompatibleSetting();
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)
        REQUIRE(CVarGetInteger(Rando::StaticData::Options[option].cvar,0)==0);
    Configure(RO_LOGIC_GLITCHLESS,true);
    for(auto option:newOptions) REQUIRE(!IncompatibleWithLogicSetting(option));
    ClearIncompatibleSetting();
    for(auto option:newOptions) REQUIRE(CVarGetInteger(Rando::StaticData::Options[option].cvar,0)==1);
    for(bool on:{false,true}) {
        gSaveContext={}; Configure(RO_LOGIC_VANILLA,on);
        Rando::GenerateSeedInSaveContext("vanilla-compatibility");
        auto& save=gSaveContext.save.shipSaveInfo.rando;
        for(auto option:newOptions) REQUIRE(save.randoSaveOptions[option]==0);
        CheckDynamicChecks(save,false,false);
        REQUIRE(save.randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].shuffled);
        REQUIRE(save.randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].randoItemId==RI_OCARINA);
        auto encoded=nlohmann::json(save);
        RandoSaveInfo loaded=encoded.get<RandoSaveInfo>();
        CheckDynamicChecks(loaded,false,false);
        REQUIRE(loaded.randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].shuffled);
    }
    // Reproduce an already-saved experimental Vanilla seed with every new flag
    // and dynamic check set, plus persistent receipts. Deserialization must fix
    // effective restrictions without erasing those receipts or native checks.
    auto old=std::make_unique<RandoSaveInfo>();
    old->randoSaveOptions[RO_LOGIC]=RO_LOGIC_VANILLA;
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions) old->randoSaveOptions[option]=1;
    for(auto& check:old->randoSaveChecks) { check.shuffled=true;check.eligible=true;check.obtained=true;check.cycleObtained=true; }
    for(auto id:{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3}) {
        old->randoSaveChecks[id].randoItemId=RI_RUPEE_PURPLE;
        old->randoSaveChecks[id].price=123;old->randoSaveChecks[id].skipped=true;
    }
    old->randoInf[0]=0x4321;
    auto direct=std::make_unique<RandoSaveInfo>(*old);
    std::vector<RandoCheckId> directChecks; std::vector<RandoItemId> directItems;
    Rando::Logic::GeneratePools(*direct,directChecks,directItems);
    CheckDynamicChecks(*direct,false,true);
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions) REQUIRE(direct->randoSaveOptions[option]==0);
    for(int logic:{RO_LOGIC_GLITCHLESS,RO_LOGIC_NO_LOGIC,RO_LOGIC_NEARLY_NO_LOGIC}) {
        direct=std::make_unique<RandoSaveInfo>(*old); direct->randoSaveOptions[RO_LOGIC]=logic;
        auto unchanged=std::make_unique<RandoSaveInfo>(*direct);
        Rando::VanillaCompatibility::NormalizeSavedState(*direct);
        REQUIRE(memcmp(direct.get(),unchanged.get(),sizeof(RandoSaveInfo))==0);
    }
    auto encoded=nlohmann::json(*old);
    // Old native saves omit the event map; experimental ones may carry zeroes.
    for(bool missing:{false,true}) {
        auto input=encoded; if(missing) input.erase("songEvents");
        auto loaded=std::make_unique<RandoSaveInfo>(input.get<RandoSaveInfo>());
        for(auto option:Rando::VanillaCompatibility::UnsupportedOptions) REQUIRE(loaded->randoSaveOptions[option]==0);
        CheckDynamicChecks(*loaded,false,true);
        for(auto id:{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3}) {
            const auto& check=loaded->randoSaveChecks[id];
            REQUIRE(check.obtained && check.cycleObtained && check.skipped);
            REQUIRE(check.randoItemId==RI_RUPEE_PURPLE && check.price==123);
        }
        REQUIRE(loaded->randoInf[0]==0x4321);
        REQUIRE(loaded->randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].shuffled);
        REQUIRE(loaded->randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].eligible);
        REQUIRE(loaded->randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].cycleObtained);
        auto again=nlohmann::json(*loaded).get<RandoSaveInfo>();
        CheckDynamicChecks(again,false,true);
    }
    for(int logic:{RO_LOGIC_GLITCHLESS,RO_LOGIC_NO_LOGIC,RO_LOGIC_NEARLY_NO_LOGIC}) {
        gSaveContext={}; Configure(logic,true);
        Rando::GenerateSeedInSaveContext("randomized-control");
        const auto& save=gSaveContext.save.shipSaveInfo.rando;
        for(auto option:newOptions) REQUIRE(save.randoSaveOptions[option]==1);
        REQUIRE(Rando::Logic::applied==logic);
        auto loaded=nlohmann::json(save).get<RandoSaveInfo>();
        for(auto option:newOptions) REQUIRE(loaded.randoSaveOptions[option]==1);
        REQUIRE(memcmp(save.songEvents,loaded.songEvents,sizeof(save.songEvents))==0);
    }
    std::puts("PASS native Vanilla generation/import compatibility, old save receipts and randomized controls");
    return 0;
}
