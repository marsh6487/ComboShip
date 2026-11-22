#pragma once

#include "../SoulShuffle.h"
#include "../../../../combo/rando/SoulTrackerData.h"
#include <cstddef>
#include <vector>

namespace Rando::SoulTracker {

using ComboRando::SoulTracker::Family;
inline constexpr char SoloPauseMapCvar[] = "gRandoEnhancements.SoulTrackerPauseMapMM";

struct Entry {
    RandoItemId item;
    RandoInf flag;
    RandoOptionId option;
    Family family;
    const char* name;
};

// Use the same native flags as the grant and actor-interaction paths. This is
// an inventory view, never an inference from check locations or cycle flags.
template <class Append> void ForEach(Append append) {
    for (int item = RI_SOUL_BOSS_GOHT; item <= RI_SOUL_ENEMY_WOLFOS; ++item) {
        const bool boss = item <= RI_SOUL_BOSS_TWINMOLD;
        append(Entry{ static_cast<RandoItemId>(item),
                      static_cast<RandoInf>(item - RI_SOUL_BOSS_GOHT + RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT),
                      boss ? RO_SHUFFLE_BOSS_SOULS : RO_SHUFFLE_ENEMY_SOULS, boss ? Family::Boss : Family::Enemy,
                      nullptr });
    }
    for (const auto& soul : SoulShuffle::Catalog) {
        const Family family = soul.option == RO_SHUFFLE_NPC_SOULS      ? Family::Npc
                              : soul.option == RO_SHUFFLE_ANIMAL_SOULS ? Family::Animal
                                                                       : Family::Misc;
        append(Entry{ soul.item, soul.flag, soul.option, family, soul.name });
    }
}

template <class Enabled> std::vector<Entry> Rows(Family family, Enabled enabled) {
    std::vector<Entry> rows;
    ForEach([&](const Entry& entry) {
        if (entry.family == family && enabled(entry.option))
            rows.push_back(entry);
    });
    return rows;
}

constexpr std::size_t RowsPerPage = 12;
inline std::size_t PageCount(std::size_t count) {
    return count == 0 ? 1 : (count + RowsPerPage - 1) / RowsPerPage;
}

} // namespace Rando::SoulTracker
