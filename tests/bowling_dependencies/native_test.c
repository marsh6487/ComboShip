/* Run the complete native bullseye/wall state machines. Resource registration,
 * actor allocation, audio and effects are boundaries; timers, hit detection,
 * dependency lookup, movement and round reset use production code. */
#include "overlays/actors/ovl_Bg_Bowl_Wall/z_bg_bowl_wall.h"
#include "overlays/actors/ovl_En_Bom_Chu/z_en_bom_chu.h"
#include "overlays/actors/ovl_En_Wall_Tubo/z_en_wall_tubo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                                         \
    do {                                                                                                         \
        if (!(condition)) {                                                                                      \
            fprintf(stderr, "%s:%d: requirement failed: %s\n", __FILE__, __LINE__, #condition);                   \
            exit(1);                                                                                             \
        }                                                                                                        \
    } while (0)

void EnWallTubo_Init(Actor*, PlayState*);
void EnWallTubo_Update(Actor*, PlayState*);
void BgBowlWall_Init(Actor*, PlayState*);
void BgBowlWall_Update(Actor*, PlayState*);

static GameInfo gameInfo;
GameInfo* gGameInfo = &gameInfo;
static CollisionHeader collision;
static EnWallTubo targets[8];
static int targetCount;
static int failNextSpawn;
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale = 1.0f;
s8 gSfxDefaultReverb;

void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void CollisionHeader_GetVirtual(void* source, CollisionHeader** dest) { *dest = &collision; }
s32 DynaPoly_SetBgActor(PlayState* play, DynaCollisionContext* dyna, Actor* actor, CollisionHeader* header) {
    CHECK(header == &collision);
    return 0;
}
void DynaPoly_DeleteBgActor(PlayState* play, DynaCollisionContext* dyna, s32 bgId) {}

Actor* Actor_SpawnAsChild(ActorContext* actors, Actor* parent, PlayState* play, s16 actorId, f32 x, f32 y, f32 z,
                          s16 rotX, s16 rotY, s16 rotZ, s16 params) {
    CHECK(actors == &play->actorCtx && actorId == ACTOR_EN_WALL_TUBO);
    if (failNextSpawn) {
        failNextSpawn = 0;
        return NULL;
    }
    CHECK(targetCount < ARRAY_COUNT(targets));
    EnWallTubo* target = &targets[targetCount++];
    memset(target, 0, sizeof(*target));
    target->actor.id = actorId;
    target->actor.params = params;
    target->actor.parent = parent;
    target->actor.world.pos = (Vec3f){ x, y, z };
    target->actor.update = EnWallTubo_Update;
    parent->child = &target->actor;
    EnWallTubo_Init(&target->actor, play);
    return &target->actor;
}

void Actor_Kill(Actor* actor) {
    actor->draw = NULL;
    actor->update = NULL;
    actor->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
}
f32 Rand_ZeroFloat(f32 scale) { return 0; }
f32 Rand_CenteredFloat(f32 scale) { return 0; }
void Audio_PlayActorSound2(Actor* actor, u16 sfx) {}
void Audio_PlaySfxGeneral(u16 sfxId, Vec3f* pos, u8 token, f32* freqScale, f32* volume, s8* reverb) {}
void EffectSsBomb2_SpawnLayered(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* accel, s16 scale, s16 step) {}
void EffectSsHahen_SpawnBurst(PlayState* play, Vec3f* pos, f32 burst, s16 unused, s16 scale, s16 range, s16 count,
                              s16 object, s16 life, Gfx* list) {}
s16 Quake_Add(Camera* camera, u32 callback) { return 0; }
u32 Quake_SetSpeed(s16 index, s16 speed) { return 1; }
u32 Quake_SetCountdown(s16 index, s16 time) { return 1; }
u32 Quake_SetQuakeValues(s16 index, s16 y, s16 x, s16 zoom, s16 rotation) { return 1; }
DebugDispObject* DebugDisplay_AddObject(f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz, f32 sx, f32 sy, f32 sz,
                                        u8 r, u8 g, u8 b, u8 alpha, s16 type, GraphicsContext* gfx) {
    abort();
}

// Rendering is outside these update tests; an accidental draw must fail.
void FrameInterpolation_RecordOpenChild(const void* actor, int epoch) { abort(); }
void FrameInterpolation_RecordCloseChild(void) { abort(); }
void Graph_OpenDisps(Gfx** refs, GraphicsContext* gfx, const char* file, s32 line) { abort(); }
void Graph_CloseDisps(Gfx** refs, GraphicsContext* gfx, const char* file, s32 line) { abort(); }
void Gfx_SetupDL_25Xlu(GraphicsContext* gfx) { abort(); }
Gfx* Gfx_TexScrollEx(GraphicsContext* gfx, u32 x, u32 y, s32 width, s32 height, s32 dx, s32 dy) { abort(); }
void gSPSegment(void* value, int segment, uintptr_t target) { abort(); }
Mtx* Matrix_NewMtx(GraphicsContext* gfx, char* file, s32 line) { abort(); }
void gSPDisplayList(Gfx* packet, Gfx* list) { abort(); }

static PlayState* NewPlay(Camera* camera) {
    PlayState* play = calloc(1, sizeof(*play));
    CHECK(play != NULL);
    play->sceneNum = SCENE_BOMBCHU_BOWLING_ALLEY;
    play->gameplayFrames = 1;
    play->cameraPtrs[CAM_ID_MAIN] = camera;
    play->activeCamera = CAM_ID_MAIN;
    targetCount = 0;
    failNextSpawn = 0;
    return play;
}

static void NewWall(BgBowlWall* wall, PlayState* play, s16 params) {
    memset(wall, 0, sizeof(*wall));
    wall->dyna.actor.params = params;
    wall->dyna.actor.world.pos = (Vec3f){ 10, 25, -400 };
    wall->dyna.actor.update = BgBowlWall_Update;
    BgBowlWall_Init(&wall->dyna.actor, play);
}

static void HitTarget(EnWallTubo* target, PlayState* play, EnBomBowlMan* lady, s16 params) {
    Camera* camera = play->cameraPtrs[CAM_ID_MAIN];
    EnBomChu chu = { 0 };
    chu.actor.id = ACTOR_EN_BOM_CHU;
    chu.actor.world.pos = target->actor.world.pos;
    chu.timer = 100;
    play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].head = &chu.actor;
    // A found host at rest must not trigger the targets.
    lady->minigamePlayStatus = 0;
    camera->setting = CAM_SET_CHU_BOWLING;
    EnWallTubo_Update(&target->actor, play);
    CHECK(lady->wallStatus[params] == 0 && chu.timer == 100);
    // An active round still requires the bowling camera.
    lady->minigamePlayStatus = 2;
    camera->setting = CAM_SET_NORMAL0;
    EnWallTubo_Update(&target->actor, play);
    CHECK(lady->wallStatus[params] == 0 && chu.timer == 100);
    camera->setting = CAM_SET_CHU_BOWLING;
    EnWallTubo_Update(&target->actor, play);
    CHECK(lady->wallStatus[params] == 1 && chu.timer == 2 && target->timer == 60);
    play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].head = NULL;
}

static void BullseyeWaits(void) {
    for (s16 params = 0; params < 2; ++params) {
        Camera camera = { 0 };
        PlayState* play = NewPlay(&camera);
        EnWallTubo target = { 0 };
        EnBomBowlMan lady = { 0 };
        Actor other = { 0 };
        target.actor.params = params;
        EnWallTubo_Init(&target.actor, play);
        // Both an empty list and a list without the required NPC stay safe.
        for (int i = 0; i < 120; ++i)
            EnWallTubo_Update(&target.actor, play);
        other.id = ACTOR_EN_NIW;
        play->actorCtx.actorLists[ACTORCAT_NPC].head = &other;
        for (int i = 0; i < 120; ++i)
            EnWallTubo_Update(&target.actor, play);
        CHECK(target.chuGirl == NULL && target.timer == 0);
        // Late lookup retries must resume normal hit detection, not kill the target.
        lady.actor.id = ACTOR_EN_BOM_BOWL_MAN;
        other.next = &lady.actor;
        EnWallTubo_Update(&target.actor, play);
        CHECK(target.chuGirl == &lady);
        HitTarget(&target, play, &lady, params);
        free(play);
    }
}

static void RunRound(BgBowlWall* wall, PlayState* play, EnBomBowlMan* lady, s16 params) {
    CHECK(targetCount == 1 && wall->chuGirl == lady);
    EnWallTubo* target = &targets[0];
    EnWallTubo_Update(&target->actor, play);
    CHECK(target->chuGirl == lady);
    HitTarget(target, play, lady, params);
    for (int i = 0; i < 60; ++i)
        EnWallTubo_Update(&target->actor, play);
    CHECK(wall->isHit && target->actor.update == NULL);
    for (int i = 0; i < 240 && lady->wallStatus[params] != 2; ++i)
        BgBowlWall_Update(&wall->dyna.actor, play);
    CHECK(lady->wallStatus[params] == 2);
    CHECK(wall->dyna.actor.world.pos.y == wall->initPos.y - 450.0f);
    for (int i = 0; i < 10; ++i)
        BgBowlWall_Update(&wall->dyna.actor, play);
    CHECK(targetCount == 1);
    lady->wallStatus[params] = 0;
    for (int i = 0; i < 240 && targetCount != 2; ++i)
        BgBowlWall_Update(&wall->dyna.actor, play);
    CHECK(targetCount == 2 && !wall->isHit);
    CHECK(wall->dyna.actor.world.pos.y == wall->initPos.y);
    for (int i = 0; i < 30; ++i)
        BgBowlWall_Update(&wall->dyna.actor, play);
    CHECK(targetCount == 2);
}

static void WallWaits(void) {
    for (s16 params = 0; params < 2; ++params) {
        Camera camera = { 0 };
        PlayState* play = NewPlay(&camera);
        BgBowlWall wall;
        EnBomBowlMan lady = { 0 };
        Actor other = { 0 };
        NewWall(&wall, play, params);
        for (int i = 0; i < 120; ++i) {
            BgBowlWall_Update(&wall.dyna.actor, play);
            CHECK(targetCount == 0);
        }
        other.id = ACTOR_EN_NIW;
        play->actorCtx.actorLists[ACTORCAT_NPC].head = &other;
        for (int i = 0; i < 120; ++i) {
            BgBowlWall_Update(&wall.dyna.actor, play);
            CHECK(targetCount == 0);
        }
        CHECK(wall.chuGirl == NULL && wall.dyna.actor.world.pos.y == wall.initPos.y);
        lady.actor.id = ACTOR_EN_BOM_BOWL_MAN;
        other.next = &lady.actor;
        BgBowlWall_Update(&wall.dyna.actor, play);
        for (int i = 0; i < 30; ++i)
            BgBowlWall_Update(&wall.dyna.actor, play);
        RunRound(&wall, play, &lady, params);
        free(play);
    }
}

static void PresentHostRound(void) {
    for (s16 params = 0; params < 2; ++params) {
        Camera camera = { 0 };
        PlayState* play = NewPlay(&camera);
        BgBowlWall wall;
        EnBomBowlMan lady = { 0 };
        lady.actor.id = ACTOR_EN_BOM_BOWL_MAN;
        play->actorCtx.actorLists[ACTORCAT_NPC].head = &lady.actor;
        NewWall(&wall, play, params);
        failNextSpawn = 1;
        BgBowlWall_Update(&wall.dyna.actor, play);
        CHECK(targetCount == 0);
        BgBowlWall_Update(&wall.dyna.actor, play);
        RunRound(&wall, play, &lady, params);
        free(play);
    }
}

int main(int argc, char** argv) {
    CHECK(argc == 2);
    if (!strcmp(argv[1], "bullseye-wait"))
        BullseyeWaits();
    else if (!strcmp(argv[1], "wall-wait"))
        WallWaits();
    else if (!strcmp(argv[1], "round"))
        PresentHostRound();
    else
        return 2;
    printf("PASS native Bowling dependency: %s (both walls)\n", argv[1]);
    return 0;
}
