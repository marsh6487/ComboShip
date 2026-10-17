// Native layouts and collision-camera math; only floor/line query services,
// hook dispatch, and native camera-handler invocation are engine boundaries.
#include "2s2h/Enhancements/Camera/CameraUtils.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
#include "global.h"
#if __has_include("2s2h/Enhancements/Camera/ComboForestArrivalCamera.h")
#include "2s2h/Enhancements/Camera/ComboForestArrivalCamera.h"
#endif
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <set>
#include <string>
#include <vector>

SaveContext gSaveContext{};
PlayState *gPlayState;
GameState *gGameState;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
s32 sCameraInterfaceFlags;
f32 qNaN0x10000 = 0x7F810000;
}
static SwingAnimation D_801EDC30[4];
static bool sUpdateCameraDirection;
Actor *gCameraDriftActor;
static Vec3f D_801EDDD0;
static CameraMode normalModes[CAM_MODE_MAX];
static CameraSetting sCameraSettings[CAM_SET_MAX];
/* NORMAL_PRESET */
extern "C" void Player_Action_35(Player *, PlayState *) {}
extern "C" s32 Player_SetAction(PlayState *, Player *player,
                                PlayerActionFunc action, s32) {
  player->actionFunc = action;
  return true;
}
extern "C" void func_8082DD2C(PlayState *, Player *) {}
extern "C" PlayerAnimationHeader *Player_GetIdleAnim(Player *) {
  return nullptr;
}
extern "C" void Player_Anim_PlayOnce(PlayState *, Player *,
                                     PlayerAnimationHeader *) {}
extern "C" void func_80835324(PlayState *, Player *, f32, s16);
extern "C" s32 func_8083A878(PlayState *play, Player *player, f32 distance) {
  func_80835324(play, player, distance, player->actor.shape.rot.y);
  return true;
}
static bool inCutscene, floorMissing, floorSteep;
static float collisionFraction;
static int collisionQueries;
static CollisionPoly floorPoly, wallPoly;
extern "C" bool Player_InCsMode(PlayState *) { return inCutscene; }
// Native terrain/collision and alternative camera modes are external services
// for this flat-ground NORMAL0 release check; unexpected branches fail loudly.
extern "C" s16 Camera_GetPitchAdjFromFloorHeightDiffs(Camera *, s16, s16) {
  return 0;
}
extern "C" s32 Camera_CalcAtForNormal1(Camera *, VecGeo *, f32, f32) {
  assert(false);
  return 0;
}
extern "C" s32 Camera_CalcAtForScreen(Camera *, VecGeo *, f32, f32 *, f32) {
  assert(false);
  return 0;
}
extern "C" f32 Camera_ClampDist1(Camera *, f32, f32, f32, s16) {
  assert(false);
  return 0;
}
extern "C" void Camera_CalcDefaultSwing(Camera *camera, VecGeo *, VecGeo *, f32,
                                        f32 rate, SwingAnimation *swing,
                                        s16 *flags) {
  assert(collisionFraction ==
         0); // Native no-obstacle collision service result.
  *flags &= ~0x1000;
  swing->swingUpdateRate = rate;
  camera->eye = camera->eyeNext;
}
extern "C" f32 BgCheck_CameraRaycastFloor2(CollisionContext *, CollisionPoly **,
                                           s32 *, Vec3f *) {
  return BGCHECK_Y_MIN;
}
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState *, CollisionContext *,
                                             CollisionPoly **poly, s32 *bg,
                                             Actor *, Vec3f *) {
  *poly = &floorPoly;
  *bg = BGCHECK_SCENE;
  floorPoly.normal.y = floorSteep ? 0 : 32767;
  return floorMissing ? BGCHECK_Y_MIN : 0.0f;
}
extern "C" s32 BgCheck_CameraLineTest1(CollisionContext *, Vec3f *from,
                                       Vec3f *to, Vec3f *hit,
                                       CollisionPoly **poly, s32 wall,
                                       s32 floor, s32 ceiling, s32 face,
                                       s32 *bg) {
  ++collisionQueries;
  assert(wall && floor && ceiling && face == -1);
  if (collisionFraction <= 0)
    return false;
  hit->x = from->x + (to->x - from->x) * collisionFraction;
  hit->y = from->y + (to->y - from->y) * collisionFraction;
  hit->z = from->z + (to->z - from->z) * collisionFraction;
  *poly = &wallPoly;
  *bg = BGCHECK_SCENE;
  return true;
}
struct GameInteractor {
  struct OnSceneInit {
    static inline std::vector<std::function<void(s8, s8)>> callbacks;
  };
  struct OnPlayDestroy {
    static inline std::vector<std::function<void()>> callbacks;
  };
  struct OnSaveLoad {
    static inline std::vector<std::function<void(s16)>> callbacks;
  };
  struct ShouldVanillaBehavior {
    static inline std::vector<
        std::function<void(GIVanillaBehavior, bool *, va_list)>>
        callbacks;
  };
  static GameInteractor *Instance;
  template <class Hook, class Fn> void RegisterGameHook(Fn fn) {
    Hook::callbacks.push_back(fn);
  }
  template <class Hook, class Fn> void RegisterGameHookForID(int flag, Fn fn) {
    assert(flag == VB_USE_CUSTOM_CAMERA);
    Hook::callbacks.push_back(fn);
  }
};
static GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
struct RegisterShipInitFunc {
  RegisterShipInitFunc(std::function<void()> fn,
                       const std::set<std::string> & = {}) {
    fn();
  }
};
extern "C" {
/* NATIVE_MATH */
}
/* ENHANCEMENT */

static void Require(bool value, const char *message) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}
static bool NativeCameraShouldRun(GIVanillaBehavior flag, ...) {
  va_list args;
  va_start(args, flag);
  bool should = true;
  for (auto &hook : GameInteractor::ShouldVanillaBehavior::callbacks) {
    hook(flag, &should, args);
  }
  va_end(args);
  return should;
}
static void SceneInit() {
  for (auto &hook : GameInteractor::OnSceneInit::callbacks)
    hook(gPlayState->sceneId, 0);
}
static void Destroy() {
  for (auto &hook : GameInteractor::OnPlayDestroy::callbacks)
    hook();
}
static PlayState play;
static Player player;
static ActorEntry spawn;
static RoomShape shape;
static RoomShapeDListsEntry entry;
static const char *mesh[3] = {
    "__OTR__scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00DL_005788",
    COMBO_OPENING_FOREST_PACK_ROOT, COMBO_OPENING_FOREST_STUMP_PACK_ROOT};
static void Reset(int route, int form = PLAYER_FORM_HUMAN, int version = 2) {
  Destroy();
  play = {};
  player = {};
  spawn = {};
  shape = {};
  entry = {};
  gSaveContext = {};
  gPlayState = &play;
  gGameState = &play.state;
  play.state.running = true;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 1;
  gSaveContext.save.shipSaveInfo.rando.finalSeed = 0x12345678;
  std::memset(&gSaveContext.save.saveInfo, 0x5A,
              sizeof(gSaveContext.save.saveInfo));
  const bool opening = route == COMBO_PORTAL_OPENING_FOREST;
  gSaveContext.save.entrance =
      opening ? ENTRANCE(LOST_WOODS, 0) : ENTRANCE(WOODS_OF_MYSTERY, 0);
  play.sceneId = opening ? SCENE_LOST_WOODS : SCENE_26SARUNOMORI;
  play.roomCtx.curRoom.num = opening ? 0 : 1;
  spawn.pos = {-417, 0, -122};
  spawn.rot.y = 11520;
  play.linkActorEntry = &spawn;
  shape.normal.base.type = ROOM_SHAPE_TYPE_NORMAL;
  shape.normal.numEntries = 1;
  shape.normal.entries = &entry;
  entry.opa = (Gfx *)mesh[version];
  play.roomCtx.curRoom.roomShape = &shape;
  player.transformation = form;
  player.actor.world.pos = opening ? Vec3f{-417, 0, -122} : Vec3f{0, 0, 0};
  player.actor.prevPos = player.actor.world.pos;
  player.actor.world.rot.y = player.actor.shape.rot.y = opening ? 11520 : -8192;
  player.yaw = player.actor.shape.rot.y;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  auto &camera = play.mainCamera;
  camera.play = &play;
  camera.focalActor = &player.actor;
  camera.camId = CAM_ID_MAIN;
  camera.status = CAM_STATUS_ACTIVE;
  camera.mode = CAM_MODE_NORMAL;
  camera.setting = CAM_SET_NORMAL0;
  camera.uid = 7;
  camera.rUpdateRateInv = 10;
  camera.yawUpdateRateInv = 40;
  camera.pitchUpdateRateInv = 100;
  camera.yOffsetUpdateRate = camera.xzOffsetUpdateRate = camera.fovUpdateRate =
      .01f;
  camera.atLerpStepScale = 1;
  camera.floorNorm = {0, 1, 0};
  camera.focalActorPosRot = player.actor.world;
  camera.at = {20, 44, 20};
  camera.eye = camera.eyeNext = {200, 64, 200};
  camera.fov = 60;
  play.cameraPtrs[CAM_ID_MAIN] = &camera;
  play.activeCamId = CAM_ID_MAIN;
  normalModes[CAM_MODE_NORMAL] = {CAM_FUNC_NORMAL1,
                                  ARRAY_COUNT(sSetNormal0ModeNormalData),
                                  sSetNormal0ModeNormalData};
  sCameraSettings[CAM_SET_NORMAL0].cameraModes = normalModes;
  inCutscene = floorMissing = floorSteep = false;
  collisionFraction = 0;
  collisionQueries = 0;
}
static Vec3f Sub(Vec3f a, Vec3f b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static float Dot(Vec3f a, Vec3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static float Length(Vec3f a) { return std::sqrt(Dot(a, a)); }
static Vec3f Normalize(Vec3f a) {
  float n = Length(a);
  return {a.x / n, a.y / n, a.z / n};
}
static Vec3f Cross(Vec3f a, Vec3f b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static Vec3f Project(Vec3f point, const Camera &camera) {
  const Vec3f forward = Normalize(Sub(camera.at, camera.eye)),
              right = Normalize(Cross(forward, {0, 1, 0}));
  const Vec3f up = Cross(right, forward), delta = Sub(point, camera.eye);
  const float depth = Dot(delta, forward),
              half = std::tan(camera.fov * 3.14159265f / 360.0f) * depth;
  return {Dot(delta, right) / (half * 4.0f / 3.0f), Dot(delta, up) / half,
          depth};
}
static void CheckFraming(int route, int version) {
  const auto &camera = play.mainCamera;
  const float height = Player_GetHeight(&player);
  Vec3f anchor = route == COMBO_PORTAL_LOST_WOODS ? Vec3f{160, 0, -160}
                 : version == 1                   ? Vec3f{-417, 0, -122}
                                : Vec3f{COMBO_OPENING_FOREST_STUMP_X, 0,
                                        COMBO_OPENING_FOREST_STUMP_Z};
  Vec3f feet = Project(player.actor.world.pos, camera),
        headPoint = player.actor.world.pos;
  headPoint.y += height;
  const Vec3f head = Project(headPoint, camera);
  Vec3f topPoint = anchor;
  topPoint.y += 110;
  const Vec3f doorTop = Project(topPoint, camera),
              doorBottom = Project(anchor, camera);
  Require(feet.z > 0 && head.z > 0 && doorTop.z > head.z,
          "camera must face Link with doorway behind");
  Require(std::fabs(feet.x) < 0.05f && std::fabs(head.x) < 0.05f &&
              std::fabs(doorTop.x) < 0.05f,
          "full Link and doorway must share the central screen axis");
  Require(feet.y > -0.98f && feet.y < 0 && head.y < 0.98f && head.y > feet.y,
          "all forms must fit fully in the lower or central frame");
  Require(doorTop.y < 0.98f && doorBottom.y > -0.98f &&
              doorTop.y > doorBottom.y,
          "the complete doorway must fit vertically behind Link");
  Require(!ComboLostWoods_InMmPassage(
              play.sceneId, play.roomCtx.curRoom.num, player.actor.world.pos.x,
              player.actor.world.pos.y, player.actor.world.pos.z) &&
              !ComboOpeningForest_AtReturn(&play, &player.actor.world.pos),
          "framed arrivals must stay outside return triggers");
  Require(collisionQueries > 0,
          "presentation must use the native camera collision service");
  Require(player.yaw == player.actor.shape.rot.y &&
              player.actor.world.rot.y == player.actor.shape.rot.y,
          "native player facing caches must agree after portal placement");
}
int main() {
  GameInteractor::Instance->RegisterGameHookForID<
      GameInteractor::ShouldVanillaBehavior>(
      VB_USE_CUSTOM_CAMERA, [](GIVanillaBehavior, bool *, va_list args) {
        va_list copy;
        va_copy(copy, args);
        Require(
            va_arg(copy, Camera *) == &play.mainCamera,
            "camera presentation must preserve varargs for other enhancements");
        va_end(copy);
      });
  for (int route : {COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_OPENING_FOREST})
    for (int version = 0; version < 3; ++version)
      for (int day = 1; day <= 3; ++day)
        for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
          Reset(route, form, version);
          gSaveContext.save.day = day;
          const auto saved = gSaveContext;
          ComboForestArrivalCamera_Queue(route);
          SceneInit();
          Require(
              !NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
              "queued forest arrival must begin the reference front-facing "
              "presentation");
          CheckFraming(route, version);
          Require(!std::memcmp(&saved, &gSaveContext, sizeof(saved)),
                  "camera arrival must preserve every save/story/history byte");
          const auto at = play.mainCamera.at, eye = play.mainCamera.eye;
          play.state.input[0].cur.stick_x = 30;
          Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
                  "movement must release native camera immediately");
          Require(!std::memcmp(&at, &play.mainCamera.at, sizeof(at)) &&
                      !std::memcmp(&eye, &play.mainCamera.eye, sizeof(eye)),
                  "release must seed native control from the current view "
                  "without a snap or reinitialization");
          play.state.input[0] = {};
          Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
                  "releasing input must never reacquire the camera");
          // Exercise the actual native normal-camera distance/yaw steps from
          // the released view. This proves a settling envelope, not full camera
          // AI.
          auto &camera = play.mainCamera;
          const float h = Player_GetHeight(&player),
                      scale = h * 0.01f * (0.8f + 68.0f / h * 0.2f);
          const float before = camera.dist;
          const float after =
              Camera_ClampDist2(&camera, before, 280 * scale, 320 * scale, 1);
          Require(std::fabs(after - before) < 40,
                  "native radial settling must remain bounded on release for "
                  "every form");
          const float beforeFov = camera.fov;
          const Vec3f beforeEye = camera.eye, beforeAt = camera.at;
          Require(
              camera.animState == 0,
              "release fixture must exercise the native initial camera state");
          Camera_Normal1(&camera);
          Require(Dot(Sub(camera.eye, player.actor.world.pos),
                      Sub(beforeEye, player.actor.world.pos)) > 0,
                  "first native NORMAL1 update must retain the front view "
                  "rather than reset behind Link");
          Require(Distance(beforeEye, camera.eye) < 50 &&
                      Distance(beforeAt, camera.at) < 10 &&
                      std::fabs(camera.fov - beforeFov) < 2,
                  "native initial-handler release must settle eye, at and FOV "
                  "smoothly for every form");
        }
  Reset(COMBO_PORTAL_OPENING_FOREST);
  ComboForestArrivalCamera_Queue(COMBO_PORTAL_OPENING_FOREST);
  SceneInit();
  int held = 0;
  while (!NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera) &&
         held < 200)
    ++held;
  Require(held > 0 && held <= 75,
          "a stationary arrival must release within a short bounded interval");
  SceneInit();
  Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
          "ordinary re-entry must not replay an unqueued presentation");
  for (int kind = 0; kind < 14; ++kind) {
    Reset(COMBO_PORTAL_OPENING_FOREST);
    ComboForestArrivalCamera_Queue(COMBO_PORTAL_OPENING_FOREST);
    switch (kind) {
    case 0:
      gSaveContext.fileNum = 2;
      break;
    case 1:
      gSaveContext.save.shipSaveInfo.rando.finalSeed++;
      break;
    case 2:
      gSaveContext.sceneLayer = 1;
      break;
    case 3:
      gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
      break;
    case 4:
      play.roomCtx.curRoom.num = 1;
      break;
    case 5:
      gSaveContext.save.cutsceneIndex = 0xFFF0;
      break;
    case 6:
      play.csCtx.state = CS_STATE_RUN;
      break;
    case 7:
      play.soaringCsOrSoTCsPlaying = true;
      break;
    case 8:
      play.state.running = false;
      break;
    case 9:
      floorMissing = true;
      break;
    case 10:
      floorSteep = true;
      break;
    case 11:
      collisionFraction = 0.03f;
      break;
    case 12:
      Destroy();
      break;
    case 13:
      for (auto &hook : GameInteractor::OnSaveLoad::callbacks)
        hook(1);
      break;
    }
    const auto originalPlayer = player;
    const auto originalCamera = play.mainCamera;
    SceneInit();
    Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
            "unsafe or stale arrivals must retain native camera");
    Require(
        !std::memcmp(&originalPlayer, &player, sizeof(player)) &&
            !std::memcmp(&originalCamera, &play.mainCamera, sizeof(Camera)),
        "failed scene preparation must roll back player and camera together");
  }
  for (int kind = 0; kind < 10; ++kind) {
    Reset(COMBO_PORTAL_OPENING_FOREST);
    ComboForestArrivalCamera_Queue(COMBO_PORTAL_OPENING_FOREST);
    SceneInit();
    Require(!NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
            "control fixture must begin active");
    switch (kind) {
    case 0:
      play.state.input[0].cur.right_stick_x = 20;
      break;
    case 1:
      play.state.input[0].cur.button = BTN_Z;
      break;
    case 2:
      play.pauseCtx.state = PAUSE_STATE_MAIN;
      break;
    case 3:
      play.gameOverCtx.state = GAMEOVER_DEATH_START;
      break;
    case 4:
      play.roomCtx.curRoom.num = 1;
      break;
    case 5:
      gSaveContext.fileNum = 0;
      break;
    case 6:
      gSaveContext.save.shipSaveInfo.rando.finalSeed++;
      break;
    case 7:
      play.mainCamera.mode = CAM_MODE_TARGET;
      break;
    case 8:
      play.transitionTrigger = TRANS_TRIGGER_START;
      break;
    case 9:
      play.mainCamera.setting = CAM_SET_TELESCOPE;
      break;
    }
    Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
            "player/native/lifetime changes must end presentation");
    play.pauseCtx.state = PAUSE_STATE_OFF;
    play.state.input[0] = {};
    Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
            "ended presentation must stay ended");
  }
  Reset(COMBO_PORTAL_OPENING_FOREST);
  collisionFraction = .8f;
  ComboForestArrivalCamera_Queue(COMBO_PORTAL_OPENING_FOREST);
  SceneInit();
  Require(!NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
          "safe camera collision shortening must remain available");
  Require(play.mainCamera.dist < 190 * .81f,
          "native solid geometry must shorten the presentation eye");
  for (int version = 0; version < 3; ++version) {
    Reset(COMBO_PORTAL_OPENING_FOREST, PLAYER_FORM_HUMAN, version);
    Player_StartMode_E(&play, &player);
    const auto saved = gSaveContext;
    ComboForestArrivalCamera_Queue(COMBO_PORTAL_OPENING_FOREST);
    SceneInit();
    Vec3f walk = Sub(player.unk_3A0, player.actor.world.pos);
    const float yaw = player.actor.shape.rot.y * (6.28318530718f / 65536.0f);
    const Vec3f forward{std::sin(yaw), 0, std::cos(yaw)};
    Require(std::fabs(Length(walk) - 120) < .1f && Dot(walk, forward) > 119.9f,
            "the native automatic-entry target must follow the relocated spawn "
            "and outward yaw");
    play.transitionTrigger = TRANS_TRIGGER_END;
    inCutscene = true;
    for (int frame = 0; frame < 15; ++frame) {
      player.actor.world.pos.x += forward.x * 2;
      player.actor.world.pos.z += forward.z * 2;
      Require(!NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
              "automatic native entry walking must retain the reveal "
              "throughout the incoming fade");
      Require(
          std::fabs(play.mainCamera.at.x - player.actor.world.pos.x) < .01f &&
              std::fabs(play.mainCamera.at.z - player.actor.world.pos.z) < .01f,
          "the reveal must track the native entry walk rather than leave Link "
          "behind");
    }
    inCutscene = false;
    player.actionFunc = nullptr;
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    Require(!NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
            "the ready-control reveal must survive completion of the native "
            "entry walk");
    Require(!std::memcmp(&saved, &gSaveContext, sizeof(saved)),
            "adapting entry walking cannot alter save or story state");
  }
  bool laterCamera = false;
  GameInteractor::Instance
      ->RegisterGameHookForID<GameInteractor::ShouldVanillaBehavior>(
          VB_USE_CUSTOM_CAMERA, [&](GIVanillaBehavior, bool *should, va_list) {
            if (laterCamera) {
              play.mainCamera.eye.x += 25;
              play.mainCamera.at.x += 25;
              *should = false;
            }
          });
  Reset(COMBO_PORTAL_OPENING_FOREST);
  ComboForestArrivalCamera_Queue(COMBO_PORTAL_OPENING_FOREST);
  SceneInit();
  laterCamera = true;
  NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera);
  laterCamera = false;
  Require(NativeCameraShouldRun(VB_USE_CUSTOM_CAMERA, &play.mainCamera),
          "a later custom-camera takeover must end arrival ownership without "
          "reacquiring its old pose");
  std::puts(
      "PASS portal camera: both routes, native/v1/v2, every day/form, centered "
      "projection, collision, bounded release, rollback and lifetime");
}
