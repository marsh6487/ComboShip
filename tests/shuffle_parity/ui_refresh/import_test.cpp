// The common foundation runs real generation/catalogue/JSON code. Only engine
// grants, placement dispatch and sidecar file writes are service boundaries.
#define main GenerationFoundationMain
#include "vanilla_test.cpp"
#undef main
#include "Rando/Spoiler/Spoiler.h"
#include "Rando/SongEvents.h"
namespace Rando {
#include "spoiler_start.inc"
namespace Spoiler {
void SaveOotItemAreas(uint32_t, const nlohmann::json&) {}
#include "import.inc"
}
}

int main() {
    using json=nlohmann::json;
    for(int logic:{RO_LOGIC_VANILLA,RO_LOGIC_GLITCHLESS,RO_LOGIC_NO_LOGIC,RO_LOGIC_NEARLY_NO_LOGIC}) {
        auto& save=gSaveContext.save.shipSaveInfo.rando;
        gSaveContext={};
        save.randoInf[0]=0x4321;
        json spoiler={{"finalSeed",1234},{"options",json::object()},{"checks",json::object()},
                      {"startingItems",json::array()},{"sariaPriorityItems",json::array()}};
        for(auto& [id,option]:Rando::StaticData::Options)
            spoiler["options"][option.name]=option.defaultValue;
        spoiler["options"]["RO_LOGIC"]=logic;
        for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)
            spoiler["options"][Rando::StaticData::Options[option].name]=1;
        // This reproduces legacy invalid Vanilla map data. Randomized controls
        // carry an actual valid mapping, which must survive unchanged.
        const auto mapping=ComboRando::SongEvents::Generate(ComboRando::SongEvents::Game::Mm,1234,true);
        spoiler["songEvents"]=logic==RO_LOGIC_VANILLA ? json(std::vector<uint8_t>(mapping.size(),0)) : json(mapping);
        for(auto& [id,row]:Rando::StaticData::Checks) {
            spoiler["checks"][row.name]="RI_JUNK";
            save.randoSaveChecks[id].obtained=true;
            save.randoSaveChecks[id].cycleObtained=true;
            save.randoSaveChecks[id].eligible=true;
        }
        spoiler["checks"][Rando::StaticData::Checks[RC_CLOCK_TOWER_ROOF_OCARINA].name]="RI_OCARINA";
        for(auto id:{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3})
            spoiler["checks"][Rando::StaticData::Checks[id].name]=
                {{"randoItemId","RI_RUPEE_PURPLE"},{"price",77},{"skipped",true}};
        Rando::Spoiler::ApplyToSaveContext(spoiler);
        for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)
            REQUIRE(save.randoSaveOptions[option]==(logic==RO_LOGIC_VANILLA?0:1));
        CheckDynamicChecks(save,logic!=RO_LOGIC_VANILLA,true);
        REQUIRE(save.randoInf[0]==0x4321);
        REQUIRE(save.randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].shuffled);
        REQUIRE(save.randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].randoItemId==RI_OCARINA);
        REQUIRE(save.randoSaveChecks[RC_CLOCK_TOWER_ROOF_OCARINA].cycleObtained);
        if(logic!=RO_LOGIC_VANILLA)
            REQUIRE(std::equal(mapping.begin(),mapping.end(),save.songEvents));
        auto loaded=json(save).get<RandoSaveInfo>();
        CheckDynamicChecks(loaded,logic!=RO_LOGIC_VANILLA,true);
        for(auto id:{RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3}) {
            const auto& check=loaded.randoSaveChecks[id];
            REQUIRE(check.obtained && check.cycleObtained && check.skipped);
            REQUIRE(check.randoItemId==RI_RUPEE_PURPLE && check.price==77);
        }
    }
    std::puts("PASS native spoiler import: Vanilla repair, valid randomized controls and saved receipts");
}
