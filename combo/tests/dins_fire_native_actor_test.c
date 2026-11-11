/* Real actor functions with engine boundaries supplied by a deterministic scene. */
#include "global.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TEST_HOST_MM
#define CW_DINS_HOST_MM
#define NATIVE_INIT MagicFireDins_Init
#define NATIVE_DESTROY MagicFireDins_Destroy
#define NATIVE_WAIT MagicFireDins_UpdateBeforeCast
#define NATIVE_UPDATE MagicFireDins_Update
#define DIN_ID ACTOR_OOT_DINS_FIRE
#define LIST_HEAD first
#define STICK_TIMER unk_B28
#define ARROW_NORMAL_PARAM ARROW_TYPE_NORMAL
#define ARROW_FIRE_PARAM ARROW_TYPE_FIRE
#define ARROW_LIT_PARAM ARROW_TYPE_NORMAL_LIT
#define ARROW_DAMAGE(arrow) ((arrow).collider.elem.atDmgInfo.dmgFlags)
#define FIRE_DAMAGE(fire) ((fire).collider.elem.atDmgInfo.damage)
#define func_800876C8 Magic_Reset
#define func_8002F7DC Player_PlaySfx
#else
#define NATIVE_INIT MagicFire_Init
#define NATIVE_DESTROY MagicFire_Destroy
#define NATIVE_WAIT MagicFire_UpdateBeforeCast
#define NATIVE_UPDATE MagicFire_Update
#define DIN_ID ACTOR_MAGIC_FIRE
#define LIST_HEAD head
#define STICK_TIMER unk_860
#define ARROW_NORMAL_PARAM ARROW_NORMAL
#define ARROW_FIRE_PARAM ARROW_FIRE
#define ARROW_LIT_PARAM ARROW_NORMAL_LIT
#define ARROW_DAMAGE(arrow) ((arrow).collider.info.toucher.dmgFlags)
#define FIRE_DAMAGE(fire) ((fire).collider.info.toucher.damage)
#endif
#include "combo/render/DinsFireNative.h"
#include "combo/render/DinsFireRemake.h"
#include "combo/audio/DinsFireSfx.h"

#define REQUIRE(c)                                                       \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            exit(1);                                                     \
        }                                                                \
    } while (0)
static PlayState play;
static Player player;
static EnArrow arrow;
static int attacks, kills, resets, lightAdds, lightRemoves, ignitionSounds, fireSounds, colliderDestroys;
static int failLight;
static int customColors;
static u32 customPrimary, customSecondary;
static LightNode light;
/* Audio service boundaries record actual actor dispatch. The production mixer
 * has separate PCM, ownership and thread/pause tests. */
static u32 nextAudioHandle, lastAudioStop;
static int audioAttaches, audioContacts, audioReleases, audioUpdates, audioStops, audioFrame;
static Vec3f audioPosition;
uint32_t DinsFireSfx_Attach(PlayState* state, const Actor* actor) {
    REQUIRE(state == &play && actor != NULL);
    ++audioAttaches;
    return nextAudioHandle++;
}
void DinsFireSfx_Contact(uint32_t handle) {
    if (handle)
        ++audioContacts;
}
void DinsFireSfx_Release(uint32_t handle) {
    if (handle)
        ++audioReleases;
}
void DinsFireSfx_UpdateActor(uint32_t handle, float x, float y, float z, int frame) {
    if (!handle)
        return;
    ++audioUpdates;
    audioFrame = frame;
    audioPosition = (Vec3f){ x, y, z };
}
void DinsFireSfx_Stop(uint32_t handle) {
    if (handle) {
        ++audioStops;
        lastAudioStop = handle;
    }
}
void Flags_SetClearTemp(PlayState* state, s32 flag) {
}

static void alive(Actor* actor, PlayState* state) {
}
void Actor_Kill(Actor* actor) {
    ++kills;
    actor->update = NULL;
    actor->draw = NULL;
}
void Actor_SetScale(Actor* actor, f32 scale) {
    actor->scale.x = actor->scale.y = actor->scale.z = scale;
}
void Actor_ProcessInitChain(Actor* actor, InitChainEntry* chain) {
}
s32 Collider_InitCylinder(PlayState* state, ColliderCylinder* collider) {
    memset(collider, 0, sizeof(*collider));
    return 1;
}
s32 Collider_SetCylinder(PlayState* state, ColliderCylinder* collider, Actor* actor, ColliderCylinderInit* init) {
    collider->base.actor = actor;
    collider->base.atFlags = init->base.atFlags;
    return 1;
}
s32 Collider_DestroyCylinder(PlayState* state, ColliderCylinder* collider) {
    ++colliderDestroys;
    return 1;
}
void Collider_UpdateCylinder(Actor* actor, ColliderCylinder* collider) {
    collider->dim.pos.x = actor->world.pos.x;
    collider->dim.pos.y = actor->world.pos.y;
    collider->dim.pos.z = actor->world.pos.z;
}
s32 CollisionCheck_SetAT(PlayState* state, CollisionCheckContext* context, Collider* collider) {
    ++attacks;
    return 0;
}
void Magic_Reset(PlayState* state) {
    ++resets;
}
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return customColors;
}
Color_RGB8 CVarGetColor24(const char* key, Color_RGB8 fallback) {
    u32 color = strstr(key, "Primary") ? customPrimary : customSecondary;
    return (Color_RGB8){ color >> 16, color >> 8, color };
}
#ifdef TEST_HOST_MM
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    if (!customColors)
        return (Color_RGBA8){ r, g, b, a };
    u32 color = strstr(id, "Primary") ? customPrimary : customSecondary;
    return (Color_RGBA8){ color >> 16, color >> 8, color, a };
}
#endif
#ifdef TEST_HOST_MM
void Player_PlaySfx(Player* actor, u16 sfx) {
#else
void Player_PlaySfx(Actor* actor, u16 sfx) {
#endif
    if (sfx == NA_SE_EV_FLAME_IGNITION)
        ++ignitionSounds;
    if (sfx == NA_SE_PL_MAGIC_FIRE)
        ++fireSounds;
}
LightNode* LightContext_InsertLight(PlayState* state, LightContext* context, LightInfo* info) {
    ++lightAdds;
    light.info = info;
    return failLight ? NULL : &light;
}
void LightContext_RemoveLight(PlayState* state, LightContext* context, LightNode* node) {
    REQUIRE(node == &light);
    ++lightRemoves;
}
#ifdef TEST_HOST_MM
void Lights_PointNoGlowSetInfo(LightInfo* info, s16 x, s16 y, s16 z, u8 r, u8 g, u8 b, s16 radius) {
#else
void Lights_PointNoGlowSetInfo(LightInfo* info, f32 x, f32 y, f32 z, u8 r, u8 g, u8 b, s16 radius) {
#endif
    info->type = LIGHT_POINT_NOGLOW;
    info->params.point.x = x;
    info->params.point.y = y;
    info->params.point.z = z;
    info->params.point.color[0] = r;
    info->params.point.color[1] = g;
    info->params.point.color[2] = b;
    info->params.point.radius = radius;
}
s32 Math_StepToF(f32* value, f32 target, f32 step) {
    if (fabsf(*value - target) <= step) {
        *value = target;
        return 1;
    }
    *value += *value < target ? step : -step;
    return 0;
}
f32 Math_SmoothStepToF(f32* value, f32 target, f32 scale, f32 step, f32 minStep) {
    f32 diff = target - *value, delta = diff * scale;
    delta = fmaxf(-step, fminf(step, delta));
    *value += delta;
    return diff;
}

#include "dins_native_actor_scheduler.inc"
#include "dins_native_actor_production.inc"
static MagicFire fire;

static void reset(int native) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&arrow, 0, sizeof(arrow));
    memset(&fire, 0, sizeof(fire));
    memset(&light, 0, sizeof(light));
    attacks = kills = resets = lightAdds = lightRemoves = ignitionSounds = fireSounds = colliderDestroys = failLight =
        0;
    audioAttaches = audioContacts = audioReleases = audioUpdates = audioStops = 0;
    nextAudioHandle = 1001;
    lastAudioStop = 0;
    audioFrame = -99;
    audioPosition = (Vec3f){ 0, 0, 0 };
    customColors = 0;
    customPrimary = 0xFFC800;
    customSecondary = 0xFF0000;
    Actor_AddToCategory(&play.actorCtx, &player.actor, ACTORCAT_PLAYER);
    Actor_AddToCategory(&play.actorCtx, &fire.actor, ACTORCAT_ITEMACTION);
    player.actor.update = alive;
    player.actor.world.pos = (Vec3f){ 25, 10, -30 };
    player.av1.actionVar1 = 2;
    player.av2.actionVar2 = 1;
    fire.actor.id = DIN_ID;
    fire.actor.params = native ? CW_DINS_NATIVE_CAST_PARAMS : 0;
    fire.actor.update = NATIVE_UPDATE;
    NATIVE_INIT(&fire.actor, &play);
    fire.actor.parent = &player.actor;
#ifdef TEST_HOST_MM
    flushCategories(&play);
#endif
}
static void step(void) {
    REQUIRE(fire.actor.update != NULL);
    fire.actor.update(&fire.actor, &play);
    ++play.gameplayFrames;
}
static void release(void) {
    for (int i = 0; i < CW_DINS_NATIVE_STRIKE_TICKS - 1; ++i)
        step();
    REQUIRE(attacks == 0 && lightAdds == 0 && resets == 0);
    CwDinsNative_Release(&fire.actor);
    step();
    REQUIRE(attacks == 1); /* Red on old actor: it ignores release and waits another 12 updates. */
    REQUIRE(fireSounds == 0 && lightAdds == 1);
    REQUIRE(audioAttaches == 1 && audioReleases == 1 && audioFrame == 0);
}
static void shoot(Vec3f start, Vec3f end, int type) {
    memset(&arrow, 0, sizeof(arrow));
    arrow.actor.id = ACTOR_EN_ARROW;
    arrow.actor.update = alive;
    arrow.actor.params = type;
    arrow.actor.prevPos = start;
    arrow.actor.world.pos = end;
    arrow.collider.base.atFlags = AT_ON | AT_TYPE_PLAYER;
    ARROW_DAMAGE(arrow) = 0x20;
    if (fire.actor.category == ACTORCAT_ITEMACTION) {
        fire.actor.next = &arrow.actor;
    } else {
        play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = &arrow.actor;
    }
}
static void lifetimeAndAnchor(void) {
    reset(1);
    release();
    Vec3f origin = fire.actor.world.pos;
    player.actor.world.pos = (Vec3f){ 600, 80, 700 };
    player.av1.actionVar1 = player.av2.actionVar2 = 0;
    for (int i = 1; i < CW_DINS_NATIVE_FIRE_TICKS; ++i) {
        step();
        REQUIRE(fire.actor.world.pos.x == origin.x && fire.actor.world.pos.y == origin.y &&
                fire.actor.world.pos.z == origin.z);
        REQUIRE(fire.actor.update != NULL && resets == 0 && lightRemoves == 0);
        REQUIRE(fire.collider.dim.radius > 0 && fire.collider.dim.height > 0);
        REQUIRE(audioFrame == i && audioReleases == 1 && fireSounds == 0 && audioStops == 0);
        REQUIRE(audioPosition.x == origin.x && audioPosition.y == origin.y && audioPosition.z == origin.z);
    }
    REQUIRE(attacks == CW_DINS_NATIVE_FIRE_TICKS);
    step();
    REQUIRE(kills == 1 && attacks == CW_DINS_NATIVE_FIRE_TICKS && lightRemoves == 1);
    REQUIRE(audioStops == 1 && lastAudioStop == 1001 && audioFrame == 137);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(resets == 1 && lightRemoves == 1 && colliderDestroys == 1);
    REQUIRE(audioStops == 1);
}
static void stickAndArrow(void) {
    reset(1);
    release();
    for (int i = 1; i <= 9; ++i)
        step();
    player.heldItemAction = PLAYER_IA_DEKU_STICK;
    player.meleeWeaponInfo[0].tip = (Vec3f){ 45, 40, -30 };
    step();
    REQUIRE(player.STICK_TIMER == 210 && ignitionSounds == 1);
    player.STICK_TIMER = 150;
    step();
    REQUIRE(player.STICK_TIMER == 200 && ignitionSounds == 1);
    player.STICK_TIMER = 240;
    step();
    REQUIRE(player.STICK_TIMER == 240);
    player.meleeWeaponInfo[0].tip.y = 1000;
    player.STICK_TIMER = 0;
    step();
    REQUIRE(player.STICK_TIMER == 0);
    shoot((Vec3f){ -300, 55, -30 }, (Vec3f){ 350, 55, -30 }, ARROW_NORMAL_PARAM);
    step();
    REQUIRE(arrow.actor.params == ARROW_LIT_PARAM && ARROW_DAMAGE(arrow) == 0x800);
    shoot((Vec3f){ -300, 1000, -30 }, (Vec3f){ 350, 1000, -30 }, ARROW_NORMAL_PARAM);
    step();
    REQUIRE(arrow.actor.params == ARROW_NORMAL_PARAM && ARROW_DAMAGE(arrow) == 0x20);
    shoot((Vec3f){ -300, 55, 500 }, (Vec3f){ 350, 55, 500 }, ARROW_NORMAL_PARAM);
    step();
    REQUIRE(arrow.actor.params == ARROW_NORMAL_PARAM);
    shoot((Vec3f){ -300, 55, -30 }, (Vec3f){ 350, 55, -30 }, ARROW_FIRE_PARAM);
    step();
    REQUIRE(arrow.actor.params == ARROW_FIRE_PARAM && ARROW_DAMAGE(arrow) == 0x20);
    shoot((Vec3f){ 25, -100, -30 }, (Vec3f){ 25, 300, -30 }, ARROW_NORMAL_PARAM);
    step();
    REQUIRE(arrow.actor.params == ARROW_LIT_PARAM);
    shoot((Vec3f){ 25, 55, -30 }, (Vec3f){ 25, 55, -30 }, ARROW_NORMAL_PARAM);
    arrow.actor.parent = &player.actor;
    step();
    REQUIRE(arrow.actor.params == ARROW_NORMAL_PARAM);
    shoot((Vec3f){ -300, 55, -30 }, (Vec3f){ 350, 55, -30 }, ARROW_NORMAL_PARAM);
    arrow.actor.update = NULL;
    step();
    REQUIRE(arrow.actor.params == ARROW_NORMAL_PARAM);
    shoot((Vec3f){ -300, 55, -30 }, (Vec3f){ 350, 55, -30 }, ARROW_NORMAL_PARAM);
    arrow.collider.base.atFlags = AT_ON | AT_TYPE_ENEMY;
    step();
    REQUIRE(arrow.actor.params == ARROW_NORMAL_PARAM);
}
static void usableLinger(void) {
    /* These are the user-facing contract, independent of the implementation's
     * phase expressions: six seconds at 20 Hz after recovery, then .6 s fade. */
    REQUIRE(CW_DINS_NATIVE_RATE == 20 && CW_DINS_NATIVE_RECOVERY_TICKS == 6);
    REQUIRE(CW_DINS_NATIVE_FADE_START_FRAME == 126 && CW_DINS_NATIVE_FIRE_TICKS == 138);
    REQUIRE(CW_DINS_NATIVE_FADE_START_FRAME - CW_DINS_NATIVE_RECOVERY_TICKS == 120);
    static const struct {
        int frame;
        u8 damage;
    } originalDamage[] = {
        { 0, 55 }, { 1, 52 }, { 7, 29 },  { 8, 25 },  { 20, 16 }, { 39, 2 },
        { 40, 1 }, { 52, 1 }, { 100, 1 }, { 125, 1 }, { 137, 1 },
    };
    reset(1);
    release();
    int damageSample = 0, fullRadius = 0;
    for (int frame = 0; frame < 138; ++frame) {
        if (frame > 0)
            step();
        REQUIRE(fire.nativeFrame == frame && fire.actor.update != NULL);
        if (damageSample < (int)(sizeof(originalDamage) / sizeof(originalDamage[0])) &&
            frame == originalDamage[damageSample].frame) {
            REQUIRE(FIRE_DAMAGE(fire) == originalDamage[damageSample].damage);
            ++damageSample;
        }
        if (frame == 8)
            fullRadius = fire.collider.dim.radius;
        if (frame >= 8 && frame < 126) {
            REQUIRE(fire.action == 2 && fire.actionTimer == 126 - frame);
            REQUIRE(fire.alphaMultiplier == 1.0f && fire.collider.dim.radius == fullRadius);
            REQUIRE(light.info->params.point.radius == 175 && light.info->params.point.color[0] > 0);
            REQUIRE(lightAdds == 1 && lightRemoves == 0 && resets == 0);
        }
        if (frame == 126) {
            REQUIRE(fire.action == 3 && fire.actionTimer == 12 && fire.alphaMultiplier == 1.0f);
        }
        if (frame > 126) {
            REQUIRE(fire.action == 3 && fire.actionTimer == 138 - frame);
            REQUIRE(fire.alphaMultiplier > 0 && fire.alphaMultiplier < 1.0f);
            REQUIRE(light.info->params.point.radius > 0 && light.info->params.point.radius < 175);
        }
    }
    REQUIRE(damageSample == (int)(sizeof(originalDamage) / sizeof(originalDamage[0])));
    step();
    REQUIRE(kills == 1 && attacks == 138 && lightRemoves == 1);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(resets == 1 && lightAdds == 1 && colliderDestroys == 1);

    /* Equip and shoot late in the added utility window, well past the old
     * 52-update lifetime. Ignition stops on the expiration update itself. */
    reset(1);
    release();
    for (int frame = 1; frame < 100; ++frame)
        step();
    player.heldItemAction = PLAYER_IA_DEKU_STICK;
    player.meleeWeaponInfo[0].tip = (Vec3f){ 45, 40, -30 };
    shoot((Vec3f){ -300, 55, -30 }, (Vec3f){ 350, 55, -30 }, ARROW_NORMAL_PARAM);
    step();
    REQUIRE(fire.nativeFrame == 100 && player.STICK_TIMER == 210 && ignitionSounds == 1);
    REQUIRE(arrow.actor.params == ARROW_LIT_PARAM && ARROW_DAMAGE(arrow) == 0x800);
    for (int frame = 101; frame < 138; ++frame)
        step();
    player.STICK_TIMER = 0;
    shoot((Vec3f){ -300, 55, -30 }, (Vec3f){ 350, 55, -30 }, ARROW_NORMAL_PARAM);
    step();
    REQUIRE(kills == 1 && attacks == 138 && player.STICK_TIMER == 0);
    REQUIRE(arrow.actor.params == ARROW_NORMAL_PARAM && ignitionSounds == 1);
}
static void cancellationsAndFallback(void) {
    reset(1);
    player.av1.actionVar1 = 0;
    step();
    REQUIRE(kills == 1 && attacks == 0);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(resets == 1 && lightRemoves == 0);
    reset(1);
    fire.actor.parent = NULL;
    step();
    REQUIRE(kills == 1);
    reset(1);
    for (int i = 0; i < CW_DINS_NATIVE_PENDING_LIMIT && fire.actor.update != NULL; ++i)
        step();
    REQUIRE(kills == 1 && attacks == 0);
    reset(1);
    failLight = 1;
    release();
    for (int i = 1; i < CW_DINS_NATIVE_FIRE_TICKS; ++i)
        step();
    step();
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(resets == 1 && lightRemoves == 0 && lightAdds == 1);
    reset(0);
    step();
    REQUIRE(fire.actionTimer == 19 && attacks == 0 && lightAdds == 0);
#ifdef TEST_HOST_MM
    reset(1);
    fire.actor.id = ACTOR_SW97_MAGIC_FIRE;
    NATIVE_INIT(&fire.actor, &play);
    step();
    REQUIRE(fire.actionTimer == 19 && attacks == 0 && lightAdds == 0);
#endif
}
static void contactLight(void) {
    reset(1);
    step();
    CwDinsNative_Contact(&fire.actor);
    step();
    REQUIRE(attacks == 0 && fireSounds == 0 && lightAdds == 1);
    REQUIRE(audioContacts == 1 && audioReleases == 0 && audioFrame == -1);
    REQUIRE(light.info->params.point.radius == 175);
    REQUIRE(light.info->params.point.color[0] == 183);
    CwDinsNative_Release(&fire.actor);
    step();
    REQUIRE(attacks == 1 && fireSounds == 0 && lightAdds == 1);
    REQUIRE(audioContacts == 1 && audioReleases == 1);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(lightRemoves == 1 && resets == 1);
    reset(1);
    CwDinsNative_Contact(&fire.actor);
    step();
    player.av1.actionVar1 = 0;
    step();
    REQUIRE(kills == 1 && attacks == 0 && lightRemoves == 1);
    REQUIRE(audioStops == 1 && audioContacts == 1 && audioReleases == 0);
}
static void audioOwnershipAndLegacy(void) {
    /* Direct scene destruction must stop a pending or released owner even
     * when the helper's Kill path was not invoked. */
    reset(1);
    REQUIRE(audioAttaches == 1);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(audioStops == 1 && lastAudioStop == 1001 && audioReleases == 0);
    reset(1);
    release();
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(audioStops == 1 && lastAudioStop == 1001);

    /* A late destroy forwards its own saved generation, not a current global
     * handle. The mixer verifies that stale generations cannot stop a new one. */
    reset(1);
    MagicFire earlier = fire;
    NATIVE_INIT(&fire.actor, &play);
    REQUIRE(audioAttaches == 2);
    NATIVE_DESTROY(&earlier.actor, &play);
    REQUIRE(lastAudioStop == 1001);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(lastAudioStop == 1002);

    reset(0);
    for (int i = 0; i < 21; ++i)
        step();
    REQUIRE(fireSounds == 1 && audioAttaches == 0 && audioReleases == 0 && audioUpdates == 0);
    NATIVE_DESTROY(&fire.actor, &play);
    REQUIRE(audioStops == 0);
}
static void moveArrow(Actor* actor, PlayState* state) {
    actor->prevPos = actor->world.pos;
    actor->world.pos = (Vec3f){ 350, 55, -30 };
}
static void nativeScheduling(void) {
    reset(1);
    release();
    for (int i = 1; i <= 9; ++i)
        step();
    REQUIRE(CwDinsNative_FindOwner(&play, &player.actor, DIN_ID) == &fire.actor);
    memset(&arrow, 0, sizeof(arrow));
    arrow.actor.id = ACTOR_EN_ARROW;
    arrow.actor.params = ARROW_NORMAL_PARAM;
    arrow.actor.update = moveArrow;
    arrow.actor.prevPos = arrow.actor.world.pos = (Vec3f){ -300, 55, -30 };
    arrow.collider.base.atFlags = AT_ON | AT_TYPE_PLAYER;
    ARROW_DAMAGE(arrow) = 0x20;
    Actor_AddToCategory(&play.actorCtx, &arrow.actor, ACTORCAT_ITEMACTION);
    /* Use each real host's list insertion, then its native category order. */
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (Actor* actor = play.actorCtx.actorLists[category].LIST_HEAD; actor; actor = actor->next)
            if (actor->update != NULL)
                actor->update(actor, &play);
    }
    /* Collision resolution follows the update pass: the first crossing must
     * already be lit before a hit flag can suppress a later frame's scan. */
    REQUIRE(arrow.actor.params == ARROW_LIT_PARAM && ARROW_DAMAGE(arrow) == 0x800);
}
static void lightFollowsNativeCosmetics(void) {
    reset(1);
    release();
    for (int i = 1; i <= 9; ++i)
        step();
    customColors = 1;
    customPrimary = 0x0000FF;
    customSecondary = 0x000080;
    step();
    REQUIRE(light.info->params.point.color[0] == 0 && light.info->params.point.color[1] == 0);
    REQUIRE(light.info->params.point.color[2] > 0);
    customSecondary = 0x00FF00;
    step();
    REQUIRE(light.info->params.point.color[0] == 0 && light.info->params.point.color[1] > 0);
    REQUIRE(light.info->params.point.color[2] > 0);
    customColors = 0;
    step();
    REQUIRE(light.info->params.point.color[0] > light.info->params.point.color[1]);
    REQUIRE(light.info->params.point.color[1] > light.info->params.point.color[2]);
    reset(1);
    customColors = 1;
    customPrimary = 0x0000FF;
    customSecondary = 0x000080;
    CwDinsNative_Contact(&fire.actor);
    step();
    REQUIRE(light.info->params.point.color[0] == 0 && light.info->params.point.color[1] == 0);
    REQUIRE(light.info->params.point.color[2] > 0 && attacks == 0);
}
int main(void) {
    lifetimeAndAnchor();
    stickAndArrow();
    cancellationsAndFallback();
    contactLight();
    nativeScheduling();
    lightFollowsNativeCosmetics();
    usableLinger();
    audioOwnershipAndLegacy();
    puts("PASS native Din actor: release, 138-update anchored fire, six usable seconds, original damage falloff, late "
         "stick/arrow ignition, magic/light cleanup, cancellation and legacy guards");
    puts("PASS native Din audio routing: authored contact/release, anchored age publication, generation-qualified "
         "kill/direct destroy, unchanged stock release and stick ignition");
    return 0;
}
