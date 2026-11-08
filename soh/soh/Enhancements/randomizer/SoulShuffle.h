#pragma once
#include "randomizerTypes.h"

namespace Rando::SoulShuffle {
struct Entry { RandomizerGet item; RandomizerInf flag; RandomizerSettingKey option; const char* name; };
inline constexpr Entry Catalog[] = {
#define SOUL(game, family, key, name) SOUL_##game(family, key, name)
#define SOUL_OOT(family, key, name) { RG_SOUL_##family##_##key, RAND_INF_SOUL_##family##_##key, RSK_SHUFFLE_##family##_SOULS, name },
#define SOUL_MM(family, key, name)
#include "../../../../combo/rando/SoulShuffleCatalog.inc"
#undef SOUL_MM
#undef SOUL_OOT
#undef SOUL
};
inline constexpr const Entry* Find(RandomizerGet item) {
    for (const auto& soul : Catalog) if (soul.item == item) return &soul;
    return nullptr;
}
} // namespace Rando::SoulShuffle
