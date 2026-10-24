#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "global.h"
#if __has_include("2s2h/Enhancements/Camera/ComboForestOcarina.h")
#include "2s2h/Enhancements/Camera/ComboForestOcarina.h"
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

SaveContext gSaveContext{};
PlayState *gPlayState;
GameState *gGameState;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static PlayState play;
static Player player;
static int letterbox, hud;
namespace SOH {
struct Scene {
  ActorEntry *spawn;
  ActorCutscene *cutscenes = nullptr;
  s16 count = 0;
};
} // namespace SOH
extern "C" void Object_InitContext(GameState *, ObjectContext *) {}
extern "C" void LightContext_Init(PlayState *, LightContext *) {}
extern "C" void Scene_ResetTransitionActorList(GameState *,
                                               TransitionActorList *) {}
extern "C" void Room_Init(PlayState *, RoomContext *) {}
extern "C" void Play_InitEnvironment(PlayState *, s16) {}
s32 OTRScene_ExecuteCommands(PlayState *state, SOH::Scene *scene) {
  state->linkActorEntry = scene->spawn;
#ifdef COMBO_BUILD
  ComboOpeningForest_PrepareSpawn(state);
#endif
  if (scene->cutscenes)
    CutsceneManager_Init(state, scene->cutscenes, scene->count);
  return 0;
}
extern "C" Actor *Actor_SpawnAsChildAndCutscene(ActorContext *, PlayState *,
                                                s16 id, f32 x, f32 y, f32 z,
                                                s16 rx, s16 ry, s16 rz,
                                                s32 params, u32 csId, u32,
                                                Actor *) {
  Actor *actor = &player.actor;
  actor->id = id;
  actor->world.pos = actor->home.pos = {x, y, z};
  actor->shape.rot = actor->home.rot = {rx, ry, rz};
  actor->params = params;
  /* CS_ID_NORMALIZATION */
  return actor;
}
extern "C" void Interface_SetHudVisibility(u16 value) { hud = value; }
extern "C" void ShrinkWindow_Letterbox_SetSizeTarget(s32 value) {
  letterbox = value;
}
extern "C" void Audio_PlaySfx(u16) {}
extern "C" s32 Player_SetCsActionWithHaltedActors(PlayState *, Actor *, u8) {
  return true;
}
extern "C" void Cutscene_StartScripted(PlayState *, u8) { assert(false); }
extern "C" void func_800E0348(Camera *) { assert(false); }
extern "C" u16 BgCheck_GetBgCamSettingImpl(CollisionContext *, u32, s32) {
  assert(false);
  return 0;
}
extern "C" f32 Math_FAtan2F(f32 y, f32 x) { return std::atan2(y, x); }
extern "C" f32 BgCheck_CameraRaycastFloor2(CollisionContext *, CollisionPoly **,
                                           s32 *, Vec3f *) {
  return BGCHECK_Y_MIN;
}
// The fixture supplies a clear collision volume to the native ocarina handler.
extern "C" s16 func_800CC260(Camera *, Vec3f *eye, Vec3f *at, VecGeo *offset,
                             Actor **, s16) {
  *eye = OLib_AddVecGeoToVec3f(at, offset);
  return 0;
}
extern "C" s32 func_800CBFA4(Camera *, Vec3f *, Vec3f *, s16) { return 0; }
#define GameInteractor_Should(...) true
#define GameInteractor_ExecuteOnCameraChangeSettingsFlags(...) ((void)0)
#define GameInteractor_ExecuteOnCameraChangeModeFlags(...) ((void)0)
#define __osMemset std::memset
#define RELOAD_PARAMS(camera)                                                  \
  ((camera)->animState == 0 || (camera)->animState == 10 ||                    \
   (camera)->animState == 20)
#define CAM_RODATA_UNSCALE(x) ((x)*.01f)
#define GET_NEXT_RO_DATA(values) ((values++)->val)
#define GET_NEXT_SCALED_RO_DATA(values)                                        \
  CAM_RODATA_UNSCALE(GET_NEXT_RO_DATA(values))
#define CAM_DATA_IS_BG (1 << 12)
#define CAM_CHANGE_SETTING_0 (1 << 0)
#define CAM_CHANGE_SETTING_1 (1 << 1)
#define CAM_CHANGE_SETTING_2 (1 << 2)
#define CAM_CHANGE_SETTING_3 (1 << 3)
#define CAM_CHANGE_MODE_0 (1 << 0)
#define CAM_CHANGE_MODE_1 (1 << 1)
#define CAM_CHANGE_MODE_BATTLE (1 << 2)
#define CAM_CHANGE_MODE_FOLLOW_TARGET (1 << 3)
#define CAM_CHANGE_MODE_4 (1 << 4)
#define CAM_CHANGE_MODE_FIRST_PERSON (1 << 5)
PlayState *sCamPlayState;
s16 sCameraNextUID;
s32 sCameraInterfaceFlags, sCameraHudVisibility, sCameraLetterboxSize,
    sCameraInitSceneTimer;
static bool sUpdateCameraDirection;
static Vec3f D_801EDDD0, D_801EDDE0, D_801EDDF0;
/* NATIVE CODE */

static void Require(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}
static ActorEntry combinedSpawn = /* COMBINED SPAWN */;
static ActorEntry soloSpawn = /* SOLO SPAWN */;
// Native Lost Woods command 0x1B, common unscripted player chain, entries 0..8.
// The immutable mm.o2r has this chain before its two prologue script entries.
static const std::array<ActorCutscene, 9> nativeChain{{
    {700, -1, -3, -1, 1, 0, 255, 0, 0, 27},
    {600, -1, -2, -1, 2, 0, 255, 0, 0, 27},
    {700, -1, -4, -1, 3, 0, 255, 0, 0, 27},
    {700, -1, -5, -1, 4, 0, 255, 0, 0, 27},
    {500, -1, -7, -1, 5, 0, 255, 0, 0, 32},
    {400, -1, -11, -1, 6, 0, 255, 0, 1, 32},
    {100, -1, -8, -1, 7, 0, 255, 0, 0, 32},
    {200, -1, -9, -1, 8, 0, 255, 0, 0, 32},
    {800, -1, -6, -1, -1, 0, 255, 0, 0, 32},
}};
static void Reset(int mode = COMBO_GAME_BOTH, s16 scene = SCENE_LOST_WOODS) {
  play = {};
  player = {};
  gSaveContext = {};
  gPlayState = &play;
  gGameState = &play.state;
  gComboGameMode = mode;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  gSaveContext.save.entrance = scene == COMBO_SOLO_FOREST_MM_SCENE
                                   ? COMBO_SOLO_FOREST_MM_ENTRANCE
                                   : ENTRANCE(LOST_WOODS, 0);
  play.sceneId = scene;
  play.state.running = true;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  player.transformation = PLAYER_FORM_HUMAN;
  player.actor.scale = {.01f, .01f, .01f};
  sCameraNextUID = 0;
}
static void Load(SOH::Scene &scene, s32 spawn = 0) {
  static_assert(sizeof(ActorEntry) == 16);
  static_assert(sizeof(ActorCutscene) == 16 &&
                offsetof(ActorCutscene, csCamId) == 4 &&
                offsetof(ActorCutscene, additionalCsId) == 8 &&
                offsetof(ActorCutscene, letterboxSize) == 15);
  play.sceneSegment = &scene;
  const ActorEntry original = *scene.spawn;
  play.mainCamera.eye = {31, 41, 51};
  play.mainCamera.fov = 53;
  const Camera untouched = play.mainCamera;
  OTRPlay_InitScene(&play, spawn);
  Require(!std::memcmp(scene.spawn, &original, sizeof(original)),
          "scene resource spawn remains byte-identical");
  Require(!std::memcmp(&play.mainCamera, &untouched, sizeof(untouched)),
          "metadata preparation does not reset a camera");
  Actor_SpawnEntry(&play.actorCtx, play.linkActorEntry, &play);
  Play_AssignPlayerCsIdsFromScene(&play, player.actor.csId);
  Require(play.linkActorEntry->id == original.id &&
              !std::memcmp(&play.linkActorEntry->pos, &original.pos,
                           sizeof(Vec3s)) &&
              play.linkActorEntry->rot.x == original.rot.x &&
              play.linkActorEntry->rot.z == original.rot.z &&
              (play.linkActorEntry->rot.y & ~0x7F) ==
                  (original.rot.y & ~0x7F) &&
              play.linkActorEntry->params == original.params,
          "metadata preparation preserves spawn pose/params/day bits");
  Camera_Init(&play.mainCamera, &play.view, &play.colCtx, &play);
  play.mainCamera.camId = CAM_ID_MAIN;
  play.mainCamera.setting = CAM_SET_NORMAL0;
  play.mainCamera.focalActor = &player.actor;
  play.mainCamera.focalActorPosRot = player.actor.world;
  play.mainCamera.focalActorPosRot.rot = player.actor.shape.rot;
  play.mainCamera.eye = play.mainCamera.eyeNext = {50, 100, 260};
  play.mainCamera.at = {0, 56, 0};
  play.mainCamera.dist = 268;
  play.cameraPtrs[CAM_ID_MAIN] = &play.mainCamera;
  Play_ChangeCameraStatus(&play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
}
struct OcarinaView {
  Vec3f eye, at;
  f32 fov;
  s16 setting, mode;
};
static OcarinaView PlayAndCancel(int form) {
  player.transformation = form;
  const s16 csId = play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA];
  Require(csId > CS_ID_NONE,
          "playable forest supplies a native ocarina cutscene ID");
  Require(!func_808323C0(&player, csId),
          "first ocarina request queues through native priority arbitration");
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  Require(CutsceneManager_IsNext(csId) == 1,
          "native manager promotes the ocarina request");
  Require(func_808323C0(&player, csId),
          "second request starts the native ocarina cutscene");
  Camera *camera = Play_GetCamera(&play, Play_GetActiveCamId(&play));
  Require(camera != &play.mainCamera &&
              play.mainCamera.status == CAM_STATUS_WAIT &&
              camera->setting == CAM_SET_ITEM1 &&
              camera->bgCamIndex == CS_CAM_ID_GLOBAL_ITEM_OCARINA,
          "native cutscene starts an ITEM1 ocarina subcamera");
  Require(letterbox == 27 && hud == HUD_VISIBILITY_NONE_ALT,
          "native ocarina HUD and letterbox metadata apply");
  for (int frame = 0; frame < 20; ++frame) {
    Camera_KeepOn4(camera);
    CutsceneManager_Update();
    Require(CutsceneManager_GetCurrentCsId() == csId,
            "ocarina cutscene remains active until native cancellation");
  }
  Require(std::isfinite(camera->eye.x) && std::isfinite(camera->eye.y) &&
              std::isfinite(camera->eye.z) && camera->eye.z != 260 &&
              camera->fov > 60,
          "native KeepOn4 creates the playing view");
  OcarinaView result{camera->eye, camera->at, camera->fov, camera->setting,
                     camera->mode};
  // This is the native stop call used by Player_Action_63 for B/cancel.
  Require(CutsceneManager_Stop(csId) == csId,
          "ocarina cancel schedules native cutscene cleanup");
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  Require(CutsceneManager_GetCurrentCsId() == CS_ID_NONE &&
              play.activeCamId == CAM_ID_MAIN &&
              play.mainCamera.status == CAM_STATUS_ACTIVE &&
              play.cameraPtrs[1] == nullptr &&
              play.mainCamera.setting == CAM_SET_NORMAL0,
          "cancel returns to the native main camera");
  return result;
}
static void SameView(const OcarinaView &a, const OcarinaView &b) {
  Require(!std::memcmp(&a.eye, &b.eye, sizeof(Vec3f)) &&
              !std::memcmp(&a.at, &b.at, sizeof(Vec3f)) && a.fov == b.fov &&
              a.setting == b.setting && a.mode == b.mode,
          "forest ocarina view matches the native scene across player forms");
}
int main() {
  InitNativeCameraTable();
  for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
    Reset();
    ActorEntry nativeSpawn = combinedSpawn;
    nativeSpawn.rot.y &= ~0x7F;
    auto chain = nativeChain;
    SOH::Scene nativeScene{&nativeSpawn, chain.data(), (s16)chain.size()};
    Load(nativeScene);
    Require(sSceneCutsceneList == chain.data() && sSceneCutsceneCount == 9 &&
                chain[0].priority == 550,
            "authored native cutscene chains are retained");
    const OcarinaView control = PlayAndCancel(form);
#ifdef COMBO_BUILD
    for (int visit = 0; visit < 3; ++visit) {
      for (bool solo : {false, true}) {
        Reset(solo ? COMBO_GAME_MM : COMBO_GAME_BOTH,
              solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS);
        SOH::Scene forest{solo ? &soloSpawn : &combinedSpawn};
        Load(forest);
        Require(play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] > CS_ID_NONE,
                "playable forest supplies a native ocarina cutscene ID");
        Require(
            sSceneCutsceneCount == 1 &&
                sSceneCutsceneList[0].scriptIndex == CS_SCRIPT_ID_NONE &&
                sSceneCutsceneList[0].additionalCsId == CS_ID_NONE &&
                sSceneCutsceneList[0].priority == 550,
            "only the missing native unscripted ocarina metadata is supplied");
        ActorCutscene expected = nativeChain[0];
        expected.additionalCsId = CS_ID_NONE;
        expected.priority =
            550; // Native Play_AssignPlayerCsIdsFromScene normalization.
        Require(
            !std::memcmp(&sSceneCutsceneList[0], &expected, sizeof(expected)),
            "native ocarina camera/script/priority/timing/HUD/end fields are "
            "retained");
        for (int id = 1; id < PLAYER_CS_ID_MAX; ++id)
          Require(
              play.playerCsIds[id] == CS_ID_NONE,
              "unrelated missing player cutscene IDs retain their behavior");
        SameView(PlayAndCancel(form), control);
      }
    }
#endif
  }
  for (int guard = 0; guard < 12; ++guard) {
    Reset();
    ActorEntry spawn = combinedSpawn;
    if (guard == 0)
      gComboGameMode = COMBO_GAME_OOT;
    if (guard == 1)
      play.sceneId = SCENE_TOWN;
    if (guard == 2)
      gSaveContext.sceneLayer = 1;
    if (guard == 3)
      gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 1);
    if (guard == 4)
      gSaveContext.save.cutsceneIndex = 0xFFF0;
    if (guard == 5)
      gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
    if (guard == 6)
      gSaveContext.fileNum = 0xFF;
    if (guard == 7)
      spawn.rot.y &= ~0x7F;
    if (guard == 8) {
      gComboGameMode = COMBO_GAME_MM;
      play.sceneId = SCENE_26SARUNOMORI;
    }
    if (guard == 10) {
      gComboGameMode = COMBO_GAME_MM;
      play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    }
    if (guard == 11) {
      play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
      gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    }
    SOH::Scene scene{&spawn};
    play.sceneSegment = &scene;
    OTRPlay_InitScene(&play, guard == 9 ? 1 : 0);
#ifdef COMBO_BUILD
    Require(
        sSceneCutsceneCount == 0,
        "nonexploration, other scenes and authored spawn chains are unchanged");
#endif
  }
#ifdef COMBO_BUILD
  Reset();
  ActorCutscene authored{600,
                         -1,
                         CS_CAM_ID_GLOBAL_ITEM_GET,
                         CS_SCRIPT_ID_NONE,
                         CS_ID_NONE,
                         0,
                         255,
                         0,
                         0,
                         27};
  SOH::Scene authoredScene{&combinedSpawn, &authored, 1};
  Load(authoredScene);
  Require(sSceneCutsceneCount == 1 && sSceneCutsceneList == &authored &&
              play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] == CS_ID_NONE,
          "an authored nonempty scene list is never replaced to add an ocarina "
          "fallback");
#endif
#ifdef COMBO_BUILD
  std::puts("PASS combined/MM-solo forest native ocarina queue, ITEM1/KeepOn4 "
            "camera and cancel across five forms/re-entry; native/SoH/intro "
            "guards and spawn bytes preserved");
#else
  std::puts("PASS standalone MM native ocarina flow unchanged across five "
            "forms; forest adapter excluded");
#endif
}
