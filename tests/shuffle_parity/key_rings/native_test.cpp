#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "Rando/StaticData/StaticData.h"
#include "tests/test_require.h"
#include "ComboDungeonKeyReceipt.h"
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
int DungeonItemToDungeon(RandoItemId);
}
namespace StaticData {
#include "checks.inc"
}
}

int main() {
    using namespace Rando;
#include "../legacy_ids.inc"
    const char* names[] = {"Woodfall Key Ring", "Snowhead Key Ring", "Great Bay Key Ring", "Stone Tower Key Ring"};
    const RandoItemId small[] = {RI_WOODFALL_SMALL_KEY, RI_SNOWHEAD_SMALL_KEY, RI_GREAT_BAY_SMALL_KEY, RI_STONE_TOWER_SMALL_KEY};
    const int totals[] = {1,3,1,4};
    RandoItemId rings[4];
    for (int i = 0; i < 4; ++i) {
        rings[i] = RI_UNKNOWN;
        for (const auto& [id, row] : StaticData::Items)
            if (!strcmp(row.name,names[i])) rings[i] = id;
        if (rings[i] == RI_UNKNOWN) {
            std::fprintf(stderr,"FAIL missing native MM catalogue entry: %s\n", names[i]);
            return 1;
        }
        REQUIRE(StaticData::Items.at(rings[i]).randoItemType == RITYPE_SMALL_KEY);
        REQUIRE(StaticData::Items.at(rings[i]).drawId != GID_NONE);
        REQUIRE(Logic::DungeonItemToDungeon(rings[i]) == Logic::DungeonItemToDungeon(small[i]));
        REQUIRE(ComboDungeonKeyReceipt::Find(names[i]).keyRingCount == totals[i]);
        REQUIRE(ComboDungeonKeyReceipt::Markup(names[i]).find(names[i]) != std::string::npos);
        REQUIRE(ComboDungeonKeyReceipt::Markup(names[i]).find("Small Key") != std::string::npos);
    }
    // The real native grant helper is shared with the headless oracle. Each
    // independently stale counter is repaired; repeat/cycle receipts do not
    // regenerate spent keys. Ownership survives a raw save-state round trip.
    for (const auto& ring : KeyRings::Catalog) {
        for (int inv : {-1,0,1}) for (int mirror : {-1,0,1}) {
            gSaveContext = {};
            DUNGEON_KEY_COUNT(ring.dungeon) = inv;
            auto& rando = gSaveContext.save.shipSaveInfo.rando;
            rando.foundDungeonKeys[ring.dungeon] = mirror;
            REQUIRE(KeyRings::Give(ring.ring));
            REQUIRE(KeyRings::Owned(ring));
            REQUIRE(DUNGEON_KEY_COUNT(ring.dungeon) == std::max(inv,int(ring.count)));
            REQUIRE(rando.foundDungeonKeys[ring.dungeon] == std::max(mirror,int(ring.count)));
            DUNGEON_KEY_COUNT(ring.dungeon) = 0;
            REQUIRE(KeyRings::Give(ring.ring));
            REQUIRE(DUNGEON_KEY_COUNT(ring.dungeon) == 0);
            auto saved = std::make_unique<SaveContext>();
            *saved = gSaveContext;
            gSaveContext = {};
            gSaveContext = *saved;
            REQUIRE(KeyRings::Owned(ring));
            // Execute the actual before/after native cycle-save hooks around
            // the engine boundary that clears transient dungeon keys.
            static PlayState play{};
            gPlayState = &play;
            play.sceneId = SCENE_MAX;
            MiscBehavior::BeforeEndOfCycleSave();
            DUNGEON_KEY_COUNT(ring.dungeon) = -1;
            MiscBehavior::AfterEndOfCycleSave();
            REQUIRE(DUNGEON_KEY_COUNT(ring.dungeon) == ring.count);
            REQUIRE(KeyRings::Owned(ring));
            REQUIRE(KeyRings::Remove(ring.ring));
            REQUIRE(!KeyRings::Owned(ring));
            REQUIRE(DUNGEON_KEY_COUNT(ring.dungeon) == 0 && rando.foundDungeonKeys[ring.dungeon] == 0);
        }
    }
    RandoOptionId option = RO_MAX;
    for (const auto& [id, row] : StaticData::Options)
        if (!strcmp(row.name,"RO_SHUFFLE_KEY_RINGS")) option = id;
    REQUIRE(option != RO_MAX && StaticData::Options.at(option).defaultValue == 0);
    auto& region = Logic::Regions[static_cast<RandoRegionId>(0)];
    for (const auto& [id, row] : StaticData::Checks) {
        if (id != RC_UNKNOWN) region.checks[id] = {[]{return true;}, "fixture region membership"};
    }
    for (int mask = 0; mask < 16; ++mask) {
        for (int placement : {RO_DUNGEON_ITEM_ANYWHERE, RO_DUNGEON_ITEM_OWN_DUNGEON,
                              RO_DUNGEON_ITEM_START_WITH, RO_DUNGEON_ITEM_VANILLA}) {
            for (bool plentiful : {false,true}) {
                auto save = std::make_unique<RandoSaveInfo>();
                for (const auto& [id, row] : StaticData::Options) save->randoSaveOptions[id] = row.defaultValue;
                save->randoSaveOptions[option] = mask;
                save->randoSaveOptions[RO_PLACEMENT_SMALL_KEYS] = placement;
                save->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
                std::vector<RandoCheckId> checks;
                std::vector<RandoItemId> items;
                Logic::GeneratePools(*save,checks,items);
                auto starting = GetComputedStartingItems(*save);
                for (int i = 0; i < 4; ++i) {
                    const bool selected = mask & (1 << i);
                    const bool pooled = placement == RO_DUNGEON_ITEM_ANYWHERE || placement == RO_DUNGEON_ITEM_OWN_DUNGEON;
                    REQUIRE(std::count(items.begin(),items.end(),rings[i]) == (pooled && selected ? 1 : 0));
                    REQUIRE(std::count(items.begin(),items.end(),small[i]) == (pooled && !selected ? totals[i] * (plentiful ? 2 : 1) : 0));
                    REQUIRE(std::count(starting.begin(),starting.end(),rings[i]) == (placement == RO_DUNGEON_ITEM_START_WITH && selected ? 1 : 0));
                    REQUIRE(std::count(starting.begin(),starting.end(),small[i]) == (placement == RO_DUNGEON_ITEM_START_WITH && !selected ? totals[i] : 0));
                }
            }
        }
    }
    std::puts("PASS actual MM catalogues/pools: all 16 ring selections, four placement modes, densities and computed starting items");
}
