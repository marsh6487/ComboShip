#define main GenerationFoundationMain
#include "vanilla_test.cpp"
#undef main
#include "tracker_total.inc"

int main() {
    auto& save=gSaveContext.save.shipSaveInfo.rando;
    const auto first=RC_CLOCK_TOWER_ROOF_OCARINA, skipped=RC_CLOCK_TOWER_ROOF_SONG_OF_TIME,
               pending=RC_ASTRAL_OBSERVATORY_MOON_TEAR;
    for(auto id:{first,skipped,pending}) save.randoSaveChecks[id].shuffled=true;
    save.randoSaveChecks[first].obtained=true;
    save.randoSaveChecks[skipped].skipped=true;
    const auto inactive=RC_SNOWHEAD_TEMPLE_ICICLE_ENTRANCE_1;
    save.randoSaveChecks[inactive].obtained=true;
    save.randoSaveChecks[RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2].skipped=true;
    REQUIRE(GetTotalCheckCount()=="2/3");
    save.randoSaveChecks[pending].obtained=true;
    REQUIRE(GetTotalCheckCount()=="3/3");
    save.randoSaveChecks[skipped].shuffled=false;
    REQUIRE(GetTotalCheckCount()=="2/2");
    REQUIRE(save.randoSaveChecks[skipped].skipped && save.randoSaveChecks[inactive].obtained);
    for(auto& check:save.randoSaveChecks) check.shuffled=false;
    REQUIRE(GetTotalCheckCount()=="0/0");

    // Retained receipts from old experimental Vanilla saves must not inflate
    // the derived count after their added dynamic sources have been disabled.
    save={}; save.randoSaveOptions[RO_LOGIC]=RO_LOGIC_VANILLA;
    size_t nativeChecks=0;
    for(auto& [id,row]:Rando::StaticData::Checks) if(id!=RC_UNKNOWN) {
        save.randoSaveChecks[id].shuffled=true;save.randoSaveChecks[id].obtained=true;
        ++nativeChecks;
    }
    Rando::VanillaCompatibility::NormalizeSavedState(save);
    const auto active=std::to_string(nativeChecks-213);
    REQUIRE(GetTotalCheckCount()==active+"/"+active);
    CheckDynamicChecks(save,false,true);
    auto loaded=nlohmann::json(save).get<RandoSaveInfo>(); save=loaded;
    REQUIRE(GetTotalCheckCount()==active+"/"+active);
    CheckDynamicChecks(save,false,true);
    // Randomized modes retain their explicit active check set and receipts.
    save.randoSaveOptions[RO_LOGIC]=RO_LOGIC_NO_LOGIC;
    for(auto& [id,row]:Rando::StaticData::Checks) if(id!=RC_UNKNOWN) save.randoSaveChecks[id].shuffled=true;
    Rando::VanillaCompatibility::NormalizeSavedState(save);
    const auto all=std::to_string(nativeChecks);
    REQUIRE(GetTotalCheckCount()==all+"/"+all);
    std::puts("PASS actual native tracker totals: active obtained/skipped/pending, inactive receipts, repaired Vanilla reload");
}
