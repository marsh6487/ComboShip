#pragma once
#include "randomizerEnums.h"
#include "SoulShuffleEnemy.h"

namespace Rando::EnemyFirstDrops {
struct Entry {
    RandomizerCheck check;
    RandomizerInf flag;
    RandomizerInf defeatedFlag;
    RandomizerGet soul;
    const char* name;
};
inline constexpr Entry Catalog[] = {
#define ENEMY_FIRST_DROP(key, name) \
    { RC_ENEMY_DROP_##key, RAND_INF_ENEMY_DROP_##key, RAND_INF_ENEMY_DEFEATED_##key, RG_SOUL_ENEMY_##key, name },
#include "EnemyFirstDropsCatalog.inc"
#undef ENEMY_FIRST_DROP
};
inline constexpr const Entry* Find(RandomizerCheck check) {
    for (const auto& entry : Catalog)
        if (entry.check == check)
            return &entry;
    return nullptr;
}
inline const Entry* ForActor(int actorId, uint16_t params, bool adult, bool gohmaArena) {
    const auto soul = SoulShuffle::EnemyForActor(actorId, params, adult, gohmaArena);
    for (const auto& entry : Catalog)
        if (entry.soul == soul)
            return &entry;
    return nullptr;
}
void RegisterLocations();
void RegisterAccess();
} // namespace Rando::EnemyFirstDrops
