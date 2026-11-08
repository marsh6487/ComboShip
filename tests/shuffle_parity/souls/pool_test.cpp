// Execute the complete native pool generator with real game/rando data types.
// Save/config inputs and RNG are test boundaries; no item-placement code is
// mocked.
#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "Rando/SoulShuffle.h"
#include "tests/test_require.h"
#include <cstdio>
#include <memory>

extern "C" int32_t CVarGetInteger(const char *, int32_t fallback) {
  return fallback;
}

extern "C" {
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
int gMMComboGoalHunt = 0;
int gMMComboGoalRequired = 0;
int gMMComboGoalPieces = -1;
s32 Ship_Random(s32 min, s32 max) { return min; }
int FleetShipCombo_GetActiveGame(void) { return -1; }
}

// Extracted unchanged from FleetShipCombo.cpp by the runner. In ComboShip this
// intentionally returns false: the launcher, not NEI's old fill, owns the pool.
#include "unified_pool.inc"

static std::vector<RandoItemId> startingItems;
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo &) {
  return startingItems;
}
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo &) {
  return {};
}
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return {}; }
namespace StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;

} // namespace StaticData
namespace Logic {
std::map<RandoRegionId, RandoRegion> Regions;
} // namespace Logic
} // namespace Rando


int main() {
    using namespace Rando;
    auto &region = Logic::Regions[(RandoRegionId)0];
    // Large junk capacity avoids pool balancing obscuring the catalog assertions.
    for (int i=1;i<300;++i) {
        auto id=static_cast<RandoCheckId>(i);
        StaticData::Checks[id]={id,"fixture",RCTYPE_CHEST,SCENE_20SICHITAI,FLAG_NONE,0,RI_JUNK};
        region.checks[id]={[]{return true;},"true"};
    }
    StaticData::Items[RI_JUNK].randoItemType=RITYPE_JUNK;
    for(auto &s:SoulShuffle::Catalog) {
        const auto& item=StaticData::Items.at(s.item);
        REQUIRE(item.randoItemType==RITYPE_MAJOR);
        REQUIRE(item.itemId==ITEM_NONE && item.drawId!=GID_NONE);
        REQUIRE(std::string(item.name)==s.name);
        REQUIRE(StaticData::GetItemIdFromName(item.spoilerName)==s.item);
        REQUIRE(StaticData::Options.at(s.option).defaultValue==RO_GENERIC_OFF);
    }
    for(int mask=0;mask<8;++mask) {
        RandoSaveInfo save{};
        save.randoSaveOptions[RO_SHUFFLE_NPC_SOULS]=!!(mask&1);
        save.randoSaveOptions[RO_SHUFFLE_ANIMAL_SOULS]=!!(mask&2);
        save.randoSaveOptions[RO_SHUFFLE_MISC_SOULS]=!!(mask&4);
        std::vector<RandoCheckId> checks; std::vector<RandoItemId> items;
        Logic::GeneratePools(save,checks,items);
        for(auto &s:SoulShuffle::Catalog) {
            auto count=std::count(items.begin(),items.end(),s.item);
            REQUIRE(count == (save.randoSaveOptions[s.option] ? 1 : 0));
        }
        // Legacy enemy catalog is untouched by the three new options.
        for(int id=RI_SOUL_ENEMY_ALIEN;id<=RI_SOUL_ENEMY_WOLFOS;++id)
            REQUIRE(std::count(items.begin(),items.end(),(RandoItemId)id)==0);
    }
    puts("PASS complete native MM GeneratePools: all eight family combinations, each single copy, defaults absent");
}
