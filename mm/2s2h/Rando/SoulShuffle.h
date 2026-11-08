#pragma once
#include "Types.h"

namespace Rando::SoulShuffle {
struct Entry { RandoItemId item; RandoInf flag; RandoOptionId option; const char* name; };
inline constexpr Entry Catalog[] = {
#define SOUL(game, family, key, name) SOUL_##game(family, key, name)
#define SOUL_MM(family, key, name) { RI_SOUL_##family##_##key, RANDO_INF_SOUL_##family##_##key, RO_SHUFFLE_##family##_SOULS, name },
#define SOUL_OOT(family, key, name)
#include "../../../combo/rando/SoulShuffleCatalog.inc"
#undef SOUL_OOT
#undef SOUL_MM
#undef SOUL
};
inline constexpr const Entry* Find(RandoItemId item) {
    for (const auto& soul : Catalog) if (soul.item == item) return &soul;
    return nullptr;
}
} // namespace Rando::SoulShuffle
