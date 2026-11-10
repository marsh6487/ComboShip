#pragma once
#include <initializer_list>
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
// Exact native identities exported for configured starting shared souls.
inline constexpr const char* SharedName(RandoItemId item) {
    struct SharedEntry {
        RandoItemId item;
        const char* name;
    };
    constexpr SharedEntry entries[] = {
#define SHARED_SOUL(category, oot, mm, ootName, mmName) { RI_SOUL_##category##_##mm, mmName },
#include "../../../combo/rando/SharedSoulCatalog.inc"
#undef SHARED_SOUL
    };
    for (const auto& entry : entries)
        if (entry.item == item)
            return entry.name;
    return nullptr;
}
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
template <class Requirement> inline bool CheckRequirements(RandoCheckId check, Requirement requirement) {
    switch (check) {
#define SOUL_CHECK(check, ...)                   \
    case check:                                  \
        for (RandoItemId item : { __VA_ARGS__ }) \
            if (!requirement(item))              \
                return false;                    \
        return true;
#include "SoulShuffleChecks.inc"
#undef SOUL_CHECK
        default:
            return true;
    }
}
} // namespace Rando::SoulShuffle
