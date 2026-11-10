#pragma once

#include "Rando/Types.h"
#include "combo/rando/SongEvents.h"
extern "C" {
#include "z64save.h"
}
#include <algorithm>
#include <iterator>

namespace Rando::VanillaCompatibility {
// Vanilla assigns native rewards directly, so it cannot supply the added
// progression items required by these restrictions or shuffle new locations.
// This single policy serves UI edits, headless generation and imported saves.
inline constexpr RandoOptionId UnsupportedOptions[] = {
    RO_SHUFFLE_BOSS_SOULS,
    RO_SHUFFLE_SWIM,
    RO_SHUFFLE_ENEMY_SOULS,
    RO_SHUFFLE_OCARINA_BUTTONS,
    RO_PLENTIFUL_ITEMS,
    RO_CLOCK_SHUFFLE,
    RO_SHUFFLE_TYCOON_WALLET,
    RO_SHUFFLE_NPC_SOULS,
    RO_SHUFFLE_ANIMAL_SOULS,
    RO_SHUFFLE_MISC_SOULS,
    RO_SHUFFLE_DEKU_STICK_BAG,
    RO_SHUFFLE_DEKU_NUT_BAG,
    RO_SHUFFLE_CHILD_WALLET,
    RO_INFINITE_UPGRADES,
    RO_SHUFFLE_RUSTY_KEYS,
    RO_SHUFFLE_KEY_RINGS,
    RO_SHUFFLE_SONG_EVENTS,
    RO_SHUFFLE_ICICLE_DROPS,
    RO_SHUFFLE_ROCK_DROPS,
    RO_SHUFFLE_BOULDER_DROPS,
    RO_SHUFFLE_RED_BOULDER_DROPS,
    RO_SHUFFLE_SOIL_DROPS,
    RO_SHUFFLE_LOTTERY_NIGHTS,
    RO_SHUFFLE_DUNGEON_ENTRANCES,
    RO_SHUFFLE_EXPANDED_DUNGEON_ENTRANCES,
    RO_SHUFFLE_BOSS_ENTRANCES,
    RO_SHUFFLE_INTERIOR_ENTRANCES,
    RO_SHUFFLE_SHOP_ENTRANCES,
    RO_SHUFFLE_DOOR_INTERIORS,
    RO_SHUFFLE_GROTTO_ENTRANCES,
    RO_SHUFFLE_OVERWORLD_ENTRANCES,
    RO_DECOUPLED_ENTRANCES,
    RO_MIX_ENTRANCE_POOLS,
    RO_MIX_DUNGEON_ENTRANCES,
    RO_MIX_BOSS_ENTRANCES,
    RO_MIX_INTERIOR_ENTRANCES,
    RO_MIX_SHOP_ENTRANCES,
    RO_MIX_GROTTO_ENTRANCES,
    RO_MIX_OVERWORLD_ENTRANCES,
};

inline bool Unsupported(RandoOptionId option) {
    return std::find(std::begin(UnsupportedOptions), std::end(UnsupportedOptions), option) !=
           std::end(UnsupportedOptions);
}

inline void NormalizeOptions(RandoSaveInfo& save) {
    if (save.randoSaveOptions[RO_LOGIC] != RO_LOGIC_VANILLA)
        return;
    for (auto option : UnsupportedOptions)
        save.randoSaveOptions[option] = RO_GENERIC_OFF;
    save.soloEntrances = {};
    save.soloEntrances.checkpoint = -1;
    save.soloEntrances.faroresWindCheckpoint = -1;
}

inline void NormalizeSavedState(RandoSaveInfo& save) {
    if (save.randoSaveOptions[RO_LOGIC] != RO_LOGIC_VANILLA)
        return;
    NormalizeOptions(save);
    // Repair only the added dynamic checks. Ordinary native Vanilla assignments
    // and every persistent receipt, price, item and skip field remain intact.
    auto disable = [&](RandoCheckId check) {
        save.randoSaveChecks[check].shuffled = false;
        save.randoSaveChecks[check].eligible = false;
    };
#define MM_ICICLE(id, scene, room, index, region) disable(id);
#include "Rando/MMEnvironmentIcicles.inc"
#undef MM_ICICLE
#define MM_ENVIRONMENT_CHECK(id, type, scene, region) disable(id);
#include "Rando/MMEnvironmentChecks.inc"
#undef MM_ENVIRONMENT_CHECK
    disable(RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2);
    disable(RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3);
    const auto map = ComboRando::SongEvents::Defaults(ComboRando::SongEvents::Game::Mm);
    std::copy(map.begin(), map.end(), save.songEvents);
}
} // namespace Rando::VanillaCompatibility
