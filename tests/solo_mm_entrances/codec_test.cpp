#include "Rando/SoloEntrances.h"
#include "Rando/Rando.h"
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>

extern "C" {
SaveContext gSaveContext{};
int gComboGameMode = COMBO_GAME_MM;
int MM_SetSoloEntranceSeed(const char*);
}

static void Rejects(const std::function<void()>& action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    assert(rejected);
}

int main() {
    using namespace Rando::SoloEntrances;
    using nlohmann::json;
    auto save = std::make_unique<RandoSaveInfo>();
    save->finalSeed = 73;
    save->randoSaveOptions[RO_LOGIC] = RO_LOGIC_GLITCHLESS;
    const RandoOptionId pools[] = {RO_SHUFFLE_DUNGEON_ENTRANCES, RO_SHUFFLE_EXPANDED_DUNGEON_ENTRANCES,
        RO_SHUFFLE_BOSS_ENTRANCES, RO_SHUFFLE_INTERIOR_ENTRANCES, RO_SHUFFLE_SHOP_ENTRANCES,
        RO_SHUFFLE_GROTTO_ENTRANCES, RO_SHUFFLE_OVERWORLD_ENTRANCES};
    const RandoOptionId mixing[] = {RO_DECOUPLED_ENTRANCES, RO_MIX_ENTRANCE_POOLS, RO_MIX_DUNGEON_ENTRANCES,
        RO_MIX_BOSS_ENTRANCES, RO_MIX_INTERIOR_ENTRANCES, RO_MIX_SHOP_ENTRANCES,
        RO_MIX_GROTTO_ENTRANCES, RO_MIX_OVERWORLD_ENTRANCES};
    for (auto setting : pools) save->randoSaveOptions[setting] = 1;
    for (auto setting : mixing) save->randoSaveOptions[setting] = 1;
    Generate(*save, 73);
    const auto original = Seed(*save);
    assert(original.at("runtime").contains("faroresWindCheckpoint") &&
           original.at("runtime").at("faroresWindCheckpoint") == -1);
    assert(original.at("soloVersion") == 2 && original.at("dungeons").size() == 124);
    assert(original.at("permutation").size() == 124 && original.at("reversePermutation").size() == 124);
    const auto routes = BuildRoutes(*save);
    assert(!routes.empty() && CatalogueForSave(*save).size() == 124);
    for (const auto& route : routes) {
        assert(route.from.game == 1 && route.to.game == 1);
        assert(!route.arrivalCheckpoint || route.arrivalCheckpoint->game == 1);
    }
    ValidateSettings(*save);
    assert(BossesUsePhysicalClears(*save));
    for (int boss = 8; boss < 12; ++boss) assert(BossSourceForSave(*save, boss) == boss);
    Generate(*save, 73);
    assert(Seed(*save) == original);
    Generate(*save, 74);
    assert(Seed(*save).at("permutation") != original.at("permutation"));
    ApplySeed(*save, original);
    const auto before = save->soloEntrances;

    auto malformed = original;
    malformed["permutation"][0] = malformed["permutation"][1];
    Rejects([&] { ApplySeed(*save, malformed); });
    assert(std::memcmp(&before, &save->soloEntrances, sizeof(before)) == 0);
    malformed = original; malformed["dungeons"][0]["entrance"]["game"] = 0;
    Rejects([&] { ApplySeed(*save, malformed); });
    malformed = original; malformed["dungeons"][0]["escapeExits"][0]["game"] = 0;
    Rejects([&] { ApplySeed(*save, malformed); });
    malformed = original; malformed["options"]["overworld"] = false;
    Rejects([&] { ApplySeed(*save, malformed); });
    malformed = original; malformed["policy"]["mixed"] = false;
    Rejects([&] { ApplySeed(*save, malformed); });
    malformed = original; malformed["runtime"]["checkpoint"] = 0xF01F;
    Rejects([&] { ApplySeed(*save, malformed); });
    malformed = original; malformed["runtime"]["completedBossMask"] = 1;
    Rejects([&] { ApplySeed(*save, malformed); });
    malformed = original; malformed["runtime"]["faroresWindCheckpoint"] = 0xF01F;
    Rejects([&] { ApplySeed(*save, malformed); });
    auto recall = original;
    recall["runtime"]["faroresWindCheckpoint"] = ENTRANCE(ODOLWAS_LAIR, 0);
    ApplySeed(*save, recall);
    assert(Seed(*save) == recall);
    recall["runtime"].erase("faroresWindCheckpoint");
    ApplySeed(*save, recall);
    assert(save->soloEntrances.faroresWindCheckpoint == -1);
    ApplySeed(*save, original);

    // Selected data is authoritative; changing settings does not silently reinterpret it.
    save->randoSaveOptions[RO_SHUFFLE_OVERWORLD_ENTRANCES] = 0;
    Rejects([&] { ValidateSettings(*save); });
    save->randoSaveOptions[RO_SHUFFLE_OVERWORLD_ENTRANCES] = 1;
    save->finalSeed = 74;
    Rejects([&] { ValidateSettings(*save); });
    save->finalSeed = 73;
    ApplySeed(*save, nullptr);
    Rejects([&] { ValidateSettings(*save); });
    assert(Seed(*save).is_null() && save->soloEntrances.checkpoint == -1);
    ApplySeed(*save, original);

    // An explicit oracle seed survives every native reset and rejected updates.
    assert(MM_SetSoloEntranceSeed(original.dump().c_str()));
    auto oracle = std::make_unique<RandoSaveInfo>();
    oracle->randoSaveOptions[RO_LOGIC] = RO_LOGIC_GLITCHLESS;
    RestoreForOracle(*oracle);
    assert(Seed(*oracle) == original && oracle->finalSeed == 73);
    ValidateSettings(*oracle);
    malformed = original; malformed["reversePermutation"][0] = 999;
    assert(!MM_SetSoloEntranceSeed(malformed.dump().c_str()));
    RestoreForOracle(*oracle);
    assert(Seed(*oracle) == original);
    gComboGameMode = COMBO_GAME_BOTH;
    assert(!MM_SetSoloEntranceSeed(original.dump().c_str()));
    Generate(*save, 73);
    assert(Seed(*save).is_null());
    gComboGameMode = COMBO_GAME_MM;
    assert(MM_SetSoloEntranceSeed("null"));
    RestoreForOracle(*oracle);
    assert(Seed(*oracle).is_null() && oracle->randoSaveOptions[RO_SHUFFLE_OVERWORLD_ENTRANCES] == 0);

    // Native event ownership follows a boss-only permutation, including Twinmold.
    *save = {};
    save->finalSeed = 73;
    save->randoSaveOptions[RO_LOGIC] = RO_LOGIC_GLITCHLESS;
    save->randoSaveOptions[RO_SHUFFLE_BOSS_ENTRANCES] = 1;
    Generate(*save, 73);
    const uint16_t swap[] = {3, 1, 2, 0};
    std::copy(std::begin(swap), std::end(swap), save->soloEntrances.permutation);
    std::copy(std::begin(swap), std::end(swap), save->soloEntrances.reversePermutation);
    assert(!BossesUsePhysicalClears(*save));
    assert(BossSourceForSave(*save, 11) == 8 && BossSourceForSave(*save, 8) == 11);
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.rando = *save;
    RANDO_EVENTS[RE_CLEARED_STONE_TOWER_TEMPLE] = 1;
    assert(LogicalClear(8, 0) && !LogicalClear(11, 0));
    RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE] = 1;
    assert(LogicalClear(11, 0));

    // A native grotto return is a virtual portal even when its physical scene
    // index is also an independently shuffled overworld walking entrance.
    *save = {};
    save->randoSaveOptions[RO_LOGIC] = RO_LOGIC_GLITCHLESS;
    save->randoSaveOptions[RO_SHUFFLE_OVERWORLD_ENTRANCES] = 1;
    Generate(*save, 73);
    for (size_t i = 0; i < save->soloEntrances.count; ++i)
        save->soloEntrances.permutation[i] = save->soloEntrances.reversePermutation[i] = i;
    std::swap(save->soloEntrances.permutation[0], save->soloEntrances.permutation[2]);
    std::swap(save->soloEntrances.reversePermutation[0], save->soloEntrances.reversePermutation[2]);
    gSaveContext.save.shipSaveInfo.rando = *save;
    assert(ResolveEntrance(ENTRANCE(GREAT_BAY_COAST, 0), RR_GREAT_BAY_COAST_FISHERMAN_GROTTO) ==
           ENTRANCE(GREAT_BAY_COAST, 0));
    assert(ResolveEntrance(ENTRANCE(GREAT_BAY_COAST, 0), RR_TERMINA_FIELD_BEFORE_GREAT_BAY_COAST) ==
           ENTRANCE(WEST_CLOCK_TOWN, 0));
    save->randoSaveOptions[RO_SHUFFLE_OVERWORLD_ENTRANCES] = 0;
    save->randoSaveOptions[RO_SHUFFLE_GROTTO_ENTRANCES] = 1;
    Generate(*save, 73);
    for (size_t i = 0; i < save->soloEntrances.count; ++i)
        save->soloEntrances.permutation[i] = save->soloEntrances.reversePermutation[i] = i;
    std::swap(save->soloEntrances.permutation[0], save->soloEntrances.permutation[1]);
    std::swap(save->soloEntrances.reversePermutation[0], save->soloEntrances.reversePermutation[1]);
    gSaveContext.save.shipSaveInfo.rando = *save;
    assert(ResolveEntrance(ENTRANCE(TERMINA_FIELD, 0), RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_3) == 0xF001);
    std::cout << "PASS native Solo codec: 124 endpoints, schema2, strict replay, settings/seed isolation, boss clears\n";
}
