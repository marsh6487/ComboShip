/* Production action/setup/ownership regression fixture. Animation resource
 * frames are controlled fixtures; the real host frame advance is exercised. */
#include "global.h"
#include "soh/ResourceManagerHelpers.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TEST_HOST_MM
#include "2s2h/BenGui/CosmeticEditor.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "mods/nei_oot_compat.h"
#include "mods/nei_save.h"
#define CW_DINS_HOST_MM
#define ANIM_HEADER PlayerAnimationHeader
#define ANIM_UPDATE PlayerAnimation_Update
#define ANIM_ONCE PlayerAnimation_Once
#define ANIM_LOOP PlayerAnimation_Loop
#define CAST_ACTION Player_Action_OotMagicSpell
#define SETUP_CAST OotSpells_SetupCast
#define MEDALLION_LATCH sOotSw97SpellActive
#define CAST_FLAGS (PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000)
#define DIN_ACTOR ACTOR_OOT_DINS_FIRE
#define LIST_HEAD first
#define SPAWN_PARAMS s32
#else
#include "objects/gameplay_keep/gameplay_keep.h"
#define ANIM_HEADER LinkAnimationHeader
#define ANIM_UPDATE LinkAnimation_Update
#define ANIM_ONCE LinkAnimation_Once
#define ANIM_LOOP LinkAnimation_Loop
#define CAST_ACTION Player_Action_808507F4
#define SETUP_CAST func_8083AF44
#define MEDALLION_LATCH sSw97SpellActive
#define CAST_FLAGS (PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE)
#define DIN_ACTOR ACTOR_MAGIC_FIRE
#define LIST_HEAD head
#define SPAWN_PARAMS s16
#endif
#include "combo/render/DinsFireRemakeDraw.h"
#include "combo/render/DinsFireNative.h"
#include "combo/audio/DinsFireSfx.h"
#include "dins_gameplay_sfx_types.inc"

#define REQUIRE(c)                                                       \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            exit(1);                                                     \
        }                                                                \
    } while (0)
#define MAGIC_REQ(cost) (cost)

SaveContext gSaveContext;
static PlayState play;
static Player player;
static Actor spawned;
static Camera camera;
static int option, missingResource, failSpawn, cameraStarts, cameraEnds, idleCount, spawnCount, magicResets;
static int errors, spent, testTick, spawnTick, contactedTick, releasedTick, normalTick, chargeDraws, subCameraEnds;
static int missingAnimation, rejectMagic;
static int chargeFrames, raiseFrames, recoveryFrames;
static int generalSkip, generalSwing, voiceSword, voiceMagic, floorCues;
static int audioBegins, audioCancels, audioCancelCalls, audioReleased, audioLive;
static uint32_t lastAudioHandle;
static float chargePhase;

uint32_t DinsFireSfx_Begin(PlayState* state, float x, float y, float z) {
    REQUIRE(state == &play && spent == 12 && gSaveContext.magicState == MAGIC_STATE_METER_FLASH_2);
    REQUIRE(x == player.actor.world.pos.x && y == player.actor.world.pos.y && z == player.actor.world.pos.z);
    ++audioBegins;
    audioLive = 1;
    return lastAudioHandle = 100 + audioBegins;
}
void DinsFireSfx_CancelCast(uint32_t handle) {
    if (!handle)
        return;
    REQUIRE(handle == lastAudioHandle);
    ++audioCancelCalls;
    if (!audioReleased) {
        ++audioCancels;
        audioLive = 0;
    }
}

static void recordGeneral(u16 sfx) {
    if (sfx == NA_SE_PL_SKIP)
        ++generalSkip;
    if (sfx == NA_SE_IT_SWORD_SWING_HARD)
        ++generalSwing;
}
static void recordVoice(Player* this, u16 sfx) {
    REQUIRE(this == &player);
    if (sfx == NA_SE_VO_LI_SWORD_N)
        ++voiceSword;
    if (sfx == NA_SE_VO_LI_MAGIC_ATTACK)
        ++voiceMagic;
}
#ifdef TEST_HOST_MM
void Player_PlaySfx(Player* this, u16 sfx) {
    recordGeneral(sfx);
}
void Player_AnimSfx_PlayVoice(Player* this, u16 sfx) {
    recordVoice(this, sfx);
}
void Player_AnimSfx_PlayFloor(Player* this, u16 sfx) {
    REQUIRE(sfx == NA_SE_PL_WALK_GROUND);
    ++floorCues;
}
void Player_AnimSfx_PlayFloorByAge(Player* this, u16 sfx) {
    ++floorCues;
}
void Player_AnimSfx_PlayFloorLand(Player* this) {
    ++floorCues;
}
void Player_AnimSfx_PlayFloorWalk(Player* this, f32 scale) {
    ++floorCues;
}
void Player_AnimSfx_PlayFloorJump(Player* this) {
    ++floorCues;
}
void Audio_PlaySfx_AtPosForMetalEffectsWithSyncedFreqAndVolume(Vec3f* pos, u16 sfx, f32 scale) {
    ++floorCues;
}
#else
u8 TransformMasks_IsTransformed(void) {
    return 0;
}
void Player_PlaySfx(Actor* this, u16 sfx) {
    recordGeneral(sfx);
}
void Player_PlayVoiceSfx(Player* this, u16 sfx) {
    recordVoice(this, sfx);
}
void Player_PlayFloorSfx(Player* this, u16 sfx) {
    ++floorCues;
}
void Player_PlayFloorSfxByAge(Player* this, u16 sfx) {
    ++floorCues;
}
void Player_PlayLandingSfx(Player* this) {
    ++floorCues;
}
void Player_PlaySteppingSfx(Player* this, f32 scale) {
    REQUIRE(scale == 6.0f || scale == 0.0f);
    ++floorCues;
}
void Player_PlayJumpingSfx(Player* this) {
    ++floorCues;
}
void func_800F4010(Vec3f* pos, u16 sfx, f32 scale) {
    ++floorCues;
}
#endif
#include "dins_gameplay_sfx_dispatch.inc"
static char resourceBytes[64];
static const void* lastAnimation;
#ifdef TEST_HOST_MM
static NeiSaveData nei;
f32 gFramerateDivisorHalf = 1.5f;
static ANIM_HEADER fixtureAnimations[10];
typedef struct OotSpellsAnimSfx {
    u16 sfxId;
    s16 flags;
} OotSpellsAnimSfx;
#else
static GameInfo testGameInfo;
GameInfo* gGameInfo = &testGameInfo;
#endif

void CAST_ACTION(Player* this, PlayState* state);
static void idleAction(Player* this, PlayState* state) {
}
#define Player_Action_Idle idleAction
static void liveActorUpdate(Actor* this, PlayState* state) {
}
static Actor* Sw97_TrySpawnMagicSpell(PlayState* state, Player* this, s32 spell);
static s32 Sw97_IsMedallionItem(s32 item) {
    return item == 1000;
}
s32 Player_DecelerateToZero(Player* this) {
    return 0;
}
void ComboFw_PublishPoint(void) {
}
int ComboFw_HasPoint(int native) {
    return native;
}

int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return !strcmp(key, "gEnhancements.DinsFireRemake") ? option : fallback;
}
Color_RGB8 CVarGetColor24(const char* key, Color_RGB8 fallback) {
    return fallback;
}
void* ResourceGetDataByName(const char* path) {
    return missingResource ? NULL : resourceBytes;
}
void Magic_Reset(PlayState* state) {
    ++magicResets;
    gSaveContext.magicState = MAGIC_STATE_IDLE;
}
Camera* Play_GetCamera(PlayState* state, s16 cameraId) {
    return &camera;
}
void Play_SetupRespawnPoint(PlayState* state, s32 mode, s32 params) {
}

#ifdef TEST_HOST_MM
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    return (Color_RGBA8){ r, g, b, a };
}
NeiSaveData* Nei_Save(void) {
    return &nei;
}
s16 Play_GetOriginalSceneId(s16 sceneId) {
    return sceneId;
}
void OotFw_NotifyPointSet(PlayState* state) {
}
void Audio_PlaySfx(u16 sfx) {
    ++errors;
}
s16 Camera_SetFinishedFlag(Camera* cam) {
    ++cameraEnds;
    return 1;
}
s32 Player_SetAction(PlayState* state, Player* this, PlayerActionFunc action, s32 flags);
void Player_SetAction_PreserveItemAction(PlayState* state, Player* this, PlayerActionFunc action, s32 flags);
void func_80836988(Player* this, PlayState* state);
static void Player_Action_96(Player* this, PlayState* state) {
}
static void Player_Action_93(Player* this, PlayState* state) {
}
void AudioOcarina_SetInstrument(u8 instrument) {
}
void ActorShadow_DrawFeet(Actor* actor, Lights* lights, PlayState* state) {
}
void func_8082DD2C(PlayState* state, Player* this) {
}
void Actor_SetScale(Actor* actor, f32 scale) {
    actor->scale.x = actor->scale.y = actor->scale.z = scale;
}
PlayerMeleeWeapon Player_GetMeleeWeaponHeld(Player* this) {
    return PLAYER_MELEEWEAPON_NONE;
}
void Player_UseItem(PlayState* state, Player* this, ItemId item) {
}
void* Effect_GetByIndex(s32 index) {
    return NULL;
}
void func_800AEF44(EffectTireMark* effect) {
}
void func_808309CC(PlayState* state, Player* this) {
}
PlayerAnimationHeader* Player_GetIdleAnim(Player* this) {
    return &fixtureAnimations[0];
}
void PlayerAnimation_PlayLoop(PlayState* state, SkelAnime* skel, PlayerAnimationHeader* animation) {
}
void Player_Anim_PlayOnceMorph(PlayState* state, Player* this, PlayerAnimationHeader* animation) {
}
void func_80831454(Player* this) {
}
void Player_Anim_ResetMove(Player* this) {
}
void Player_ResetCylinder(Player* this) {
}
void func_8082E00C(Player* this) {
    if (this->actionFunc == idleAction)
        ++idleCount;
}
bool Player_CheckHostileLockOn(Player* this) {
    return false;
}
bool Player_FriendlyLockOnOrParallel(Player* this) {
    return false;
}
void func_80836888(Player* this, PlayState* state) {
    Player_SetAction(state, this, idleAction, 1);
}
void func_8083692C(Player* this, PlayState* state) {
    Player_SetAction(state, this, idleAction, 1);
}
PlayerModelGroup Player_ActionToModelGroup(Player* this, PlayerItemAction action) {
    return 0;
}
void Player_SetModels(Player* this, PlayerModelGroup group) {
}
void Player_SetModelGroup(Player* this, PlayerModelGroup group) {
}
s32 Magic_Consume(PlayState* state, s16 amount, s16 type) {
    if (rejectMagic)
        return 0;
    REQUIRE(type == MAGIC_CONSUME_WAIT_PREVIEW);
    spent = amount;
    gSaveContext.magicState = MAGIC_STATE_METER_FLASH_2;
    return 1;
}
u8 ResourceMgr_FileExists(const char* path) {
    return !missingAnimation;
}
AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path) {
    int index = strstr(path, "tame")        ? 0
                : strstr(path, "tamashii1") ? 7
                : strstr(path, "tamashii2") ? 8
                : strstr(path, "tamashii3") ? 9
                : strstr(path, "kaze1")     ? 1
                : strstr(path, "kaze2")     ? 2
                : strstr(path, "kaze3")     ? 3
                : strstr(path, "honoo1")    ? 4
                : strstr(path, "honoo2")    ? 5
                                            : 6;
    return &fixtureAnimations[index].common;
}
void PlayerAnimation_AnimateFrame(PlayState* state, SkelAnime* skel) {
}
#else
static void Player_Action_8084E3C4(Player* this, PlayState* state) {
}
static void Player_Action_80840450(Player* this, PlayState* state) {
}
static void Player_Action_808407CC(Player* this, PlayState* state) {
}
void AudioOcarina_SetInstrument(u8 instrument) {
}
void func_80834644(PlayState* state, Player* this) {
}
void Player_FinishAnimMovement(Player* this) {
}
void func_808326F0(Player* this) {
    if (this->actionFunc == idleAction)
        ++idleCount;
}
s32 Player_CheckHostileLockOn(Player* this) {
    return 0;
}
s32 Player_FriendlyLockOnOrParallel(Player* this) {
    return 0;
}
s32 Player_ActionToModelGroup(Player* this, s32 action) {
    return 0;
}
void Player_SetModels(Player* this, s32 group) {
}
void Player_SetModelGroup(Player* this, s32 group) {
}
s16 OnePointCutscene_EndCutscene(PlayState* state, s16 id) {
    ++subCameraEnds;
    return 1;
}
s16 func_8005B1A4(Camera* cam) {
    ++cameraEnds;
    return 1;
}
s16 OnePointCutscene_Init(PlayState* state, s16 id, s16 timer, Actor* actor, s16 cameraId) {
    ++cameraStarts;
    return 1;
}
void Player_SetTurnAroundCamera(PlayState* state, s32 type) {
    ++cameraStarts;
}
s32 Magic_RequestChange(PlayState* state, s16 amount, s16 type) {
    if (rejectMagic)
        return 0;
    REQUIRE(type == MAGIC_CONSUME_WAIT_PREVIEW);
    spent = amount;
    gSaveContext.magicState = MAGIC_STATE_METER_FLASH_2;
    return 1;
}
void LinkAnimation_AnimateFrame(PlayState* state, SkelAnime* skel) {
}
#endif

#include "dins_gameplay_animation.inc"

static void setAnimation(PlayState* state, SkelAnime* skel, ANIM_HEADER* animation, f32 speed, int loop) {
    lastAnimation = animation;
    skel->animation = animation;
    skel->curFrame = skel->startFrame = 0;
#ifdef TEST_HOST_MM
    skel->animLength = animation == (ANIM_HEADER*)gPlayerAnim_link_normal_wait ? 31 : animation->common.frameCount;
    skel->update.player = loop ? ANIM_LOOP : ANIM_ONCE;
#else
    skel->animLength = strstr((const char*)animation, "tame") ? chargeFrames
                       : strstr((const char*)animation, "1")  ? raiseFrames
                                                              : recoveryFrames;
    skel->update.link = loop ? ANIM_LOOP : ANIM_ONCE;
#endif
    skel->endFrame = skel->animLength - 1;
    skel->playSpeed = speed;
}
#ifdef TEST_HOST_MM
void PlayerAnimation_PlayOnceSetSpeed(PlayState* state, SkelAnime* skel, ANIM_HEADER* anim, f32 speed) {
    setAnimation(state, skel, anim, speed, 0);
}
void PlayerAnimation_PlayLoopSetSpeed(PlayState* state, SkelAnime* skel, ANIM_HEADER* anim, f32 speed) {
    setAnimation(state, skel, anim, speed, 1);
}
#else
void LinkAnimation_PlayOnceSetSpeed(PlayState* state, SkelAnime* skel, ANIM_HEADER* anim, f32 speed) {
    setAnimation(state, skel, anim, speed, 0);
}
void LinkAnimation_PlayLoopSetSpeed(PlayState* state, SkelAnime* skel, ANIM_HEADER* anim, f32 speed) {
    setAnimation(state, skel, anim, speed, 1);
}
#endif

Actor* Actor_Spawn(ActorContext* ctx, PlayState* state, s16 id, f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz,
                   SPAWN_PARAMS params) {
    ++spawnCount;
    if (failSpawn)
        return NULL;
    spawned.id = id;
    spawned.params = params;
    spawned.world.pos = (Vec3f){ x, y, z };
    spawned.update = liveActorUpdate;
    spawned.next = ctx->actorLists[ACTORCAT_ITEMACTION].LIST_HEAD;
    ctx->actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = &spawned;
    spawnTick = testTick;
    return &spawned;
}
static Actor* Sw97_TrySpawnMagicSpell(PlayState* state, Player* this, s32 spell) {
    ++spawnCount;
    spawned.id = 1000 + spell;
    spawned.params = 0;
    return failSpawn ? NULL : &spawned;
}

static int recordNativeCharge(PlayState* state, Player* this, float phase) {
    REQUIRE(state == &play && this == &player);
    ++chargeDraws;
    chargePhase = phase;
    return 1;
}
#define CwDinsFireRemake_DrawNativeCharge recordNativeCharge
#include "dins_gameplay_production.inc"
#undef CwDinsFireRemake_DrawNativeCharge

static void reset(void) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&spawned, 0, sizeof(spawned));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    option = 1;
    missingResource = failSpawn = cameraStarts = cameraEnds = idleCount = spawnCount = magicResets = 0;
    errors = spent = testTick = spawnTick = contactedTick = releasedTick = normalTick = chargeDraws = subCameraEnds = 0;
    missingAnimation = rejectMagic = 0;
    generalSkip = generalSwing = voiceSword = voiceMagic = floorCues = 0;
    audioBegins = audioCancels = audioCancelCalls = audioReleased = audioLive = 0;
    lastAudioHandle = 0;
    sDinsFireRemakeAudioHandle = 0;
    chargeFrames = 41;
    raiseFrames = 51;
    recoveryFrames = 11;
    player.itemAction = PLAYER_IA_DINS_FIRE;
    player.actor.world.pos = (Vec3f){ 11, 20, 33 };
    play.actorCtx.actorLists[ACTORCAT_PLAYER].LIST_HEAD = &player.actor;
    player.actionFunc = idleAction;
    MEDALLION_LATCH = 0;
#ifdef TEST_HOST_MM
    memset(&nei, 0, sizeof(nei));
    play.state.framerateDivisor = 3;
    gSaveContext.magicCapacity = 48;
    gSaveContext.save.saveInfo.playerData.magic = 48;
    for (int i = 0; i < 10; ++i) {
        fixtureAnimations[i].common.frameCount = i == 0 ? 41 : (i == 1 || i == 4 || i == 7) ? 51 : 11;
        fixtureAnimations[i].segmentVoid = resourceBytes;
    }
#else
    R_UPDATE_RATE = 3;
#endif
}

static void test_native_cast_uses_normal_camera_and_releases_player(void) {
    reset();
    SETUP_CAST(&play, &player, 5);
    REQUIRE(cameraStarts == 0);
    REQUIRE(spent == 12);
    for (testTick = 1; testTick <= 35 && !idleCount; ++testTick) {
        CAST_ACTION(&player, &play);
        if (!contactedTick && (spawned.params & 2))
            contactedTick = testTick;
        if (!releasedTick && (spawned.params & 1))
            releasedTick = testTick;
        if (idleCount)
            normalTick = testTick;
        int previousDraws = chargeDraws;
        Player_DrawDinsFireRemakeCharge(&player, &play);
        if (testTick < 21) {
            REQUIRE(chargeDraws == previousDraws + 1);
            REQUIRE(fabsf(chargePhase - testTick / 20.0f) < 0.0001f);
        } else {
            REQUIRE(chargeDraws == previousDraws);
        }
        if (testTick == 12) {
            REQUIRE(spawned.params == 0x4D00 && spawned.parent == &player.actor);
#ifdef TEST_HOST_MM
            REQUIRE(lastAnimation == &fixtureAnimations[7]);
#else
            REQUIRE(lastAnimation == &gPlayerAnim_link_magic_tamashii1);
#endif
        }
        if (testTick == 20 || testTick == 21)
            REQUIRE(fabsf(player.skelAnime.curFrame - 30.0f) < 0.0001f);
    }
    REQUIRE(spawnCount == 1 && spawned.id == DIN_ACTOR);
    REQUIRE(idleCount == 1);
    REQUIRE(cameraEnds == 0);
    REQUIRE((player.stateFlags1 & CAST_FLAGS) == 0);
    REQUIRE(spawned.update != NULL);
    REQUIRE(magicResets == 0 && gSaveContext.magicState != MAGIC_STATE_IDLE);
    REQUIRE(spawnTick == 12 && contactedTick == 20 && releasedTick == 21 && normalTick == 27);
    REQUIRE(!Player_InCsMode(&play));
#ifdef TEST_HOST_MM
    REQUIRE(!OotSpells_TryUseItem(&play, &player, PLAYER_IA_DEKU_STICK, ITEM_DEKU_STICK));
    REQUIRE(!OotSpells_TryUseItem(&play, &player, PLAYER_IA_BOW, ITEM_BOW));
    REQUIRE(magicResets == 0 && gSaveContext.magicState != MAGIC_STATE_IDLE);
#endif
}

static void test_disabled_and_missing_resources_keep_native_path(void) {
    for (int reason = 0; reason < 2; ++reason) {
        reset();
        option = reason != 0;
        missingResource = reason == 1;
        SETUP_CAST(&play, &player, 5);
#ifndef TEST_HOST_MM
        REQUIRE(cameraStarts == 1);
#endif
        for (testTick = 1; testTick <= 150; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(spawnCount == 1 && spawned.id == DIN_ACTOR);
        REQUIRE(spawned.parent == NULL && spawned.params < 2);
        REQUIRE(idleCount == 0 && magicResets == 0);
        gSaveContext.magicState = MAGIC_STATE_IDLE;
        CAST_ACTION(&player, &play);
        REQUIRE(idleCount == 1 && cameraEnds == 1);
    }
}

static void test_mm_live_fire_owns_magic_after_control_returns(void) {
#ifdef TEST_HOST_MM
    reset();
    spawned.id = DIN_ACTOR;
    spawned.params = 0x4D01;
    spawned.parent = &player.actor;
    spawned.update = liveActorUpdate;
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = &spawned;
    gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 0);
    REQUIRE(gSaveContext.magicState == MAGIC_STATE_METER_FLASH_1);
    REQUIRE(errors == 1 && player.unk_AA5 != PLAYER_UNKAA5_5);
    /* Its destroy callback still owns the reset while a killed actor remains
     * linked; otherwise that callback can erase a new spell's reservation. */
    spawned.update = NULL;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 0 && gSaveContext.magicState == MAGIC_STATE_METER_FLASH_1);
    REQUIRE(errors == 2 && player.unk_AA5 != PLAYER_UNKAA5_5);
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = NULL;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 1 && gSaveContext.magicState == MAGIC_STATE_IDLE);
    REQUIRE(player.unk_AA5 == PLAYER_UNKAA5_5);
    /* MM migrates native Din to MISC so later-fired arrows update first. */
    reset();
    spawned.id = DIN_ACTOR;
    spawned.params = 0x4D03;
    spawned.parent = &player.actor;
    spawned.update = liveActorUpdate;
    play.actorCtx.actorLists[ACTORCAT_MISC].LIST_HEAD = &spawned;
    gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 0 && player.unk_AA5 != PLAYER_UNKAA5_5);
    spawned.update = NULL;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 0 && player.unk_AA5 != PLAYER_UNKAA5_5);
    play.actorCtx.actorLists[ACTORCAT_MISC].LIST_HEAD = NULL;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 1 && player.unk_AA5 == PLAYER_UNKAA5_5);
    for (int mismatch = 0; mismatch < 3; ++mismatch) {
        reset();
        spawned.id = DIN_ACTOR;
        spawned.params = 0x4D01;
        spawned.parent = &player.actor;
        spawned.update = liveActorUpdate;
        if (mismatch == 0)
            spawned.id = ACTOR_SW97_MAGIC_FIRE;
        if (mismatch == 1)
            spawned.params = 1;
        if (mismatch == 2)
            spawned.parent = NULL;
        play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = &spawned;
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
        REQUIRE(magicResets == 1 && player.unk_AA5 == PLAYER_UNKAA5_5);
    }
    reset();
    gSaveContext.magicState = MAGIC_STATE_CONSUME_LENS;
    OotSpells_TryUseItem(&play, &player, PLAYER_IA_DINS_FIRE, 0);
    REQUIRE(magicResets == 0 && gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS);
#endif
}

static void test_native_cast_keeps_world_updates_running(void) {
    reset();
    SETUP_CAST(&play, &player, 5);
    REQUIRE((TEST_FREEZE_MASKS[ACTORCAT_ENEMY] & player.stateFlags1) == 0);
    REQUIRE((TEST_FREEZE_MASKS[ACTORCAT_BG] & player.stateFlags1) == 0);
    REQUIRE(player.invincibilityTimer == 2);
    for (testTick = 1; testTick <= 27; ++testTick) {
        --player.invincibilityTimer;
        CAST_ACTION(&player, &play);
        REQUIRE(player.invincibilityTimer == 2);
        REQUIRE((TEST_FREEZE_MASKS[ACTORCAT_ENEMY] & player.stateFlags1) == 0);
        REQUIRE((TEST_FREEZE_MASKS[ACTORCAT_BG] & player.stateFlags1) == 0);
    }
}

static void test_native_latch_survives_live_changes(void) {
    for (int change = 0; change < 3; ++change) {
        reset();
        SETUP_CAST(&play, &player, 5);
        if (change == 0)
            option = 0;
        if (change == 1)
            missingResource = 1;
        if (change == 2)
            missingAnimation = 1;
        for (testTick = 1; testTick <= 27; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(spawned.params == 0x4D03 && spawned.parent == &player.actor);
        REQUIRE(idleCount == 1 && magicResets == 0 && cameraStarts == 0 && cameraEnds == 0);
    }
    reset();
    option = 0;
    SETUP_CAST(&play, &player, 5);
    option = 1;
    for (testTick = 1; testTick <= 50; ++testTick)
        CAST_ACTION(&player, &play);
    REQUIRE(spawned.params < 2 && spawned.parent == NULL);
    int priorDraws = chargeDraws;
    Player_DrawDinsFireRemakeCharge(&player, &play);
    REQUIRE(chargeDraws == priorDraws);
}

static void test_failed_spawn_and_lost_owner_cannot_strand_player(void) {
    reset();
    failSpawn = 1;
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 12; ++testTick)
        CAST_ACTION(&player, &play);
    REQUIRE(idleCount == 1 && magicResets == 1 && gSaveContext.magicState == MAGIC_STATE_IDLE);
    REQUIRE(cameraStarts == 0 && cameraEnds == 0 && subCameraEnds == 0);
    reset();
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 27; ++testTick) {
        if (testTick == 20)
            play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = NULL;
        CAST_ACTION(&player, &play);
    }
    REQUIRE(idleCount == 1 && magicResets == 1 && gSaveContext.magicState == MAGIC_STATE_IDLE);
    REQUIRE(cameraStarts == 0 && cameraEnds == 0 && subCameraEnds == 0);
    reset();
    rejectMagic = 1;
    gSaveContext.magicState = MAGIC_STATE_CONSUME_LENS;
    SETUP_CAST(&play, &player, 5);
    REQUIRE(idleCount == 1 && spawnCount == 0 && magicResets == 0);
    REQUIRE(gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS && cameraStarts == 0 && cameraEnds == 0);
}

static void test_native_timing_uses_available_animation_frames(void) {
    reset();
    chargeFrames = 18;
    raiseFrames = 13;
    recoveryFrames = 7;
#ifdef TEST_HOST_MM
    fixtureAnimations[0].common.frameCount = chargeFrames;
    fixtureAnimations[7].common.frameCount = raiseFrames;
    fixtureAnimations[9].common.frameCount = recoveryFrames;
#endif
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 27; ++testTick) {
        CAST_ACTION(&player, &play);
        if (testTick == 20 || testTick == 21)
            REQUIRE(fabsf(player.skelAnime.curFrame - 12.0f) < 0.0001f);
    }
    REQUIRE(idleCount == 1 && spawned.params == 0x4D03);
#ifdef TEST_HOST_MM
    reset();
    missingAnimation = 1;
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 50; ++testTick)
        CAST_ACTION(&player, &play);
    REQUIRE(spawned.params == 1 && spawned.parent == NULL);
    REQUIRE(idleCount == 0);
#endif
    const s8 preservedTimers[] = { -20, 30 };
    for (int i = 0; i < 2; ++i) {
        reset();
        player.invincibilityTimer = preservedTimers[i];
        SETUP_CAST(&play, &player, 5);
        CAST_ACTION(&player, &play);
        REQUIRE(player.invincibilityTimer == preservedTimers[i]);
    }
}

static void test_other_spells_and_medallions_keep_their_routes(void) {
    for (int spell = 3; spell <= 4; ++spell) {
        reset();
        player.itemAction = PLAYER_IA_MAGIC_SPELL_15 + spell;
        SETUP_CAST(&play, &player, spell);
        for (testTick = 1; testTick <= 50 && !spawnCount; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(spawnCount == 1 && spawned.id != DIN_ACTOR && spawned.parent == NULL && spawned.params < 2);
        REQUIRE(audioBegins == 0 && audioCancelCalls == 0);
#ifndef TEST_HOST_MM
        REQUIRE(cameraStarts == 1);
#endif
    }
    for (int spell = 0; spell < 6; ++spell) {
        reset();
        MEDALLION_LATCH = 1;
        player.itemAction = PLAYER_IA_MAGIC_SPELL_15 + spell;
        SETUP_CAST(&play, &player, spell);
        for (testTick = 1; testTick <= 50 && !spawnCount; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(spawnCount == 1 && spawned.id == 1000 + spell && spawned.parent == NULL && spawned.params == 0);
        REQUIRE(gSaveContext.magicState == MAGIC_STATE_CONSUME_SETUP);
        REQUIRE(audioBegins == 0 && audioCancelCalls == 0);
    }
}

static void test_native_exit_never_returns_a_spell_camera(void) {
    reset();
    SETUP_CAST(&play, &player, 5);
#ifndef TEST_HOST_MM
    // A native cast owns no spell camera, even if an old id remains in the player.
    player.subCamId = 1;
    camera.csId = 1100;
    play.cameraPtrs[1] = &camera;
#endif
    for (testTick = 1; testTick <= 27; ++testTick)
        CAST_ACTION(&player, &play);
    REQUIRE(cameraStarts == 0 && cameraEnds == 0 && subCameraEnds == 0);
#ifndef TEST_HOST_MM
    reset();
    SETUP_CAST(&play, &player, 5);
    player.subCamId = 1;
    camera.csId = 1100;
    play.cameraPtrs[1] = &camera;
    // Forced action changes must not leave the native latch affecting later
    // ordinary camera cleanup or an unrelated direct spell actor spawn.
    Player_SetupAction(&play, &player, idleAction, 1);
    REQUIRE(subCameraEnds == 0);
    func_80832340(&play, &player);
    REQUIRE(subCameraEnds == 1);
    Player_SpawnMagicSpell(&play, &player, 2);
    REQUIRE(spawned.params == 0 && spawned.parent == NULL);
#endif
}

static void forceNormalAction(void) {
#ifdef TEST_HOST_MM
    Player_SetAction(&play, &player, idleAction, 1);
#else
    Player_SetupAction(&play, &player, idleAction, 1);
#endif
}

static void test_pre_spawn_cancellation_releases_only_its_reservation(void) {
    const int cancelTicks[] = { 0, 1, 11 };
    for (int i = 0; i < 3; ++i) {
        reset();
        SETUP_CAST(&play, &player, 5);
        for (int tick = 0; tick < cancelTicks[i]; ++tick)
            CAST_ACTION(&player, &play);
        REQUIRE(spawnCount == 0 && gSaveContext.magicState == MAGIC_STATE_METER_FLASH_2);
        forceNormalAction();
        REQUIRE(magicResets == 1 && gSaveContext.magicState == MAGIC_STATE_IDLE);
        REQUIRE(!Player_InCsMode(&play));
        REQUIRE(cameraStarts == 0 && cameraEnds == 0 && subCameraEnds == 0);
    }
    const int ownedTicks[] = { 12, 20, 21 };
    for (int i = 0; i < 3; ++i) {
        for (int killed = 0; killed < 2; ++killed) {
            reset();
            SETUP_CAST(&play, &player, 5);
            for (int tick = 0; tick < ownedTicks[i]; ++tick)
                CAST_ACTION(&player, &play);
            if (killed)
                spawned.update = NULL;
            forceNormalAction();
            REQUIRE(magicResets == 0 && gSaveContext.magicState == MAGIC_STATE_CONSUME_SETUP);
        }
    }
    const int foreignStates[] = { MAGIC_STATE_METER_FLASH_2, MAGIC_STATE_CONSUME_LENS };
    for (int i = 0; i < 2; ++i) {
        reset();
        rejectMagic = 1;
        gSaveContext.magicState = foreignStates[i];
        SETUP_CAST(&play, &player, 5);
        REQUIRE(idleCount == 1 && magicResets == 0 && gSaveContext.magicState == foreignStates[i]);
    }
    reset();
    SETUP_CAST(&play, &player, 5);
    gSaveContext.magicState = MAGIC_STATE_CONSUME_LENS;
    forceNormalAction();
    REQUIRE(magicResets == 0 && gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS);
    for (int killed = 0; killed < 2; ++killed) {
        reset();
        SETUP_CAST(&play, &player, 5);
        spawned.id = DIN_ACTOR;
        spawned.params = 0x4D00;
        spawned.parent = &player.actor;
        spawned.update = killed ? NULL : liveActorUpdate;
        play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = &spawned;
        forceNormalAction();
        REQUIRE(magicResets == 0 && gSaveContext.magicState == MAGIC_STATE_METER_FLASH_2);
    }
    reset();
    option = 0;
    SETUP_CAST(&play, &player, 5);
    forceNormalAction();
    REQUIRE(magicResets == 0 && gSaveContext.magicState == MAGIC_STATE_METER_FLASH_2);
}

static void test_native_audio_charge_lifecycle(void) {
    const int cancelTicks[] = { 0, 1, 11, 12, 20, 21, 27 };
    for (int i = 0; i < 7; ++i) {
        reset();
        SETUP_CAST(&play, &player, 5);
        REQUIRE(audioBegins == 1 && audioLive && sDinsFireRemakeAudioHandle == lastAudioHandle);
        for (testTick = 1; testTick <= cancelTicks[i]; ++testTick) {
            CAST_ACTION(&player, &play);
            // The real actor owns release; this fixture records only the
            // player/action boundary. Mixer/actor suites verify that ownership.
            if (spawned.params & CW_DINS_NATIVE_RELEASED)
                audioReleased = 1;
        }
        forceNormalAction();
        REQUIRE(audioCancelCalls == 1 && sDinsFireRemakeAudioHandle == 0);
        REQUIRE(audioCancels == (cancelTicks[i] < 21));
        REQUIRE(audioLive == (cancelTicks[i] >= 21));
        forceNormalAction();
        REQUIRE(audioCancelCalls == 1);
    }
    reset();
    rejectMagic = 1;
    SETUP_CAST(&play, &player, 5);
    REQUIRE(audioBegins == 0 && audioCancelCalls == 0 && !audioLive);
    reset();
    failSpawn = 1;
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 12; ++testTick)
        CAST_ACTION(&player, &play);
    REQUIRE(audioBegins == 1 && audioCancels == 1 && audioCancelCalls == 1 && !audioLive);
    REQUIRE(sDinsFireRemakeAudioHandle == 0);
    reset();
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 21; ++testTick) {
        if (testTick == 20)
            play.actorCtx.actorLists[ACTORCAT_ITEMACTION].LIST_HEAD = NULL;
        CAST_ACTION(&player, &play);
    }
    REQUIRE(audioBegins == 1 && audioCancels == 1 && !audioLive);
    REQUIRE(sDinsFireRemakeAudioHandle == 0);
}

static void test_native_audio_dispatch_and_stock_preservation(void) {
    reset();
    SETUP_CAST(&play, &player, 5);
    for (testTick = 1; testTick <= 27; ++testTick)
        CAST_ACTION(&player, &play);
    REQUIRE(generalSkip == 0 && generalSwing == 0);
    REQUIRE(voiceSword == 1 && voiceMagic == 1 && floorCues == 1);
    REQUIRE(audioBegins == 1);

    for (int change = 0; change < 3; ++change) {
        reset();
        SETUP_CAST(&play, &player, 5);
        if (change == 0)
            option = 0;
        if (change == 1)
            missingResource = 1;
        if (change == 2)
            missingAnimation = 1;
        for (testTick = 1; testTick <= 27; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(audioBegins == 1 && generalSkip == 0 && generalSwing == 0);
        REQUIRE(voiceSword == 1 && voiceMagic == 1 && floorCues == 1);
    }

    // The same stock lists are also used by other spells and medallions.
    for (int spell = 3; spell <= 5; ++spell) {
        reset();
        option = 0;
        player.itemAction = PLAYER_IA_MAGIC_SPELL_15 + spell;
        SETUP_CAST(&play, &player, spell);
        for (testTick = 1; testTick <= 100; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(generalSkip == 1 && voiceSword == 1);
        REQUIRE(generalSwing == (spell == 5));
        REQUIRE(audioBegins == 0 && audioCancelCalls == 0);
    }
    for (int spell = 0; spell < 6; ++spell) {
        reset();
        MEDALLION_LATCH = 1;
        player.itemAction = PLAYER_IA_MAGIC_SPELL_15 + spell;
        SETUP_CAST(&play, &player, spell);
        for (testTick = 1; testTick <= 100; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(generalSkip == 1 && voiceSword == 1);
        REQUIRE(generalSwing == (spell == 2 || spell == 4));
        REQUIRE(audioBegins == 0 && audioCancelCalls == 0);
    }

    for (int fallback = 0; fallback < 3; ++fallback) {
#ifndef TEST_HOST_MM
        if (fallback == 2)
            continue;
#endif
        reset();
        option = fallback != 0;
        missingResource = fallback == 1;
        missingAnimation = fallback == 2;
        SETUP_CAST(&play, &player, 5);
        for (testTick = 1; testTick <= 100; ++testTick)
            CAST_ACTION(&player, &play);
        REQUIRE(generalSkip == 1 && generalSwing == 1);
        REQUIRE(voiceSword == 1 && voiceMagic == 1 && floorCues == 1);
        REQUIRE(audioBegins == 0 && audioCancelCalls == 0);
    }
}

int main(int argc, char** argv) {
    if (argc == 1 || !strcmp(argv[1], "audio"))
        test_native_audio_dispatch_and_stock_preservation();
    if (argc == 1 || !strcmp(argv[1], "audio"))
        test_native_audio_charge_lifecycle();
    if (argc == 1 || !strcmp(argv[1], "fallback"))
        test_disabled_and_missing_resources_keep_native_path();
    if (argc == 1 || !strcmp(argv[1], "ownership"))
        test_mm_live_fire_owns_magic_after_control_returns();
    if (argc == 1 || !strcmp(argv[1], "cast"))
        test_native_cast_uses_normal_camera_and_releases_player();
    if (argc == 1 || !strcmp(argv[1], "world"))
        test_native_cast_keeps_world_updates_running();
    if (argc == 1 || !strcmp(argv[1], "camera"))
        test_native_exit_never_returns_a_spell_camera();
    if (argc == 1 || !strcmp(argv[1], "cancellation"))
        test_pre_spawn_cancellation_releases_only_its_reservation();
    if (argc == 1) {
        test_native_latch_survives_live_changes();
        test_failed_spawn_and_lost_owner_cannot_strand_player();
        test_native_timing_uses_available_animation_frames();
        test_other_spells_and_medallions_keep_their_routes();
    }
    puts("PASS native Din: spawn12/contact20/release21/control27, native audio begin/cancel, real animation SFX "
         "suppression with voice/floor/legacy preservation, camera/world, ownership and failures");
    return 0;
}
