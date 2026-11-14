/* Shared native actor behavior. Include after MagicFire and the host declarations. */
#ifndef COMBO_DINS_FIRE_NATIVE_ACTOR_H
#define COMBO_DINS_FIRE_NATIVE_ACTOR_H

#include "DinsFireNative.h"
#include "DinsFireRemake.h"
#include "DinsFireColors.h"
#include "../audio/DinsFireSfx.h"

static inline void CwDinsNativeActor_Init(MagicFire* fire, PlayState* play) {
#ifdef CW_DINS_HOST_MM
    fire->nativeActive = fire->actor.id == ACTOR_OOT_DINS_FIRE && CwDinsNative_Is(&fire->actor);
    if (fire->nativeActive) {
        /* MM appends new projectiles; run lingering fire after their movement.
         * The engine safely migrates the list at the end of its update pass. */
        Actor_ChangeCategory(play, &play->actorCtx, &fire->actor, ACTORCAT_MISC);
    }
#else
    fire->nativeActive = fire->actor.id == ACTOR_MAGIC_FIRE && CwDinsNative_Is(&fire->actor);
#endif
    fire->nativeFrame = -1;
    fire->nativePendingTicks = 0;
    fire->nativeLightAttempted = 0;
    fire->nativeLightNode = NULL;
    fire->nativeAudioHandle = fire->nativeActive ? DinsFireSfx_Attach(play, &fire->actor) : 0;
}

static inline void CwDinsNativeActor_StopAudio(MagicFire* fire) {
    DinsFireSfx_Stop(fire->nativeAudioHandle);
    fire->nativeAudioHandle = 0;
}

static inline void CwDinsNativeActor_RemoveLight(MagicFire* fire, PlayState* play) {
    if (fire->nativeLightNode != NULL) {
        LightContext_RemoveLight(play, &play->lightCtx, fire->nativeLightNode);
        fire->nativeLightNode = NULL;
    }
}

static inline void CwDinsNativeActor_Kill(MagicFire* fire, PlayState* play) {
    CwDinsNativeActor_StopAudio(fire);
    CwDinsNativeActor_RemoveLight(fire, play);
    fire->action = 0;
    fire->alphaMultiplier = 0;
    Actor_Kill(&fire->actor);
}

/* Clip the swept projectile to the finite fire cylinder before testing X/Z.
 * A fast arrow may cross the entire flame between two native updates. */
static inline int CwDinsNativeActor_Intersects(const Vec3f* origin, float radius, float height, const Vec3f* start,
                                               const Vec3f* end) {
    float lower = 0, upper = 1;
    float dy = end->y - start->y;
    if (fabsf(dy) < .00001f) {
        if (start->y < origin->y || start->y > origin->y + height)
            return 0;
    } else {
        float a = (origin->y - start->y) / dy;
        float b = (origin->y + height - start->y) / dy;
        lower = fmaxf(lower, fminf(a, b));
        upper = fminf(upper, fmaxf(a, b));
        if (lower > upper)
            return 0;
    }
    float dx = end->x - start->x, dz = end->z - start->z;
    float x = start->x - origin->x, z = start->z - origin->z;
    float distanceSquared = dx * dx + dz * dz;
    float t = distanceSquared > .00001f ? -(x * dx + z * dz) / distanceSquared : lower;
    t = fmaxf(lower, fminf(upper, t));
    x += t * dx;
    z += t * dz;
    return x * x + z * z <= radius * radius;
}

static inline void CwDinsNativeActor_PlaySfx(Player* player, u16 sound) {
#ifdef CW_DINS_HOST_MM
    Player_PlaySfx(player, sound);
#else
    Player_PlaySfx(&player->actor, sound);
#endif
}

static inline void CwDinsNativeActor_Ignite(MagicFire* fire, PlayState* play, Player* player, float radius,
                                            float height) {
    Vec3f* origin = &fire->actor.world.pos;
    Vec3f* tip = &player->meleeWeaponInfo[0].tip;
    if (player->heldItemAction == PLAYER_IA_DEKU_STICK &&
        CwDinsNativeActor_Intersects(origin, radius, height, tip, tip)) {
#ifdef CW_DINS_HOST_MM
        s16* timer = &player->unk_B28;
#else
        s16* timer = &player->unk_860;
#endif
        /* Exactly the native lit-torch stick duration and refresh behavior. */
        if (*timer == 0) {
            *timer = 210;
            CwDinsNativeActor_PlaySfx(player, NA_SE_EV_FLAME_IGNITION);
        } else if (*timer < 200) {
            *timer = 200;
        }
    }
#ifdef CW_DINS_HOST_MM
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
#else
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head;
#endif
    for (; actor != NULL; actor = actor->next) {
        if (actor->id != ACTOR_EN_ARROW || actor->update == NULL || actor->parent != NULL)
            continue;
        EnArrow* arrow = (EnArrow*)actor;
#ifdef CW_DINS_HOST_MM
        int normal = actor->params == ARROW_TYPE_NORMAL || actor->params == ARROW_TYPE_NORMAL_HORSE;
        int embedded = arrow->unk_262;
#else
        int normal = actor->params == ARROW_NORMAL || actor->params == ARROW_NORMAL_HORSE;
        int embedded = arrow->touchedPoly;
#endif
        float dx = actor->world.pos.x - actor->prevPos.x;
        float dy = actor->world.pos.y - actor->prevPos.y;
        float dz = actor->world.pos.z - actor->prevPos.z;
        if (!normal || embedded || !(arrow->collider.base.atFlags & AT_TYPE_PLAYER) ||
            !(arrow->collider.base.atFlags & AT_ON) || (arrow->collider.base.atFlags & AT_HIT) ||
            dx * dx + dy * dy + dz * dz < .00001f)
            continue;
        if (CwDinsNativeActor_Intersects(origin, radius, height, &actor->prevPos, &actor->world.pos)) {
            /* The existing normal-arrow-with-fire path, with no magic charge. */
#ifdef CW_DINS_HOST_MM
            actor->params = ARROW_TYPE_NORMAL_LIT;
            arrow->collider.elem.atDmgInfo.dmgFlags = DMG_FIRE_ARROW;
#else
            actor->params = ARROW_NORMAL_LIT;
            arrow->collider.info.toucher.dmgFlags = 0x800;
#endif
        }
    }
}

static inline void CwDinsNativeActor_SetLight(MagicFire* fire, float x, float y, float z, float strength, s16 radius) {
    uint32_t primary, secondary;
    CwDinsFire_GetColors(&primary, &secondary);
    uint32_t color = CwDinsFire_LocalLightColor(primary, secondary);
    Lights_PointNoGlowSetInfo(&fire->nativeLightInfo, x, y, z, (u8)(((color >> 16) & 255) * strength),
                              (u8)(((color >> 8) & 255) * strength), (u8)((color & 255) * strength), radius);
}

static inline void CwDinsNativeActor_UpdateLight(MagicFire* fire) {
    float age = fire->nativeFrame / (float)CW_DINS_NATIVE_RATE;
    float growth = CwDins_Smooth(age / (CW_DINS_NATIVE_GROW_TICKS / (float)CW_DINS_NATIVE_RATE));
    float tail =
        1 - CwDins_Smooth((age - CW_DINS_NATIVE_FADE_AGE) / (CW_DINS_NATIVE_FADE_TICKS / (float)CW_DINS_NATIVE_RATE));
    float strength =
        (.72f * expf(-(age + .05f) / .13f) + .4f * growth *
                                                 (1 + .055f * sinf((age + CW_DINS_NATIVE_FIRE_START_TIME) * 17) +
                                                  .025f * sinf((age + CW_DINS_NATIVE_FIRE_START_TIME) * 29))) *
        tail;
    strength = CwDins_Clamp(strength, 0, 1);
    CwDinsNativeActor_SetLight(fire, fire->actor.world.pos.x + 14 * (1 - growth),
                               fire->actor.world.pos.y + 20 + 12 * growth, fire->actor.world.pos.z + 15 * (1 - growth),
                               strength, (s16)(175 * tail));
}

/* Returns true for the latched native cast, including wait/expiry. Legacy actors
 * never enter this path, even if the setting changes while they are alive. */
static inline int CwDinsNativeActor_Update(MagicFire* fire, PlayState* play) {
    if (!fire->nativeActive)
        return 0;
    Player* player = GET_PLAYER(play);
    if (player == NULL || player->actor.update == NULL || play->msgCtx.msgMode == 0xD || play->msgCtx.msgMode == 0x11) {
        CwDinsNativeActor_Kill(fire, play);
        return 1;
    }
    if (fire->nativeFrame < 0) {
        if (fire->actor.parent != &player->actor) {
            CwDinsNativeActor_Kill(fire, play);
            return 1;
        }
        if (!(fire->actor.params & CW_DINS_NATIVE_RELEASED)) {
            if (player->av1.actionVar1 != 2 || player->av2.actionVar2 != 1 ||
                ++fire->nativePendingTicks >= CW_DINS_NATIVE_PENDING_LIMIT) {
                CwDinsNativeActor_Kill(fire, play);
            } else {
                const Vec3f* position = fire->nativeLightAttempted ? &fire->actor.world.pos : &player->actor.world.pos;
                DinsFireSfx_UpdateActor(fire->nativeAudioHandle, position->x, position->y, position->z, -1);
                if ((fire->actor.params & CW_DINS_NATIVE_CONTACTED) && !fire->nativeLightAttempted) {
                    fire->actor.world.pos = fire->actor.home.pos = player->actor.world.pos;
                    CwDinsNativeActor_SetLight(fire, fire->actor.world.pos.x + 14, fire->actor.world.pos.y + 20,
                                               fire->actor.world.pos.z + 15, .72f, 175);
                    fire->nativeLightAttempted = 1;
                    fire->nativeLightNode = LightContext_InsertLight(play, &play->lightCtx, &fire->nativeLightInfo);
                    DinsFireSfx_Contact(fire->nativeAudioHandle);
                }
            }
            return 1;
        }
        /* Resolve the cast point once. Link may move freely after recovery. */
        if (!fire->nativeLightAttempted)
            fire->actor.world.pos = fire->actor.home.pos = player->actor.world.pos;
        fire->actor.world.rot = (Vec3s){ 0, 0, 0 };
        fire->actor.shape.rot = (Vec3s){ 0, 0, 0 };
        fire->nativeFrame = 0;
        CwDinsNativeActor_UpdateLight(fire);
        if (!fire->nativeLightAttempted) {
            fire->nativeLightAttempted = 1;
            fire->nativeLightNode = LightContext_InsertLight(play, &play->lightCtx, &fire->nativeLightInfo);
        }
    } else if (++fire->nativeFrame >= CW_DINS_NATIVE_FIRE_TICKS) {
        CwDinsNativeActor_Kill(fire, play);
        return 1;
    }

    int frame = fire->nativeFrame;
    float age = frame / (float)CW_DINS_NATIVE_RATE;
    float radius = CwDins_NativeRadius(age);
    float height = radius * 1.10f;
    float fade = CwDins_Clamp((frame - CW_DINS_NATIVE_FADE_START_FRAME) / (float)CW_DINS_NATIVE_FADE_TICKS, 0, 1);
    fire->action = frame < CW_DINS_NATIVE_GROW_TICKS ? 1 : frame < CW_DINS_NATIVE_FADE_START_FRAME ? 2 : 3;
    fire->actionTimer = frame < CW_DINS_NATIVE_GROW_TICKS         ? CW_DINS_NATIVE_GROW_TICKS - frame
                        : frame < CW_DINS_NATIVE_FADE_START_FRAME ? CW_DINS_NATIVE_FADE_START_FRAME - frame
                                                                  : CW_DINS_NATIVE_FIRE_TICKS - frame;
    fire->alphaMultiplier = powf(1 - fade, .65f);
    fire->screenTintIntensity = 0;
    Actor_SetScale(&fire->actor, radius / 325.0f);
    fire->collider.dim.radius = (s16)radius;
    fire->collider.dim.height = (s16)height;
    fire->collider.dim.yShift = 0;
    Collider_UpdateCylinder(&fire->actor, &fire->collider);
    /* Keep the original strong damage falloff. The extra interaction hold uses
     * the existing low-damage tail instead of prolonging the strong attack. */
    u8 damage = frame < CW_DINS_NATIVE_GROW_TICKS ? (u8)(55 - frame * 30 / CW_DINS_NATIVE_GROW_TICKS)
                : frame < CW_DINS_NATIVE_GROW_TICKS + CW_DINS_NATIVE_DAMAGE_HOLD_TICKS
                    ? (u8)(25 - (frame - CW_DINS_NATIVE_GROW_TICKS) * 24 / CW_DINS_NATIVE_DAMAGE_HOLD_TICKS)
                    : 1;
#ifdef CW_DINS_HOST_MM
    fire->collider.elem.atDmgInfo.damage = damage;
#else
    fire->collider.info.toucher.damage = damage;
#endif
    CollisionCheck_SetAT(play, &play->colChkCtx, &fire->collider.base);
    CwDinsNativeActor_Ignite(fire, play, player, radius, height);
    CwDinsNativeActor_UpdateLight(fire);
    DinsFireSfx_UpdateActor(fire->nativeAudioHandle, fire->actor.world.pos.x, fire->actor.world.pos.y,
                            fire->actor.world.pos.z, frame);
    if (frame == 0)
        DinsFireSfx_Release(fire->nativeAudioHandle);
    return 1;
}

#endif
