/* Native Din cast/actor ownership. Include after the host's actor declarations. */
#ifndef COMBO_DINS_FIRE_NATIVE_H
#define COMBO_DINS_FIRE_NATIVE_H

#include "DinsFireTiming.h"

#define CW_DINS_NATIVE_CAST_PARAMS 0x4D00
#define CW_DINS_NATIVE_CAST_MASK 0xFF00
#define CW_DINS_NATIVE_RELEASED 0x0001
#define CW_DINS_NATIVE_CONTACTED 0x0002

#define CW_DINS_NATIVE_PENDING_LIMIT 40

static inline int CwDinsNative_Is(const Actor* actor) {
    return actor != NULL && (((unsigned short)actor->params & CW_DINS_NATIVE_CAST_MASK) == CW_DINS_NATIVE_CAST_PARAMS);
}

static inline Actor* CwDinsNative_Find(PlayState* play, Actor* playerActor, s16 actorId, int liveOnly) {
#ifdef CW_DINS_HOST_MM
    /* Native MM fire moves after projectiles so first-frame crossings ignite
     * before collision resolution. Also cover its deferred category migration. */
    const u8 categories[] = { ACTORCAT_ITEMACTION, ACTORCAT_MISC };
#else
    const u8 categories[] = { ACTORCAT_ITEMACTION };
#endif
    for (unsigned int category = 0; category < sizeof(categories) / sizeof(categories[0]); ++category) {
#ifdef CW_DINS_HOST_MM
        Actor* actor = play->actorCtx.actorLists[categories[category]].first;
#else
        Actor* actor = play->actorCtx.actorLists[categories[category]].head;
#endif
        for (; actor != NULL; actor = actor->next) {
            if (actor->id == actorId && actor->parent == playerActor && (!liveOnly || actor->update != NULL) &&
                CwDinsNative_Is(actor)) {
                return actor;
            }
        }
    }
    return NULL;
}

/* Resolve each time: an action never retains a pointer after the actor may die. */
static inline Actor* CwDinsNative_FindOwner(PlayState* play, Actor* playerActor, s16 actorId) {
    return CwDinsNative_Find(play, playerActor, actorId, 1);
}

/* Killed actors retain the magic reservation until Destroy unlinks them. A
 * stale-magic recovery must not arm a new spell in that one-frame interval. */
static inline int CwDinsNative_HasMagicOwner(PlayState* play, Actor* playerActor, s16 actorId) {
    return CwDinsNative_Find(play, playerActor, actorId, 0) != NULL;
}

static inline void CwDinsNative_Release(Actor* actor) {
    if (CwDinsNative_Is(actor) && actor->update != NULL) {
        actor->params |= CW_DINS_NATIVE_RELEASED;
    }
}

static inline void CwDinsNative_Contact(Actor* actor) {
    if (CwDinsNative_Is(actor) && actor->update != NULL) {
        actor->params |= CW_DINS_NATIVE_CONTACTED;
    }
}

#endif
