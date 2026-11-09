#pragma once
#include "randomizerEnums.h"
#include "SoulShuffleEnemy.h"
#include "z64scene.h"

namespace Rando::EnemyFirstDrops {
struct Entry {
    RandomizerCheck check;
    RandomizerInf flag;
    RandomizerInf defeatedFlag;
    RandomizerGet soul;
    RandomizerHintTextKey hint;
    const char* name;
};
inline constexpr Entry Catalog[] = {
#define ENEMY_FIRST_DROP(key, name)                                                  \
    { RC_ENEMY_DROP_##key, RAND_INF_ENEMY_DROP_##key, RAND_INF_ENEMY_DEFEATED_##key, \
      RG_SOUL_ENEMY_##key, RHT_ENEMY_DROP_##key,      name },
#include "EnemyFirstDropsCatalog.inc"
#undef ENEMY_FIRST_DROP
};
// Physical source layouts, separate from the globally owned check. Conditions
// remain on the real source regions; this table only determines seed inclusion.
struct WitnessProfile {
    RandomizerCheck check;
    SceneID scene;
    RandomizerCheckQuest quest;
};
inline constexpr WitnessProfile Witnesses[] = {
#define ENEMY_DROP_WITNESS(key, region, scene, quest, condition) { RC_ENEMY_DROP_##key, scene, quest },
#include "EnemyFirstDropsWitnesses.inc"
#include "EnemyFirstDropsAdditionalWitnesses.inc"
#undef ENEMY_DROP_WITNESS
};
template <class IsMQ> bool IsAvailable(RandomizerCheck check, IsMQ isMQ) {
    for (const auto& witness : Witnesses) {
        if (witness.check == check &&
            (witness.quest == RCQUEST_BOTH || isMQ(witness.scene) == (witness.quest == RCQUEST_MQ)))
            return true;
    }
    return false;
}
bool SeedUsesMQ(SceneID scene);
bool HasSeededCheck(RandomizerCheck check);
void RefreshSeedExclusions();
inline constexpr const Entry* Find(RandomizerCheck check) {
    for (const auto& entry : Catalog)
        if (entry.check == check)
            return &entry;
    return nullptr;
}
inline const Entry* ForActor(int actorId, uint16_t params, bool adult, bool gohmaArena) {
    // EnTp emits defeat for severed tail segments too. Only its negative-params
    // head is an enemy-type defeat; see native EnTp_SetupDie / TailpasaranType.
    if (actorId == ACTOR_EN_TP && static_cast<int16_t>(params) >= 0)
        return nullptr;
    const auto soul = SoulShuffle::EnemyForActor(actorId, params, adult, gohmaArena);
    for (const auto& entry : Catalog)
        if (entry.soul == soul)
            return &entry;
    return nullptr;
}
void RegisterLocations();
void RegisterAccess();
} // namespace Rando::EnemyFirstDrops
