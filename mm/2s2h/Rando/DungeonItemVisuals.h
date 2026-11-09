#ifndef RANDO_DUNGEON_ITEM_VISUALS_H
#define RANDO_DUNGEON_ITEM_VISUALS_H

#include "Types.h"
#include "../../../combo/ComboMmKeyRingPolicy.h"

// Ownership travels with the item, including shuffled checks in other dungeons.
static inline int DungeonItem_GetOwner(RandoItemId id) {
    switch (id) {
        case RI_WOODFALL_SMALL_KEY:
        case RI_WOODFALL_KEY_RING:
        case RI_WOODFALL_BOSS_KEY:
        case RI_WOODFALL_MAP:
        case RI_WOODFALL_COMPASS:
            return 0;
        case RI_SNOWHEAD_SMALL_KEY:
        case RI_SNOWHEAD_KEY_RING:
        case RI_SNOWHEAD_BOSS_KEY:
        case RI_SNOWHEAD_MAP:
        case RI_SNOWHEAD_COMPASS:
            return 1;
        case RI_GREAT_BAY_SMALL_KEY:
        case RI_GREAT_BAY_KEY_RING:
        case RI_GREAT_BAY_BOSS_KEY:
        case RI_GREAT_BAY_MAP:
        case RI_GREAT_BAY_COMPASS:
            return 2;
        case RI_STONE_TOWER_SMALL_KEY:
        case RI_STONE_TOWER_KEY_RING:
        case RI_STONE_TOWER_BOSS_KEY:
        case RI_STONE_TOWER_MAP:
        case RI_STONE_TOWER_COMPASS:
            return 3;
        default:
            return -1;
    }
}

static inline int DungeonItem_GetKeyRingCount(RandoItemId id) {
    switch (id) {
        case RI_WOODFALL_KEY_RING:
        case RI_SNOWHEAD_KEY_RING:
        case RI_GREAT_BAY_KEY_RING:
        case RI_STONE_TOWER_KEY_RING:
            return ComboMmKeyRing_KeyCount(DungeonItem_GetOwner(id));
        default:
            return 0;
    }
}

#endif
