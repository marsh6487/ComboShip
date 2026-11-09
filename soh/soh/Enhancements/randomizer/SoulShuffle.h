#pragma once
#include "randomizerTypes.h"

namespace Rando::SoulShuffle {
struct Entry {
    RandomizerGet item;
    RandomizerInf flag;
    RandomizerSettingKey option;
    const char* name;
};
inline constexpr Entry Catalog[] = {
#define SOUL(game, family, key, name) SOUL_##game(family, key, name)
#define SOUL_OOT(family, key, name) \
    { RG_SOUL_##family##_##key, RAND_INF_SOUL_##family##_##key, RSK_SHUFFLE_##family##_SOULS, name },
#define SOUL_MM(family, key, name)
#include "../../../../combo/rando/SoulShuffleCatalog.inc"
#undef SOUL_MM
#undef SOUL_OOT
#undef SOUL
};
// Exact native identities exported for configured starting shared souls.
inline constexpr const char* SharedName(RandomizerGet item) {
    struct SharedEntry {
        RandomizerGet item;
        const char* name;
    };
    constexpr SharedEntry entries[] = {
#define SHARED_SOUL(category, oot, mm, ootName, mmName) { RG_SOUL_##category##_##oot, ootName },
#include "../../../../combo/rando/SharedSoulCatalog.inc"
#undef SHARED_SOUL
    };
    for (const auto& entry : entries)
        if (entry.item == item)
            return entry.name;
    switch (item) {
#define SOUL_LEGACY(item, name) \
    case item:                  \
        return name;
#include "SoulShuffleLegacy.inc"
#undef SOUL_LEGACY
        default:
            break;
    }
    return nullptr;
}
inline constexpr const Entry* Find(RandomizerGet item) {
    for (const auto& soul : Catalog)
        if (soul.item == item)
            return &soul;
    return nullptr;
}
} // namespace Rando::SoulShuffle
