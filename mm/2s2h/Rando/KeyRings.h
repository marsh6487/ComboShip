#pragma once

#include "Rando/Rando.h"
#include <algorithm>
#include "../../../combo/ComboMmKeyRingPolicy.h"
extern "C" {
#include "functions.h"
}

namespace Rando::KeyRings {
struct Definition {
    RandoItemId ring;
    RandoItemId smallKey;
    int dungeon;
    s8 count;
    RandoInf flag;
    uint32_t selection;
};
inline constexpr Definition Catalog[] = {
    { RI_WOODFALL_KEY_RING, RI_WOODFALL_SMALL_KEY, DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE, ComboMmKeyRing_KeyCount(0),
      RANDO_INF_WOODFALL_KEY_RING, 1u },
    { RI_SNOWHEAD_KEY_RING, RI_SNOWHEAD_SMALL_KEY, DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE, ComboMmKeyRing_KeyCount(1),
      RANDO_INF_SNOWHEAD_KEY_RING, 2u },
    { RI_GREAT_BAY_KEY_RING, RI_GREAT_BAY_SMALL_KEY, DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE, ComboMmKeyRing_KeyCount(2),
      RANDO_INF_GREAT_BAY_KEY_RING, 4u },
    { RI_STONE_TOWER_KEY_RING, RI_STONE_TOWER_SMALL_KEY, DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE,
      ComboMmKeyRing_KeyCount(3), RANDO_INF_STONE_TOWER_KEY_RING, 8u },
};
inline const Definition* Find(RandoItemId item) {
    for (const auto& ring : Catalog)
        if (ring.ring == item)
            return &ring;
    return nullptr;
}
inline bool Selected(const RandoSaveInfo& save, const Definition& ring) {
    return save.randoSaveOptions[RO_PLACEMENT_SMALL_KEYS] != RO_DUNGEON_ITEM_VANILLA &&
           (save.randoSaveOptions[RO_SHUFFLE_KEY_RINGS] & ring.selection) != 0;
}
inline bool Owned(const Definition& ring) {
    return Flags_GetRandoInf(ring.flag) != 0;
}
// Both the native grant and the headless oracle use these same save-only writes.
// The inventory and solver mirror can independently contain the -1 sentinel.
inline bool Give(RandoItemId item) {
    const auto* ring = Find(item);
    if (!ring)
        return false;
    if (!Owned(*ring)) {
        auto& save = gSaveContext.save.shipSaveInfo.rando;
        Flags_SetRandoInf(ring->flag);
        DUNGEON_KEY_COUNT(ring->dungeon) = std::max(DUNGEON_KEY_COUNT(ring->dungeon), ring->count);
        save.foundDungeonKeys[ring->dungeon] = std::max(save.foundDungeonKeys[ring->dungeon], ring->count);
    }
    return true;
}
inline bool Remove(RandoItemId item) {
    const auto* ring = Find(item);
    if (!ring)
        return false;
    if (Owned(*ring)) {
        auto& save = gSaveContext.save.shipSaveInfo.rando;
        Flags_ClearRandoInf(ring->flag);
        DUNGEON_KEY_COUNT(ring->dungeon) = std::max(0, DUNGEON_KEY_COUNT(ring->dungeon) - ring->count);
        save.foundDungeonKeys[ring->dungeon] = std::max(0, save.foundDungeonKeys[ring->dungeon] - ring->count);
    }
    return true;
}
inline void TransformPool(const RandoSaveInfo& save, std::vector<RandoItemId>& pool) {
    for (const auto& ring : Catalog) {
        if (Selected(save, ring) && std::erase(pool, ring.smallKey) != 0)
            pool.push_back(ring.ring);
    }
}
} // namespace Rando::KeyRings
