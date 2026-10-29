#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void lusprintf(const char* file, s32 line, s32 level, const char* format, ...) {
    (void)file; (void)line; (void)level; (void)format;
}

static int sFailCacheGrowth;
static int sOwnedCacheAllocations;
static void* CacheRealloc(void* previous, size_t bytes) {
    if (sFailCacheGrowth)
        return NULL;
    void* grown = realloc(previous, bytes);
    if (grown != NULL && previous == NULL)
        ++sOwnedCacheAllocations;
    return grown;
}
static void CacheFree(void* allocation) {
    if (allocation != NULL)
        --sOwnedCacheAllocations;
    free(allocation);
}
#define realloc CacheRealloc
#define free CacheFree
#include "mods/items/helpers/timestop_helper.c"
#undef realloc
#undef free

static PlayState play;
static Player player;
static Actor actors[160];
static ColliderCylinder cylinders[160];
PlayState* gPlayState = &play;
f32 gChampionSlowFactor = 1;
u16 gTimeSpeed = 17;
void Audio_StopSfxByPos(Vec3f* position) { (void)position; }
static void ActorUpdate(Actor* actor, PlayState* state) { (void)actor; (void)state; }

static void Setup(int count) {
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    play.sceneNum = 1;
    player.actor.category = ACTORCAT_PLAYER;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    play.actorCtx.actorLists[ACTORCAT_ENEMY].head = actors;
    for (int i = 0; i < count; ++i) {
        actors[i].id = i;
        actors[i].category = ACTORCAT_ENEMY;
        actors[i].update = ActorUpdate;
        actors[i].next = i + 1 < count ? &actors[i + 1] : NULL;
        cylinders[i].base.actor = &actors[i];
        cylinders[i].base.shape = COLSHAPE_CYLINDER;
        cylinders[i].base.acFlags = AC_ON;
    }
    TimeCtl_Update(&play);
}

static void Admit(int count) {
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    for (int i = 0; i < count; ++i)
        assert(CollisionCheck_SetAC(&play, &play.colChkCtx, &cylinders[i].base) == i);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "growth")) {
        Setup(160);
        Admit(160);
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        TimeCtl_Request(TIMECTL_OWNER_PERMAFROST, 0, 1);
        assert(play.colChkCtx.colACCount == 160);
        assert(gTimeSpeed == 0 && gChampionSlowFactor == 0);
        for (int i = 0; i < 160; ++i) {
            assert(play.colChkCtx.colAC[i] == &cylinders[i].base);
            assert(actors[i].freezeTimer == TIMECTL_FREEZE_REFRESH);
        }
        cylinders[159].base.acFlags |= AC_HIT;
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        TimeCtl_Update(&play);
        assert(play.colChkCtx.colACCount == 159 && actors[159].freezeTimer == 0);
        assert(cylinders[159].base.acFlags & AC_HIT);
        TimeCtl_Release(TIMECTL_OWNER_PERMAFROST);
        assert(gTimeSpeed == 17 && gChampionSlowFactor == 1);
        for (int i = 0; i < 160; ++i)
            assert(actors[i].freezeTimer == 0);
        Admit(160);
        for (int i = 0; i < 160; ++i)
            actors[i].freezeTimer = 2;
        actors[0].freezeTimer = 1;
        cylinders[159].base.acFlags |= AC_HIT;
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        TimeCtl_Request(TIMECTL_OWNER_CHAMPION, .33f, 0);
        assert(TimeCtl_GetStutterFrames() == 2);
        assert(play.colChkCtx.colACCount == 158 && actors[159].freezeTimer == 0);
        TimeCtl_Reset(&play);
    } else if (!strcmp(argv[1], "filters")) {
        Setup(3);
        actors[0].category = ACTORCAT_PLAYER;
        actors[1].category = ACTORCAT_ITEMACTION;
        actors[2].parent = &player.actor;
        Admit(3);
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        TimeCtl_Request(TIMECTL_OWNER_PERMAFROST, 0, 1);
        assert(play.colChkCtx.colACCount == 0);
        for (int i = 0; i < 3; ++i)
            assert(actors[i].freezeTimer == 0);
        TimeCtl_Reset(&play);
    } else if (!strcmp(argv[1], "failure")) {
        Setup(65);
        Admit(64);
        sFailCacheGrowth = 1;
        assert(CollisionCheck_SetAC(&play, &play.colChkCtx, &cylinders[64].base) == 64);
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        assert(!TimeCtl_ReapplyAc(&play, &actors[0]));
        assert(play.colChkCtx.colACCount == 1 && play.colChkCtx.colAC[0] == &cylinders[0].base);
        sFailCacheGrowth = 0;
        Admit(65);
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        TimeCtl_Request(TIMECTL_OWNER_PERMAFROST, 0, 1);
        assert(play.colChkCtx.colACCount == 65);
        TimeCtl_Reset(&play);
    } else {
        assert(!strcmp(argv[1], "lifetime"));
        Setup(160);
        Admit(160);
        assert(sOwnedCacheAllocations == 1);
        ++play.sceneNum;
        TimeCtl_Update(&play);
        assert(sOwnedCacheAllocations == 0);
        Admit(160);
        TimeCtl_Request(TIMECTL_OWNER_PERMAFROST, 0, 1);
        TimeCtl_Reset(&play); /* The destruction hook runs before actors retire. */
        assert(sOwnedCacheAllocations == 0 && gTimeSpeed == 17);
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        assert(!TimeCtl_ReapplyAc(&play, &actors[159]) && play.colChkCtx.colACCount == 0);
        TimeCtl_Reset(&play);
    }
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(sOwnedCacheAllocations == 0);
    printf("PASS TimeCtl %s: grown native AC queue, unchanged filters/timing and safe owned-cache lifecycle\n", argv[1]);
}
