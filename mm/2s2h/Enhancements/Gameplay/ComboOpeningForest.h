#pragma once

#ifdef COMBO_BUILD
#include "global.h"
#include "../../../../combo/entrances/LostWoodsEntrance.h"
#include <math.h>
#include <string.h>

// The archive binder replaces only room zero's opaque mesh reference with
// this wrapper. Inspect the active mesh, so another pack winning load order
// cannot hide the fallback arch merely because this archive is mounted.
#define COMBO_OPENING_FOREST_PACK_ROOT "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_v1"
#define COMBO_OPENING_FOREST_STUMP_PACK_ROOT "__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_stump_v2"

// West face of the large broken stump in the opening clearing. The native
// ground is walkable here; the fade starts in front of its solid collision.
#define COMBO_OPENING_FOREST_STUMP_X 120.0f
#define COMBO_OPENING_FOREST_STUMP_Z 240.0f
#define COMBO_OPENING_FOREST_STUMP_YAW (-16384)

// A playable visit uses spawn zero/layer zero. Song of Time flashbacks use
// other spawns/layers and retain their native behavior. No story flag marks a visit.
static inline int ComboOpeningForest_IsExploration(const PlayState* play) {
    return play && play->sceneId == SCENE_LOST_WOODS && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && gSaveContext.sceneLayer == 0 &&
           gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0) && gSaveContext.save.cutsceneIndex == 0;
}

static inline void ComboOpeningForest_PrepareSpawn(PlayState* play) {
    if (!ComboOpeningForest_IsExploration(play) || !play->linkActorEntry)
        return;
    // Never edit the cached native/Alt resource or change the saved player form.
    static ActorEntry spawn;
    spawn = *play->linkActorEntry;
    spawn.params = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E);
    play->linkActorEntry = &spawn;
}

static inline void ComboOpeningForest_NormalizeEntry(void) {
    if (gSaveContext.gameMode != GAMEMODE_NORMAL || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) ||
        (gSaveContext.save.entrance >> 9) != ENTR_SCENE_OPENING_DUNGEON)
        return;
    // The forward fall normally enters the curse/tutorial sequence. Continue
    // into the ordinary Clock Tower instead, before any prologue actor initializes.
    gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR, 0);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.respawnFlag = 0;
}

static inline int ComboOpeningForest_ReturnPackVersion(const PlayState* play) {
    if (!ComboOpeningForest_IsExploration(play) || play->roomCtx.curRoom.num != 0)
        return 0;
    const RoomShape* shape = play->roomCtx.curRoom.roomShape;
    if (!shape || shape->base.type != ROOM_SHAPE_TYPE_NORMAL || shape->normal.numEntries != 1 ||
        !shape->normal.entries || !shape->normal.entries[0].opa)
        return 0;
    const char* path = (const char*)shape->normal.entries[0].opa;
    if (strcmp(path, COMBO_OPENING_FOREST_STUMP_PACK_ROOT) == 0)
        return 2;
    return strcmp(path, COMBO_OPENING_FOREST_PACK_ROOT) == 0 ? 1 : 0;
}

static inline int ComboOpeningForest_UsesPackTunnel(const PlayState* play) {
    return ComboOpeningForest_ReturnPackVersion(play) != 0;
}

static inline int ComboOpeningForest_AtReturn(const PlayState* play, const Vec3f* position) {
    if (!ComboOpeningForest_IsExploration(play) || play->roomCtx.curRoom.num != 0 || !play->linkActorEntry)
        return 0;
    const int version = ComboOpeningForest_ReturnPackVersion(play);
    // Keep the visible doorway and trigger together when an older pack wins
    // load order. Native assets use the temporary arch at the new stump.
    const float x = version == 1 ? play->linkActorEntry->pos.x : COMBO_OPENING_FOREST_STUMP_X;
    const float z = version == 1 ? play->linkActorEntry->pos.z : COMBO_OPENING_FOREST_STUMP_Z;
    const float y = version == 1 ? play->linkActorEntry->pos.y : 0.0f;
    const s16 yaw = version == 1 ? play->linkActorEntry->rot.y : COMBO_OPENING_FOREST_STUMP_YAW;
    const float dx = position->x - x, dz = position->z - z, dy = position->y - y;
    const float angle = yaw * (6.28318530718f / 65536.0f);
    const float lateral = dx * cosf(angle) - dz * sinf(angle);
    const float depth = dx * sinf(angle) + dz * cosf(angle);
    const float halfWidth = version ? 58.0f : 42.0f;
    // Trigger ahead of the black face (depth -24), before the existing stump
    // wall. The retained native arrival is well outside these bounds.
    return lateral > -halfWidth && lateral < halfWidth && depth > -62.0f && depth < 12.0f && dy > -40.0f && dy < 130.0f;
}
#endif
