#include "2s2h/BenJsonConversions.hpp"
#include "combo/rando/CrossDungeonSeed.h"
#include "ComboGameMode.h"
#include "tests/test_require.h"
#include <cstdio>
#include <memory>

extern "C" {
#include "global.h"
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_MM;
#endif
}

static json SavedDungeonMap() {
    using namespace ComboRando::CrossDungeon;
    std::vector<Dungeon> dungeons{
        { "mm:woodfall", { 1, ENTRANCE(WOODFALL_TEMPLE, 0) }, { 1, ENTRANCE(WOODFALL, 1) },
          { 1, ENTRANCE(WOODFALL, 1) } },
        { "mm:snowhead", { 1, ENTRANCE(SNOWHEAD_TEMPLE, 0) }, { 1, ENTRANCE(SNOWHEAD, 1) },
          { 1, ENTRANCE(SNOWHEAD, 1) } },
        { "mm:greatbay", { 1, ENTRANCE(GREAT_BAY_TEMPLE, 0) }, { 1, ENTRANCE(ZORA_CAPE, 7) },
          { 1, ENTRANCE(ZORA_CAPE, 7) } },
        { "mm:stonetower", { 1, ENTRANCE(STONE_TOWER_TEMPLE, 0) }, { 1, ENTRANCE(STONE_TOWER, 2) },
          { 1, ENTRANCE(STONE_TOWER, 2) } },
        { "mm:inverted", { 1, ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0) },
          { 1, ENTRANCE(STONE_TOWER_INVERTED, 1) }, { 1, ENTRANCE(STONE_TOWER_INVERTED, 1) } },
    };
    dungeons[0].escapeExits = { { 1, ENTRANCE(WOODFALL, 3) } };
    dungeons[1].escapeExits = { { 1, ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7) } };
    dungeons[2].escapeExits = { { 1, ENTRANCE(ZORA_CAPE, 9) } };
    dungeons[4].escapeExits = { { 1, ENTRANCE(IKANA_CANYON, 15) } };
    auto seed = Encode(dungeons, { 1, 2, 0, 4, 3 });
    seed["version"] = 4;
    seed["options"] = OptionsJson(PoolOptions{});
    seed["options"]["overworld"] = false;
    seed["policy"] = { { "decoupled", false }, { "mixed", false }, { "mixDungeons", false },
                       { "mixBosses", false }, { "mixInteriors", false }, { "mixShops", false },
                       { "mixGrottos", false }, { "mixOverworld", false } };
    seed["reversePermutation"] = std::vector<size_t>{ 2, 0, 1, 4, 3 };
    seed["soloVersion"] = 2;
    seed["masterSeed"] = 73;
    seed["runtime"] = { { "checkpoint", ENTRANCE(SNOWHEAD_TEMPLE, 0) },
                        { "faroresWindCheckpoint", -1 }, { "completedBossMask", 0 } };
    return seed;
}

int main() {
    // A positioned Spiritual Stone recall needs its separately captured logical
    // entrance owner, especially for grottos sharing one physical scene/room.
    SpiritualStonesSaveData stones{};
    json stoneDocument = stones;
    stoneDocument["warp"][0]["entranceId"] = ENTRANCE(GROTTOS, 4);
    stoneDocument["warp"][0]["posX"] = 123.0f;
    stoneDocument["warp"][0]["soloEntranceCheckpoint"] = ENTRANCE(GROTTOS, 19);
    stoneDocument.get_to(stones);
    const json stoneRoundtrip = stones;
    REQUIRE(stoneRoundtrip["warp"][0].contains("soloEntranceCheckpoint"));
    REQUIRE(stoneRoundtrip["warp"][0]["soloEntranceCheckpoint"] == ENTRANCE(GROTTOS, 19));
    REQUIRE(stoneRoundtrip["warp"][0]["entranceId"] == ENTRANCE(GROTTOS, 4));
    REQUIRE(stoneRoundtrip["warp"][0]["posX"] == 123.0f);
    stoneDocument["warp"][0].erase("soloEntranceCheckpoint");
    stoneDocument.get_to(stones);
    REQUIRE(json(stones)["warp"][0]["soloEntranceCheckpoint"] == -1);
    json::object().get_to(stones);
    const json clearedStones = stones;
    for (const auto& warp : clearedStones["warp"])
        REQUIRE(warp["soloEntranceCheckpoint"] == -1);

    // Break caught: a native JSON load silently drops the saved entrance map,
    // so the same file resumes against a different or absent permutation.
    auto source = std::make_unique<RandoSaveInfo>();
    source->finalSeed = 73;
    auto legacyEvents = json(*source);
    legacyEvents["randoEvents"] = std::vector<uint8_t>(RE_MAX - 1, 0);
    source->randoEvents[RE_MAX - 1] = 255;
    bool legacyEventsSupported = true;
    try { legacyEvents.get_to(*source); }
    catch (const std::exception&) { legacyEventsSupported = false; }
    REQUIRE(legacyEventsSupported);
    REQUIRE(source->randoEvents[RE_MAX - 1] == 0);
    json saved = *source;
    saved["randoSaveOptions"][RO_SHUFFLE_DUNGEON_ENTRANCES] = 1;
    const auto entrances = SavedDungeonMap();
    saved["soloEntrances"] = entrances;
    auto selected = std::make_unique<RandoSaveInfo>();
    saved.get_to(*selected);
    json roundtrip = *selected;
    REQUIRE(roundtrip.contains("soloEntrances"));
    REQUIRE(roundtrip.at("soloEntrances") == entrances);

    // File select parses other files into temporary structures. Such a preview
    // must not change the selected save's replayable map or recovery metadata.
    auto previewDocument = saved;
    previewDocument["soloEntrances"]["permutation"] = std::vector<size_t>{ 0, 1, 2, 3, 4 };
    previewDocument["soloEntrances"]["reversePermutation"] = std::vector<size_t>{ 0, 1, 2, 3, 4 };
    previewDocument["soloEntrances"]["runtime"]["checkpoint"] = -1;
    auto preview = std::make_unique<RandoSaveInfo>();
    previewDocument.get_to(*preview);
    REQUIRE(json(*selected).at("soloEntrances") == entrances);
    REQUIRE(json(*preview).at("soloEntrances").at("permutation") == json({ 0, 1, 2, 3, 4 }));

    // A legacy/disabled file loaded into a reused context must clear every
    // local mapping and checkpoint rather than retaining the preceding file.
    auto legacy = saved;
    legacy.erase("soloEntrances");
    legacy["randoSaveOptions"][RO_SHUFFLE_DUNGEON_ENTRANCES] = 0;
    legacy.get_to(*selected);
    const auto cleared = json(*selected).at("soloEntrances");
    REQUIRE(cleared.is_null() || (cleared.contains("enabled") && !cleared.at("enabled").get<bool>()));

    // Foreign endpoints and non-bijective maps must be rejected before a bad
    // file can install a cross-game handoff in an otherwise solo session.
    for (int invalid = 0; invalid < 2; ++invalid) {
        auto corrupt = saved;
        if (invalid == 0) {
            corrupt["soloEntrances"]["dungeons"][0]["entrance"]["game"] = 0;
        } else {
            corrupt["soloEntrances"]["permutation"] = std::vector<size_t>{ 1, 1, 0, 4, 3 };
        }
        bool rejected = false;
        try {
            corrupt.get_to(*selected);
        } catch (const std::exception&) {
            rejected = true;
        }
        REQUIRE(rejected);
    }
    std::puts("PASS actual MM JSON: saved entrance map, checkpoint, preview isolation, legacy clear and foreign rejection");
}
