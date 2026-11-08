/* Runner inserts unmodified production functions. External archive/allocator,
 * object-bank, animation sampling and renderer boundaries are controlled. */
/* PRODUCTION_INCLUDES */
#include "tests/test_require.h"
#include "vt.h"
#include <stdlib.h>

uintptr_t gSegments[16];
static GameInfo gameInfo;
GameInfo *gGameInfo = &gameInfo;
static unsigned allocations, frees, registrations, unregisters, dependencies,
    bankRequests, killed;
static unsigned interactions, collisions, draws, billboards, matrixPushes,
    matrixPops, frameUpdates;
static bool objectReady, replacement;
static int expectedObject;
static const char *expectedSkeleton;
static const char *expectedAnimation;
static FlexSkeletonHeader loaded[16];
static void *limbs[16][28];
static StandardLimb limbData[16][28];
static unsigned loadedCount;
static Gfx commands[256], translucent[1], meshes[16][28][1];
static Mtx matrix;
static EnViewer *drawing;
static ColliderCylinderInit sStaticCylinderInit;
void EnViewerStatic_Update(EnViewer *, PlayState *);
void EnViewerStatic_WaitForObjects(EnViewer *, PlayState *);
void EnViewer_SetupAction(EnViewer *actor, EnViewerActionFunc action) {
  actor->actionFunc = action;
}
void ActorCatalogue_LogLifecycle(const char *s, int a, int b, int c, int d,
                                 int e, int f, int g) {}
void lusprintf(const char *file, int32_t line, int32_t level, const char *fmt,
               ...) {}
void osSyncPrintfUnused(const char *fmt, ...) {}
void FrameInterpolation_RecordOpenChild(const void *a, int b) {}
void FrameInterpolation_RecordCloseChild(void) {}
void *ZeldaArena_MallocDebug(size_t size, const char *file, s32 line) {
  ++allocations;
  return calloc(1, size);
}
void ZeldaArena_FreeDebug(void *pointer, const char *file, s32 line) {
  ++frees;
  free(pointer);
}
int ResourceMgr_OTRSigCheck(char *path) {
  return memcmp(path, "__OTR__", 7) == 0;
}
SkeletonHeader *ResourceMgr_LoadSkeletonByName(const char *path,
                                               SkelAnime *animation) {
  REQUIRE(strcmp(path, expectedSkeleton) == 0);
  unsigned index = loadedCount++;
  REQUIRE(index < 16);
  unsigned count = expectedObject == OBJECT_FR ? 23 : 17;
  if (replacement)
    count += 4;
  loaded[index].sh.segment = limbs[index];
  loaded[index].sh.limbCount = count;
  loaded[index].sh.skeletonType = SKELANIME_TYPE_FLEX;
  loaded[index].dListCount = count;
  for (unsigned limb = 0; limb < count; ++limb) {
    limbData[index][limb].dList = meshes[index][limb];
    limbData[index][limb].child = LIMB_DONE;
    limbData[index][limb].sibling = LIMB_DONE;
    limbs[index][limb] = &limbData[index][limb];
  }
  ++registrations;
  return &loaded[index].sh;
}
void ResourceMgr_UnregisterSkeleton(SkelAnime *animation) {
  REQUIRE(animation->skeletonHeader != NULL);
  ++unregisters;
}
uint8_t ResourceMgr_FileExists(const char *path) { return 0; }
bool ResourceMgr_IsAltAssetsEnabled(void) { return replacement; }
void Animation_PlayLoop(SkelAnime *animation, AnimationHeader *header) {
  REQUIRE(false);
}
void Animation_PlayLoopSetSpeed(SkelAnime *animation, AnimationHeader *header,
                                float speed) {
  REQUIRE(strcmp((char *)header, expectedAnimation) == 0);
  REQUIRE(speed == 1.0f);
  animation->animation = header;
  animation->playSpeed = speed;
}
s32 SkelAnime_Init(PlayState *p, SkelAnime *s, SkeletonHeader *h,
                   AnimationHeader *a, Vec3s *j, Vec3s *m, s32 n) {
  REQUIRE(false);
  return 0;
}
s32 SkelAnime_Update(SkelAnime *animation) {
  ++frameUpdates;
  animation->curFrame += 1.0f;
  return 0;
}
void Skin_Free(PlayState *p, Skin *s) { REQUIRE(false); }
s32 Object_GetIndex(ObjectContext *context, s16 id) {
  REQUIRE(id == expectedObject);
  return -1;
}
s32 Object_Spawn(ObjectContext *context, s16 id) {
  REQUIRE(id == expectedObject);
  ++bankRequests;
  return 2;
}
s32 Object_IsLoaded(ObjectContext *context, s32 slot) {
  REQUIRE(slot == 2);
  return objectReady;
}
void Actor_SetObjectDependency(PlayState *play, Actor *actor) {
  REQUIRE(actor->objBankIndex == 2);
  ++dependencies;
}
void Actor_Kill(Actor *actor) { ++killed; }
void Actor_SetScale(Actor *actor, float scale) {
  actor->scale = (Vec3f){scale, scale, scale};
}
void Actor_SetFocus(Actor *actor, float height) {
  actor->focus.pos = actor->world.pos;
  actor->focus.pos.y += height;
}
void ActorShape_Init(ActorShape *shape, f32 y, ActorShadowFunc draw,
                     f32 radius) {
  shape->yOffset = y;
}
void ActorShadow_DrawCircle(Actor *actor, Lights *lights, PlayState *play) {}
s32 Collider_InitCylinder(PlayState *p, ColliderCylinder *c) { return 1; }
s32 Collider_SetCylinder(PlayState *p, ColliderCylinder *c, Actor *a,
                         ColliderCylinderInit *i) {
  return 1;
}
s32 Collider_DestroyCylinder(PlayState *p, ColliderCylinder *c) { return 1; }
void Collider_UpdateCylinder(Actor *a, ColliderCylinder *c) { ++collisions; }
s32 CollisionCheck_SetOC(PlayState *p, CollisionCheckContext *c, Collider *a) {
  ++collisions;
  return 0;
}
static void EnViewerStatic_UpdateTracking(EnViewer *actor, PlayState *play) {
  ++interactions;
}
void EnViewerStatic_OfferTalk(EnViewer *actor, PlayState *play) {
  ++interactions;
}
static void EnViewerStatic_InitRutoWater(EnViewer *a, PlayState *p,
                                         const StaticStoryPoseDescriptor *d) {
  REQUIRE(false);
}
static bool EnViewerStatic_UpdateRutoWater(EnViewer *a, PlayState *p,
                                           bool ended) {
  REQUIRE(false);
  return false;
}
static void EnViewerStatic_SetDaruniaDanceStep(EnViewer *a, u8 step) {
  REQUIRE(false);
}
int StaticStoryKokiri_RequestObjects(EnViewer *a, PlayState *p) {
  REQUIRE(false);
  return false;
}
void StaticStoryKokiri_Init(EnViewer *a, PlayState *p) { REQUIRE(false); }
void StaticStoryKokiri_UpdatePose(EnViewer *a, float step) { REQUIRE(false); }
s16 Rand_S16Offset(s16 base, s16 range) { return base; }
f32 Math_SinS(s16 angle) { return 0; }
s16 Math_SmoothStepToS(s16 *value, s16 target, s16 scale, s16 step, s16 min) {
  REQUIRE(false);
  return 0;
}
bool StaticRutoWater_ShouldTurnBody(const StaticRutoWaterState *state) {
  REQUIRE(false);
  return false;
}
s32 MmSfx_PlayAtPos(u16 id, Vec3f *pos) {
  REQUIRE(false);
  return 0;
}
void MmSfx_StopAtPos(u16 id, Vec3f *pos) { REQUIRE(false); }
void MmAssets_Init(void) { REQUIRE(false); }
bool MmAssets_LoadNormalActor(int type, unsigned char pose,
                              MmNormalActorResources *out) {
  REQUIRE(false);
  return false;
}
bool MmAssets_LoadKafei(unsigned char pose, MmNormalActorResources *out) {
  REQUIRE(false);
  return false;
}
void MmAssets_ReleaseNormalActor(void *owner) { REQUIRE(false); }
void *MmAssets_LoadSkeleton(const char *path) {
  REQUIRE(false);
  return NULL;
}
void *MmAssets_LoadAnimation(const char *path) {
  REQUIRE(false);
  return NULL;
}
void *MmAssets_LoadResource(const char *path) {
  REQUIRE(false);
  return NULL;
}
Gfx *MmAssets_LoadDisplayListGraphStrict(const char *path) {
  REQUIRE(false);
  return NULL;
}
const MmAnjuDisplayLists *MmAssets_GetAnjuDisplayLists(uint8_t pose) {
  REQUIRE(false);
  return NULL;
}
const MmSkullKidDisplayLists *MmAssets_GetSkullKidDisplayLists(void) {
  REQUIRE(false);
  return NULL;
}
static bool EnViewer_StaticSelectSkullKidModel(EnViewer *actor) {
  REQUIRE(false);
  return false;
}
/* PRODUCTION_NATIVE_LIFETIME */
/* PRODUCTION_INIT */

void Graph_OpenDisps(Gfx **d, GraphicsContext *g, const char *file, s32 line) {}
void Graph_CloseDisps(Gfx **d, GraphicsContext *g, const char *file, s32 line) {
}
void Gfx_SetupDL_25Opa(GraphicsContext *gfx) {}
void gSPSegment(void *value, int segment, uintptr_t target) {
  __gSPSegment((Gfx *)value, segment, target);
}
void gSPDisplayList(Gfx *command, Gfx *displayList) {
  __gSPDisplayList(command, displayList);
}
void Matrix_Push(void) { ++matrixPushes; }
void Matrix_Pop(void) { ++matrixPops; }
void Matrix_ReplaceRotation(MtxF *rotation) { ++billboards; }
Mtx *Matrix_NewMtx(GraphicsContext *g, char *file, s32 line) { return &matrix; }
static uintptr_t lastCommand(PlayState *play, u32 instruction) {
  uintptr_t value = 0;
  for (Gfx *command = commands; command < play->state.gfxCtx->polyOpa.p;
       ++command)
    if (command->words.w0 == instruction)
      value = command->words.w1;
  return value;
}
static unsigned listCount(PlayState *play, const void *list) {
  unsigned count = 0;
  for (Gfx *command = commands; command < play->state.gfxCtx->polyOpa.p;
       ++command)
    if ((command->words.w0 >> 24) == G_DL &&
        command->words.w1 == (uintptr_t)list)
      ++count;
  return count;
}
/* This renderer boundary follows native FLEX semantics: post receives the
 * original loaded limb DL even when override suppresses the ordinary draw. */
void SkelAnime_DrawSkeletonOpa(PlayState *play, SkelAnime *animation,
                               OverrideLimbDrawOpa override,
                               PostLimbDrawOpa post, void *arg) {
  REQUIRE(arg == drawing && animation == &drawing->skin.skelAnime &&
          post != NULL);
  ++draws;
  if (drawing->staticState.type == STATIC_STORY_ACTOR_DAMPE_GHOST)
    REQUIRE(lastCommand(play, 0xFB000000) == 0xFFFFFFFF);
  for (unsigned index = 0; index < animation->limbCount - 1; ++index) {
    Gfx *original = ((StandardLimb *)animation->skeleton[index])->dList;
    Gfx *mesh = original;
    Vec3f position = {0};
    Vec3s rotation = {0};
    if (override)
      REQUIRE(!override(play, index + 1, &mesh, &position, &rotation, arg));
    REQUIRE(mesh ==
            ((override && (index == 6 || index == 7)) ? NULL : original));
    unsigned prior = listCount(play, original);
    post(play, index + 1, &original, &rotation, arg);
    REQUIRE(listCount(play, original) ==
            prior + (override && (index == 6 || index == 7)));
  }
}
/* UNRELATED_DRAW_BOUNDARIES */
/* PRODUCTION_DRAW */

int main(void) {
  static PlayState play;
  GraphicsContext graphics = {0};
  play.state.gfxCtx = &graphics;
  play.objectCtx.status[2].segment = (void *)0x80001000;
  const u32 nativeColors[] = {0xC8AA00FF, 0x00AAC8FF, 0xD27864FF, 0x7882E6FF,
                              0xBEBEBEFF};
  const char *frogEyes[] = {object_fr_Tex_0059A0, object_fr_Tex_005BA0};
  const char *ghostEyes[] = {gDampeEyeOpenTex, gDampeEyeHalfTex,
                             gDampeEyeClosedTex};
  for (unsigned alt = 0; alt < 2; ++alt) {
    EnViewer actors[8] = {0};
    replacement = alt;
    for (unsigned index = 0; index < 8; ++index) {
      EnViewer *actor = &actors[index];
      unsigned entry = index < 6 ? index : (index == 6 ? 0 : 5);
      expectedObject = entry < 5 ? OBJECT_FR : OBJECT_TK;
      expectedSkeleton = entry < 5 ? object_fr_Skel_00B498 : gDampeSkel;
      expectedAnimation = entry < 5 ? object_fr_Anim_001534 : gDampeFloatAnim;
      actor->actor.params = 0x7D04 + entry;
      actor->actor.world.pos = actor->actor.home.pos = (Vec3f){123, 456, 789};
      actor->actor.shape.rot = actor->actor.world.rot =
          (Vec3s){0x2222, -0x1234, 0x3333};
      actor->actor.flags =
          ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
      actor->staticState.staticMode = true;
      objectReady = false;
      EnViewerStatic_Init(actor, &play);
      REQUIRE(actor->staticState.type == 29 + entry &&
              actor->staticState.pose == 0);
      REQUIRE(actor->actor.world.rot.x == 0 && actor->actor.world.rot.z == 0);
      REQUIRE(actor->actor.world.rot.y == -0x1234 &&
              actor->actor.shape.rot.y == -0x1234);
      actor->actionFunc(actor, &play);
      REQUIRE(!actor->staticState.initialized && !actor->isVisible);
      objectReady = true;
      actor->actionFunc(actor, &play);
      REQUIRE(actor->staticState.initialized && actor->isVisible &&
              actor->actor.objBankIndex == 2);
      REQUIRE(actor->skin.skelAnime.limbCount ==
              (entry < 5 ? 24 : 18) + (alt ? 4 : 0));
      REQUIRE(actor->skin.skelAnime.skeletonHeader ==
              &loaded[alt * 8 + index].sh);
      REQUIRE(actor->skin.skelAnime.skeleton == limbs[alt * 8 + index]);
      REQUIRE(actor->actor.scale.x == (entry < 5 ? .015f : .01f));
      REQUIRE(!(actor->actor.flags &
                (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY)));
      actor->staticState.blinkTimer = 0;
      actor->staticState.eyeIndex = entry < 5 ? 1 : 2;
      actor->actionFunc(actor, &play);
      REQUIRE(actor->staticState.eyeIndex == 0 &&
              actor->skin.skelAnime.curFrame == 1.0f);
      REQUIRE(actor->staticState.blinkTimer == (entry < 5 ? 20 : 0));
      REQUIRE(interactions == 0 && collisions == 0 && killed == 0);
      for (unsigned eye = 0; eye < 4; ++eye) {
        actor->staticState.eyeIndex = eye;
        graphics.polyOpa.p = commands;
        graphics.polyXlu.p = translucent;
        gSPSegment(graphics.polyOpa.p++, 8, (uintptr_t) "stale eye");
        gDPSetEnvColor(graphics.polyOpa.p++, 1, 2, 3, 4);
        EnViewer before = *actor;
        StandardLimb beforeLimbs[28];
        memcpy(beforeLimbs, limbData[alt * 8 + index], sizeof(beforeLimbs));
        FlexSkeletonHeader beforeHeader = loaded[alt * 8 + index];
        drawing = actor;
        unsigned priorBillboards = billboards;
        EnViewerStatic_Draw(actor, &play);
        REQUIRE(memcmp(actor, &before, sizeof(before)) == 0);
        REQUIRE(memcmp(beforeLimbs, limbData[alt * 8 + index],
                       sizeof(beforeLimbs)) == 0);
        REQUIRE(memcmp(&beforeHeader, &loaded[alt * 8 + index],
                       sizeof(beforeHeader)) == 0);
        REQUIRE(graphics.polyXlu.p == translucent);
        const char *expectedEye = entry < 5 ? frogEyes[eye < 2 ? eye : 0]
                                            : ghostEyes[eye < 3 ? eye : 0];
        REQUIRE(strcmp((char *)lastCommand(&play, 0xDB060020), expectedEye) ==
                0);
        if (entry < 5) {
          REQUIRE(strcmp((char *)lastCommand(&play, 0xDB060024), expectedEye) ==
                  0);
          REQUIRE(lastCommand(&play, 0xFB000000) == nativeColors[entry]);
          REQUIRE(billboards == priorBillboards + 2);
          REQUIRE(listCount(&play, gDampeLanternDL) == 0);
        } else {
          REQUIRE(lastCommand(&play, 0xFB000000) == 0xF0CDAF80);
          REQUIRE(listCount(&play, gDampeLanternDL) == 1 &&
                  listCount(&play, gDampeHaloDL) == 1);
          REQUIRE(billboards == priorBillboards);
        }
      }
      REQUIRE(actor->actor.world.pos.x == 123 &&
              actor->actor.world.pos.y == 456 &&
              actor->actor.world.pos.z == 789);
      REQUIRE(actor->actor.shape.rot.y == -0x1234);
    }
    /* All six plus duplicate yellow frog and ghost coexist; destroying one must
     * not invalidate the others. */
    for (unsigned index = 0; index < 8; ++index) {
      EnViewer_Destroy(&actors[index].actor, &play);
      REQUIRE(!actors[index].staticState.initialized);
    }
  }
  REQUIRE(allocations == 32 && frees == allocations && registrations == 16 &&
          unregisters == registrations);
  REQUIRE(bankRequests == 16 && dependencies == 16 && frameUpdates == 16 &&
          draws == 64);
  REQUIRE(matrixPushes == matrixPops && billboards == 96);
  puts("PASS independent frogs/Dampe: object wait, loaded replacement "
       "ownership, idle/blink, native draw and cleanup");
}
