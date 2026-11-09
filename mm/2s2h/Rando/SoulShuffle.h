#pragma once
#include "Types.h"

namespace Rando::SoulShuffle {
struct Entry {
    RandoItemId item;
    RandoInf flag;
    RandoOptionId option;
    const char* name;
};
inline constexpr Entry Catalog[] = {
#define SOUL(game, family, key, name) SOUL_##game(family, key, name)
#define SOUL_MM(family, key, name) \
    { RI_SOUL_##family##_##key, RANDO_INF_SOUL_##family##_##key, RO_SHUFFLE_##family##_SOULS, name },
#define SOUL_OOT(family, key, name)
#include "../../../combo/rando/SoulShuffleCatalog.inc"
#undef SOUL_OOT
#undef SOUL_MM
#undef SOUL
};
inline constexpr const Entry* Find(RandoItemId item) {
    for (const auto& soul : Catalog)
        if (soul.item == item)
            return &soul;
    return nullptr;
}
inline constexpr RandoItemId EventItem(RandoEvent event) {
    switch (event) {
#define SOUL_EVENT(event, item) \
    case event:                 \
        return item;
#include "SoulShuffleEvents.inc"
#undef SOUL_EVENT
        default:
            return RI_UNKNOWN;
    }
}
inline constexpr RandoItemId CheckItem(RandoCheckId check) {
    switch (check) {
#define SOUL_CHECK(check, item) \
    case check:                 \
        return item;
#include "SoulShuffleChecks.inc"
#undef SOUL_CHECK
        default:
            return RI_UNKNOWN;
    }
}
} // namespace Rando::SoulShuffle
