#include "global.h"
#include "overlays/actors/ovl_En_Sa/z_en_sa.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "ComboGameMode.h"
#include "soh/Enhancements/cosmetics/NativeNpcModels.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/OTRGlobals.h"
#include "objects/object_sa/object_sa.h"
#include "scenes/overworld/spot04/spot04_scene.h"
#include "scenes/overworld/spot05/spot05_scene.h"
#include "combo/encounters/NativeCustomEnemies.h"
#include "rando/SariaHintPolicy.h"
#include <assert.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
u32 gBitFlags[32];
u8 gItemSlots[56];
int gMapLoading;
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode;
static PlayState play;
static Player player;
static EnSa saria;
static Actor fairy;
static u8 savedSettings[RSK_MAX];
static bool killed;
static int initCalls, songCutsceneStarts, grants;
static u8 messageState;
void SariaFixture_OpenNativeHint(uint16_t textId);
void SariaFixture_VerifyRuntimeControls(void);
int SariaFixture_GetClassic(void) { return savedSettings[RSK_SARIA_HINT]; }
int SariaFixture_GetStat(void) { return savedSettings[RSK_MAGIC_STAT_UPGRADE]; }
int SariaFixture_IsRandomizer(void) { return IS_RANDO; }
void SariaFixture_SetRandomizer(int enabled) { gSaveContext.ship.quest.id = enabled ? QUEST_RANDOMIZER : QUEST_NORMAL; }
void SariaFixture_SetOptions(int classic, int stat) {
    savedSettings[RSK_SARIA_HINT] = classic;
    savedSettings[RSK_MAGIC_STAT_UPGRADE] = stat;
}

void EnSa_Init(Actor*, PlayState*);
void EnSa_Update(Actor*, PlayState*);
void func_80AF6448(EnSa*, PlayState*);
void func_80AF67D0(EnSa*, PlayState*);
void func_80AF683C(EnSa*, PlayState*);
void func_80AF68E4(EnSa*, PlayState*);
void EnSa_ChangeAnim(EnSa*, s32);

u8 Randomizer_GetSettingValue(RandomizerSettingKey key) { return savedSettings[key]; }
bool GameInteractor_Should(GIVanillaBehavior behavior, u32 value, ...) {
    if (IS_RANDO && behavior == VB_BE_ELIGIBLE_FOR_SARIAS_SONG)
        return !Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SARIAS_SONG);
    if (behavior == VB_PLAY_SARIAS_SONG_CS)
        ++songCutsceneStarts;
    return value;
}
void GameInteractor_ExecuteOnFlagSet(s16 flagType, s16 flag) {}
u8 ResourceMgr_FileExists(const char* path) { return false; }
u8 ResourceMgr_FileAltExists(const char* path) { return false; }
bool ResourceMgr_IsAltAssetsEnabled(void) { return false; }
void CustomEnemies_RecordEntry(PlayState* p, int16_t id, float x, float y, float z, int16_t params) {}
void osSyncPrintfUnused(const char* format, ...) {}
#undef osSyncPrintf
#define osSyncPrintf(...) osSyncPrintfUnused(__VA_ARGS__)
#define LUSLOG_INFO(...) ((void)0)

// Renderer, animation storage, collision and tracking services do not select
// actor eligibility, dialogue mode or native item/event progression.
void ActorShape_Init(ActorShape* shape, f32 yOffset, ActorShadowFunc draw, f32 scale) {}
void ActorShadow_DrawCircle(Actor* actor, Lights* lights, PlayState* p) {}
s32 SkelAnime_InitFlex(PlayState* p, SkelAnime* skel, FlexSkeletonHeader* header,
                       AnimationHeader* anim, Vec3s* joints, Vec3s* morphs, s32 count) {
    skel->animation = anim;
    ++initCalls;
    return true;
}
s16 Animation_GetLastFrame(void* animation) { return 60; }
void Animation_Change(SkelAnime* skel, AnimationHeader* anim, f32 speed, f32 start, f32 end, u8 mode, f32 morph) {
    skel->animation = anim;
    skel->curFrame = start;
    skel->playSpeed = speed;
}
s32 SkelAnime_Update(SkelAnime* skel) { return false; }
s32 Collider_InitCylinder(PlayState* p, ColliderCylinder* collider) { return true; }
s32 Collider_SetCylinder(PlayState* p, ColliderCylinder* collider, Actor* actor, ColliderCylinderInit* init) {
    collider->dim.radius = init->dim.radius;
    return true;
}
void CollisionCheck_SetInfo2(CollisionCheckInfo* info, DamageTable* damage, CollisionCheckInfoInit2* init) {}
void Collider_UpdateCylinder(Actor* actor, ColliderCylinder* collider) {}
s32 CollisionCheck_SetOC(PlayState* p, CollisionCheckContext* ctx, Collider* collider) { return true; }
void Actor_SetScale(Actor* actor, f32 scale) { actor->scale = (Vec3f){scale, scale, scale}; }
void Actor_Kill(Actor* actor) { killed = true; actor->update = NULL; actor->draw = NULL; }
Actor* Actor_SpawnAsChild(ActorContext* ctx, Actor* parent, PlayState* p, s16 id, f32 x, f32 y,
                         f32 z, s16 rx, s16 ry, s16 rz, s16 params) { return &fairy; }
Actor* Actor_Spawn(ActorContext* ctx, PlayState* p, s16 id, f32 x, f32 y, f32 z,
                   s16 rx, s16 ry, s16 rz, s16 params) {
    assert(id == ACTOR_EN_SA);
    memset(&saria, 0, sizeof(saria));
    saria.actor.id = id;
    saria.actor.home.pos = saria.actor.world.pos = (Vec3f){x, y, z};
    saria.actor.home.rot = saria.actor.world.rot = saria.actor.shape.rot = (Vec3s){rx, ry, rz};
    saria.actor.params = params;
    saria.actor.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY;
    saria.actor.update = EnSa_Update;
    saria.actor.init = EnSa_Init;
    return &saria.actor;
}
void Actor_GetScreenPos(PlayState* p, Actor* actor, s16* x, s16* y) { *x = 160; *y = 100; }
u16 Text_GetFaceReaction(PlayState* p, u32 reaction) { return 0; }
u8 Message_GetState(MessageContext* ctx) { return messageState; }
s32 Player_InCsMode(PlayState* p) { return false; }
s32 Play_InCsMode(PlayState* p) { return false; }
s16 Actor_UpdateAlphaByDistance(Actor* actor, PlayState* p, s16 alpha, f32 range) { return 255; }
void Actor_UpdatePos(Actor* actor) {}
void Actor_UpdateBgCheckInfo(PlayState* p, Actor* actor, f32 a, f32 b, f32 c, s32 flags) {}
void func_80AF5CD4(EnSa* actor, u8 value) {}
void func_80AF5CE4(EnSa* actor) {}
void func_80AF609C(EnSa* actor) {}
void func_80AF5F34(EnSa* actor, PlayState* p) {}
void func_80AF68E4(EnSa* actor, PlayState* p) {}
u8 Item_Give(PlayState* p, u8 item) { ++grants; return item; }

/* PRODUCTION */

static void Reset(bool hint, bool metZelda, bool learned, bool randomizer, bool adult) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(savedSettings, 0, sizeof(savedSettings));
    for (int bit = 0; bit < 32; ++bit)
        gBitFlags[bit] = 1u << bit;
    gSaveContext.ship.quest.id = randomizer ? QUEST_RANDOMIZER : QUEST_NORMAL;
    gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
    savedSettings[RSK_SARIA_HINT] = hint;
    savedSettings[RSK_MAGIC_STAT_UPGRADE] = 1;
    // The user's three unrelated seed options are all off/vanilla.
    savedSettings[RSK_SHUFFLE_ZELDAS_LETTER] = 0;
    savedSettings[RSK_SKIP_CHILD_STEALTH] = 0;
    savedSettings[RSK_SHUFFLE_WEIRD_EGG] = RO_WEIRD_EGG_VANILLA;
    if (metZelda)
        Flags_SetEventChkInf(EVENTCHKINF_OBTAINED_ZELDAS_LETTER);
    if (learned)
        Flags_SetEventChkInf(EVENTCHKINF_LEARNED_SARIAS_SONG);
    play.sceneNum = SCENE_SACRED_FOREST_MEADOW;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.talkActorDistance = FLT_MAX;
    player.actor.world.pos.z = -2300;
    killed = false;
    initCalls = songCutsceneStarts = grants = 0;
    messageState = TEXT_STATE_NONE;
}

static void SpawnAndInit(void) {
    // Controlled room row: source actor eligibility is tested separately from
    // the unprovided active game pack's actor-list and OBJECT_SA dependencies.
    ActorEntry entry = {ACTOR_EN_SA, {118, 489, -2941}, {0, -3277, 0}, 0};
    Actor* actor = Actor_SpawnEntry(&play.actorCtx, &entry, &play);
    assert(actor == &saria.actor && actor->init == EnSa_Init);
    actor->init(actor, &play);
    actor->init = NULL;
    assert(initCalls == 1);
    saria.actor.xzDistToPlayer = 30;
    saria.actor.yawTowardsPlayer = saria.actor.shape.rot.y;
}

int main(void) {
    Reset(true, false, false, true, false);
    SpawnAndInit();
    assert(!killed && saria.actor.update == EnSa_Update && saria.alpha == 255);
    assert(saria.actionFunc == func_80AF6448);
    EnSa_Update(&saria.actor, &play);
    assert(player.talkActor == &saria.actor && saria.actor.textId == 0x10AD);
    saria.actor.flags |= ACTOR_FLAG_TALK; // Player accepts the real offered talk.
    EnSa_Update(&saria.actor, &play);
    assert(saria.interactInfo.talkState == NPC_TALK_STATE_TALKING);
    EnSa_Update(&saria.actor, &play);
    assert(saria.actionFunc == func_80AF67D0);
    messageState = TEXT_STATE_CLOSING;
    EnSa_Update(&saria.actor, &play);
    EnSa_Update(&saria.actor, &play);
    assert(saria.actionFunc == func_80AF6448 && saria.interactInfo.talkState == NPC_TALK_STATE_IDLE);
    assert(!Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_ZELDAS_LETTER));
    assert(!Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SARIAS_SONG) && !songCutsceneStarts && !grants);
    puts("PASS native Saria scene-entry/init/update/talk: pre-Zelda saved hint on, all three reported options off, no song/event grant");

    Reset(false, false, false, true, false);
    SpawnAndInit();
    assert(!killed && saria.actionFunc == func_80AF6448 &&
           "Magic Stat Upgrade fallback must survive a saved classic Saria Hint value of zero");
    EnSa_Update(&saria.actor, &play);
    assert(player.talkActor == &saria.actor && saria.actor.textId == 0x10AD);
    SariaFixture_OpenNativeHint(saria.actor.textId);
    assert(savedSettings[RSK_SARIA_HINT] == 0 && !songCutsceneStarts && !grants);
    assert(!Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_ZELDAS_LETTER) &&
           !Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SARIAS_SONG));
    puts("PASS native Saria old saved hint off + Magic Stat Upgrade: actor and hint text survive without rewriting settings/events");

    Reset(false, false, false, true, false);
    savedSettings[RSK_MAGIC_STAT_UPGRADE] = 0;
    SpawnAndInit();
    assert(killed && !saria.actor.update);
    puts("PASS native Saria classic hint off without stat-upgrade fallback retains the original story gate");

    Reset(false, true, false, true, false);
    savedSettings[RSK_MAGIC_STAT_UPGRADE] = 0;
    SpawnAndInit();
    assert(!killed && saria.actionFunc == func_80AF683C);
    EnSa_Update(&saria.actor, &play);
    assert(Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SARIAS_SONG));
    assert(songCutsceneStarts == 1 && gSaveContext.cutsceneTrigger == 1);
    assert(saria.actionFunc == func_80AF68E4 && !grants);
    puts("PASS native Saria Met Zelda event still unlocks the original song trigger with hint off");

    Reset(true, false, false, true, true);
    SpawnAndInit();
    assert(killed);
    Reset(true, false, false, false, false);
    SpawnAndInit();
    assert(killed);
    Reset(false, true, true, true, false);
    SpawnAndInit();
    assert(!killed && saria.actionFunc == func_80AF6448 && !songCutsceneStarts);
    puts("PASS native Saria adult/vanilla pre-Zelda guards and completed-song conversation preserved");
    SariaFixture_VerifyRuntimeControls();
    return 0;
}
