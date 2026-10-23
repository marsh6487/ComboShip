#include "ComboGameMode.h"
#include "entrances/RegionalPassages.h"
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_haka/object_haka.h"
#include "overlays/actors/ovl_Bg_Haka/z_bg_haka.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include "soh/Enhancements/Gameplay/ComboRegionalPassages.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <vector>
SaveContext gSaveContext{};
PlayState *gPlayState;
EntranceInfo gEntranceTable[ENTR_MAX]{};
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static u32 seed = 77;
static bool mq;
extern "C" u32 ResourceMgr_IsSceneMasterQuest(s16) { return mq; }
static bool heldSaveMutex, wet, exitFloor, dynamicFloor, missingWall,
    apronObstacle, lowCeiling, inCs;
#undef IS_RANDO
#define IS_RANDO 1
namespace Rando {
struct Context {
  static Context *GetInstance() {
    static Context c;
    return &c;
  }
  u32 GetSeed() { return seed; }
};
} // namespace Rando
struct SaveManager {
  static SaveManager *Instance;
  std::function<void(bool)> init;
  std::function<void()> load;
  std::function<void(const SaveContext &, int, bool)> save;
  nlohmann::json disk, written;
  void ThreadPoolWait() { assert(!heldSaveMutex); }
  void AddInitFunction(decltype(init) c) { init = c; }
  void AddLoadFunction(const std::string &, int, decltype(load) c) { load = c; }
  void AddSaveFunction(const std::string &, int, decltype(save) c, bool, int) {
    save = c;
  }
  void SaveData(const std::string &, const nlohmann::json &value) {
    written = value;
  }
  void LoadData(const std::string &, nlohmann::json &value) { value = disk; }
};
static SaveManager manager;
SaveManager *SaveManager::Instance = &manager;
constexpr int SECTION_PARENT_NONE = -1;
struct GameInteractor {
  struct OnLoadGame {};
  struct OnPlayerUpdate {};
  struct OnPlayDestroy {};
  struct OnExitGame {};
  static GameInteractor *Instance;
  std::function<void(int)> load, exit;
  std::function<void()> update, destroy;
  template <class Hook, class F> void RegisterGameHook(F f) {
    if constexpr (std::is_same_v<Hook, OnLoadGame>)
      load = f;
    if constexpr (std::is_same_v<Hook, OnPlayerUpdate>)
      update = f;
    if constexpr (std::is_same_v<Hook, OnPlayDestroy>)
      destroy = f;
    if constexpr (std::is_same_v<Hook, OnExitGame>)
      exit = f;
  }
};
static GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
struct RegisterShipInitFunc {
  explicit RegisterShipInitFunc(void (*f)()) { f(); }
};
struct ActorDBInit {
  std::string name, desc;
  int category = 0, objectId = 0;
  size_t instanceSize = 0;
  u32 flags = 0;
  ActorFunc init = nullptr, destroy = nullptr, update = nullptr, draw = nullptr;
};
struct ActorDB {
  struct Entry {
    struct {
      int id;
    } entry;
  };
  static ActorDB *Instance;
  std::vector<ActorDBInit> profiles;
  Entry entry{};
  Entry &AddEntry(const ActorDBInit &p) {
    entry.entry.id = 9000 + profiles.size();
    profiles.push_back(p);
    return entry;
  }
};
static ActorDB database;
ActorDB *ActorDB::Instance = &database;
static std::vector<Actor *> allocated;
static int collisionRegistrations, collisionDeletes, chimes, slides,
    titleRequests;
static CollisionPoly floorPoly{}, wallPoly{};
static CollisionHeader nativeGraveCollision{};
extern "C" {
void (*gComboSceneSwitchCallback)(int) = [](int) {};
s32 Player_InCsMode(PlayState *) { return inCs; }
f32 BgCheck_EntityRaycastFloor5(PlayState *, CollisionContext *,
                                CollisionPoly **poly, s32 *bg, Actor *,
                                Vec3f *) {
  floorPoly.normal.y = 32767;
  *poly = &floorPoly;
  *bg = dynamicFloor ? 0 : BGCHECK_SCENE;
  return 0;
}
u32 SurfaceType_GetSceneExitIndex(CollisionContext *, CollisionPoly *, s32) {
  return exitFloor;
}
s32 WaterBox_GetSurface1(PlayState *, CollisionContext *, f32, f32, f32 *y,
                         WaterBox **) {
  *y = wet ? 10 : -100;
  return true;
}
s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *a, Vec3f *b, Vec3f *hit,
                            CollisionPoly **poly, s32 checkWall, s32 checkFloor,
                            s32 checkCeiling, s32 oneFace, s32 *bg) {
  if (checkFloor || checkCeiling) {
    assert(checkWall && checkFloor && checkCeiling && !oneFace);
    if (apronObstacle) {
      *poly = &wallPoly;
      *bg = 0;
      *hit = *a;
      return true;
    }
    if (lowCeiling && std::fmin(a->y, b->y) < 40 &&
        std::fmax(a->y, b->y) > 40) {
      *poly = &wallPoly;
      *bg = BGCHECK_SCENE;
      wallPoly.normal = {0, -32767, 0};
      *hit = *a;
      return true;
    }
  }
  if (missingWall || (a->z - 200) * (b->z - 200) >= 0)
    return false;
  const float t = (200 - a->z) / (b->z - a->z);
  *hit = {a->x + (b->x - a->x) * t, a->y, 200};
  wallPoly.normal = {0, 0, -32767};
  *poly = &wallPoly;
  *bg = BGCHECK_SCENE;
  return true;
}
void Actor_SetScale(Actor *a, f32 scale) { a->scale = {scale, scale, scale}; }
f32 Math_SinS(s16 yaw) { return std::sin(yaw * ComboRegional::kAngle); }
f32 Math_CosS(s16 yaw) { return std::cos(yaw * ComboRegional::kAngle); }
s32 Math_StepToF(f32 *value, f32 target, f32 step) {
  if (std::fabs(target - *value) <= step + .000001f) {
    *value = target;
    return true;
  }
  *value += target > *value ? step : -step;
  return false;
}
void Actor_PlaySfx_Flagged(Actor *, u16 sfx) {
  assert(sfx == NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
  ++slides;
}
void Sfx_PlaySfxCentered(u16 sfx) {
  assert(sfx == NA_SE_SY_CORRECT_CHIME);
  ++chimes;
}
void Actor_ProcessInitChain(Actor *a, InitChainEntry *) {
  a->minVelocityY = 0;
  Actor_SetScale(a, .1f);
}
void DynaPolyActor_Init(DynaPolyActor *, s32 flags) {
  assert(flags == DPM_UNK);
}
void CollisionHeader_GetVirtual(void *resource, CollisionHeader **out) {
  assert(
      !std::strcmp(reinterpret_cast<const char *>(resource), gGravestoneCol));
  *out = &nativeGraveCollision;
}
s32 DynaPoly_SetBgActor(PlayState *, DynaCollisionContext *, Actor *,
                        CollisionHeader *col) {
  assert(col == &nativeGraveCollision);
  ++collisionRegistrations;
  return 23;
}
void DynaPoly_DeleteBgActor(PlayState *, DynaCollisionContext *, s32 bg) {
  assert(bg == 23);
  ++collisionDeletes;
}
void Actor_WorldToActorCoords(Actor *a, Vec3f *out, Vec3f *p) {
  const float x = p->x - a->world.pos.x, z = p->z - a->world.pos.z;
  const float si = Math_SinS(a->shape.rot.y), co = Math_CosS(a->shape.rot.y);
  *out = {x * co - z * si, p->y - a->world.pos.y, x * si + z * co};
}
s32 Collider_InitCylinder(PlayState *, ColliderCylinder *) { return 0; }
s32 Collider_SetCylinder(PlayState *, ColliderCylinder *, Actor *,
                         ColliderCylinderInit *) {
  return 0;
}
s32 Collider_DestroyCylinder(PlayState *, ColliderCylinder *) { return 0; }
void BgHaka_IdleClosed(BgHaka *, PlayState *) {}
void BgHaka_Draw(Actor *, PlayState *) {}
void DoorAna_WaitClosed(DoorAna *, PlayState *) {}
void DoorAna_WaitOpen(DoorAna *, PlayState *) {}
void DoorAna_Draw(Actor *, PlayState *) {}
void Combo_SOH_RequestCrossGameTitleCard(void) { ++titleRequests; }
void Actor_Kill(Actor *actor) { actor->update = nullptr; }
}
static void *Combo_ResolveSym(const char *, const char *) {
  return reinterpret_cast<void *>(1);
}
#define CollisionHeader_GetVirtual(resource, out)                              \
  CollisionHeader_GetVirtual(                                                  \
      const_cast<void *>(static_cast<const void *>(resource)), out)
/* NATIVE_ACTORS */
#undef CollisionHeader_GetVirtual
extern "C" Actor *Actor_Spawn(ActorContext *ctx, PlayState *play, s16 id, f32 x,
                              f32 y, f32 z, s16 rx, s16 ry, s16 rz,
                              s16 params) {
  assert(id >= 9000 && id < 9000 + database.profiles.size());
  const auto &p = database.profiles[id - 9000];
  auto *actor = static_cast<Actor *>(std::calloc(1, p.instanceSize));
  allocated.push_back(actor);
  actor->id = id;
  actor->room = play->roomCtx.curRoom.num;
  actor->params = params;
  actor->world.pos = actor->home.pos = {x, y, z};
  actor->world.rot = actor->home.rot = actor->shape.rot = {rx, ry, rz};
  actor->init = p.init;
  actor->update = p.update;
  actor->destroy = p.destroy;
  actor->draw = p.draw;
  actor->next = ctx->actorLists[p.category].head;
  ctx->actorLists[p.category].head = actor;
  p.init(actor, play);
  return actor;
}
static int jumpCalls, cutsceneId, cutsceneDuration;
static float jumpVelocity, sWaterSpeedFactor;
static const char *jumpAnimation;
void Player_Action_8084F9C0(Player *, PlayState *) {}
s32 Player_SetupAction(PlayState *, Player *p, PlayerActionFunc action, s32) {
  p->actionFunc = action;
  return 1;
}
void func_808389E8(Player *, const void *animation, float velocity,
                   PlayState *) {
  ++jumpCalls;
  jumpAnimation = static_cast<const char *>(animation);
  jumpVelocity = velocity;
}
extern "C" s16 OnePointCutscene_Init(PlayState *, s16 id, s16 duration, Actor *,
                                     s16 camera) {
  cutsceneId = id;
  cutsceneDuration = duration;
  assert(camera == CAM_ID_MAIN);
  return 0;
}
/* NATIVE_GROTTO */
/* PRODUCTION */
static PlayState play;
static Player player;
static BgHaka original;
static DoorAna originalHole;
static ActorEntry entries[5]{};
static EntranceEntry entrances[5]{};
static CollisionHeader collision;
static void Noop(Actor *, PlayState *) {}
static void LoadFile(const nlohmann::json &disk = nullptr, int slot = 0) {
  manager.disk = disk;
  heldSaveMutex = true;
  manager.init(false);
  if (disk.is_object())
    manager.load();
  heldSaveMutex = false;
  gSaveContext.fileNum = slot;
  interactor.load(slot);
}
static void Scene(int scene) {
  for (auto *actor : allocated) {
    if (actor->destroy)
      actor->destroy(actor, &play);
    std::free(actor);
  }
  allocated.clear();
  play = {};
  player = {};
  original = {};
  originalHole = {};
  gPlayState = &play;
  play.state.running = true;
  play.sceneNum = scene;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  play.colCtx.colHeader = &collision;
  play.setupEntranceList = entrances;
  for (int i = 0; i < 5; ++i) {
    entrances[i] = {};
    entrances[i].spawn = i;
    entries[i] = {};
  }
  if (scene == SCENE_GRAVEYARD) {
    original.dyna.actor.id = ACTOR_BG_HAKA;
    original.dyna.actor.update = Noop;
    originalHole.actor.id = ACTOR_DOOR_ANA;
    originalHole.actor.update = Noop;
    play.actorCtx.actorLists[ACTORCAT_BG].head = &original.dyna.actor;
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = &originalHole.actor;
  }
  ComboRegionalPassagesOot_ReadEntrances(&play, 5);
  ComboRegionalPassagesOot_ReadSpawns(&play, entries, 5);
}
int main() {
  gEntranceTable[ENTR_GRAVEYARD_ENTRANCE].scene = SCENE_GRAVEYARD;
  gEntranceTable[ENTR_LAKE_HYLIA_OUTSIDE_LAB].scene = SCENE_LAKE_HYLIA;
  gEntranceTable[ENTR_ICE_CAVERN_ENTRANCE].scene = SCENE_ICE_CAVERN;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.health = 48;
  gSaveContext.rupees = 321;
  LoadFile();
  Scene(SCENE_GRAVEYARD);
  gSaveContext.entranceIndex = ENTR_GRAVEYARD_ENTRANCE;
  const auto nativeGrave = original;
  const auto nativeHole = originalHole;
  Vec3f mouth{};
  int route;
  s16 yaw;
  assert(ComboRegionalPassagesOot_Get(&play, &route, &mouth, &yaw) &&
         route == 5 && mouth.x == 150);
  assert(collisionRegistrations == 1 &&
         sRegional.grave->native.dyna.actor.scale.x == .1f);
  assert(database.profiles[0].objectId == OBJECT_HAKA &&
         database.profiles[0].destroy == BgHaka_Destroy &&
         database.profiles[0].draw == BgHaka_Draw &&
         database.profiles[1].draw == DoorAna_Draw);
  auto *added = sRegional.grave;
  added->native.dyna.unk_150 = -1;
  player.stateFlags2 |= PLAYER_STATE2_MOVING_DYNAPOLY;
  for (int frame = 0; frame < 120 && !added->native.state; ++frame)
    GraveUpdate(&added->native.dyna.actor, &play);
  assert(added->native.state == 1 &&
         added->native.dyna.actor.minVelocityY == 60 && chimes == 1 &&
         slides > 40);
  assert(!(player.stateFlags2 & PLAYER_STATE2_MOVING_DYNAPOLY));
  assert(added->native.state == 1 && added->native.dyna.unk_150 == 0);
  player.stateFlags2 |= PLAYER_STATE2_MOVING_DYNAPOLY;
  GraveUpdate(&added->native.dyna.actor, &play);
  assert(player.stateFlags2 & PLAYER_STATE2_MOVING_DYNAPOLY);
  added->native.dyna.unk_150 = 1;
  GraveUpdate(&added->native.dyna.actor, &play);
  assert(!(player.stateFlags2 & PLAYER_STATE2_MOVING_DYNAPOLY));
  assert(!std::memcmp(&nativeGrave, &original, sizeof(original)) &&
         !std::memcmp(&nativeHole, &originalHole, sizeof(originalHole)));
  player.actor.world.pos = mouth;
  for (int i = 0; i < 11; ++i)
    HoleUpdate(sRegional.hole, &play);
  assert(player.stateFlags1 & PLAYER_STATE1_FLOOR_DISABLED);
  assert(ComboRegionalPassagesOot_OwnsGraveFall(&play));
  player.actor.world.pos.y = -10;
  assert(ComboRegionalPassagesOot_AtDeparture(&play, &player.actor.world.pos) ==
         5);
  manager.save(gSaveContext, 0, false);
  const auto disk = manager.written;
  assert(disk["version"] == 1 && disk["seed"] == 77 &&
         disk["origins"][0]["x"] == 150);
  ComboRegionalPassagesOot_CancelDeparture();
  assert(!(player.stateFlags1 & PLAYER_STATE1_FLOOR_DISABLED));
  LoadFile(disk);
  const auto progress = gSaveContext;
  gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y = 222;
  const auto preserved = gSaveContext.respawn[RESPAWN_MODE_RETURN];
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  assert(gSaveContext.respawnFlag == 0 &&
         gSaveContext.entranceIndex == ENTR_GRAVEYARD_ENTRANCE);
  Scene(SCENE_GRAVEYARD);
  interactor.update();
  const auto arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
  assert(arrival.playerParams == 0x04FF && arrival.pos.x == 150 &&
         gSaveContext.respawnFlag == 2);
  assert(gSaveContext.health == progress.health &&
         gSaveContext.rupees == progress.rupees &&
         gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222);
  player.actor.world.pos = arrival.pos;
  Player_StartMode_Grotto(&play, &player);
  assert(jumpCalls == 1 && jumpVelocity == 12 &&
         !std::strcmp(jumpAnimation, gPlayerAnim_link_normal_jump));
  assert(player.actionFunc == Player_Action_8084F9C0 &&
         player.stateFlags1 & PLAYER_STATE1_IN_CUTSCENE);
  assert(player.fallStartHeight == arrival.pos.y && cutsceneId == 5110 &&
         cutsceneDuration == 40);
  interactor.update();
  assert(!std::memcmp(&preserved, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                      sizeof(preserved)));
  ++seed;
  assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  assert(!ComboRegionalPassagesOot_ApplyArrival(5));
  --seed;
  for (int route = 6; route <= 7; ++route) {
    Scene(route == 6 ? SCENE_LAKE_HYLIA : SCENE_ICE_CAVERN);
    gSaveContext.entranceIndex =
        route == 6 ? ENTR_LAKE_HYLIA_OUTSIDE_LAB : ENTR_ICE_CAVERN_ENTRANCE;
    assert(ComboRegionalPassagesOot_Get(&play, &route, &mouth, &yaw));
    Vec3f trigger{mouth.x, 0, mouth.z - 28};
    assert(ComboRegionalPassagesOot_AtDeparture(&play, &trigger) == route);
    assert(ComboRegionalPassagesOot_ApplyArrival(route));
    assert(gSaveContext.respawnFlag == 0);
    Scene(route == 6 ? SCENE_LAKE_HYLIA : SCENE_ICE_CAVERN);
    interactor.update();
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams == 0x0EFF);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.z < mouth.z - 90);
    interactor.update();
  }
  Scene(SCENE_ICE_CAVERN);
  entrances[0].room = 1;
  entries[0].pos = {500, 0, 0};
  play.roomCtx.curRoom.num = 1;
  ComboRegionalPassagesOot_ReadSpawns(&play, entries, 5);
  assert(ComboRegionalPassagesOot_Get(&play, nullptr, &mouth, nullptr) &&
         std::fabs(mouth.x - 500) < .1f);
  assert(sOrigins[2].room == 0);
  // First visits, changed setup and invalid documents resolve loaded native
  // floor.
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  assert(gSaveContext.entranceIndex == ENTR_GRAVEYARD_ENTRANCE &&
         gSaveContext.respawnFlag == 0);
  Scene(SCENE_GRAVEYARD);
  interactor.update();
  assert(play.transitionTrigger == TRANS_TRIGGER_START &&
         gSaveContext.respawnFlag == 2);
  assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams == 0x04FF &&
         sRegional.grave->native.state == 1);
  interactor.update();
  LoadFile();
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  gSaveContext.respawnFlag = 3;
  gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {444, 55, 666};
  Scene(SCENE_GRAVEYARD);
  gSaveContext.respawnFlag = 0;
  interactor.update();
  assert(sArrivalRoute == 0 && play.transitionTrigger == TRANS_TRIGGER_OFF &&
         gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x == 444);
  LoadFile(disk, 1);
  assert(ComboRegionalPassagesOot_ApplyArrival(5));
  assert(gSaveContext.respawnFlag == 0 &&
         gSaveContext.entranceIndex == ENTR_GRAVEYARD_ENTRANCE);
  LoadFile(disk);
  gSaveContext.linkAge = 1;
  assert(ComboRegionalPassagesOot_ApplyArrival(5) &&
         gSaveContext.respawnFlag == 0);
  gSaveContext.linkAge = 0;
  LoadFile(disk);
  mq = true;
  assert(ComboRegionalPassagesOot_ApplyArrival(5) &&
         gSaveContext.respawnFlag == 0);
  mq = false;
  auto invalid = disk;
  invalid["origins"][0]["x"] = nullptr;
  LoadFile(invalid);
  assert(ComboRegionalPassagesOot_ApplyArrival(5) &&
         gSaveContext.respawnFlag == 0);
  LoadFile(disk);
  gSaveContext.fileNum = 0xFF;
  assert(!ComboRegionalPassagesOot_ApplyArrival(5));
  gSaveContext.fileNum = 0;
  for (int failure = 0; failure < 6; ++failure) {
    Scene(SCENE_ICE_CAVERN);
    wet = failure == 0;
    exitFloor = failure == 1;
    dynamicFloor = failure == 2;
    missingWall = failure == 3;
    apronObstacle = failure == 4;
    lowCeiling = failure == 5;
    assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  }
  wet = exitFloor = dynamicFloor = missingWall = apronObstacle = lowCeiling =
      false;
  Scene(SCENE_GRAVEYARD);
  apronObstacle = true;
  assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
  apronObstacle = false;
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
    gComboGameMode = mode;
    const auto before = gSaveContext;
    assert(!ComboRegionalPassagesOot_Get(&play, nullptr, nullptr, nullptr));
    assert(!ComboRegionalPassagesOot_ApplyArrival(7));
    assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  gComboGameMode = COMBO_GAME_BOTH;
  Scene(SCENE_ICE_CAVERN);
  assert(ComboRegionalPassagesOot_Get(&play,nullptr,nullptr,nullptr));
  interactor.destroy();
  assert(!ComboRegionalPassagesOot_Get(&play,nullptr,nullptr,nullptr));
  for (auto *actor : allocated) {
    if (actor->destroy)
      actor->destroy(actor, &play);
    std::free(actor);
  }
  assert(collisionDeletes == collisionRegistrations);
  puts("PASS regional OoT native endpoint: additional native "
       "grave/collision/pull/hole, saved slot/seed origin, native jump/pop-out "
       "action, cave returns and preserved checks/progress/Farore");
}
