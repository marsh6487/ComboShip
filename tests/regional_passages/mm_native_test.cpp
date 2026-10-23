#include "ComboGameMode.h"
#include "entrances/RegionalPassages.h"
#include "global.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
SaveContext gSaveContext{};
PlayState *gPlayState;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static bool wet, exitFloor, dynamicFloor, missingWall, apronObstacle,
    lowCeiling;
static CollisionPoly floorPoly{}, wallPoly{};
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState *, CollisionContext *,
                                             CollisionPoly **poly, s32 *bg,
                                             Actor *, Vec3f *) {
  floorPoly.normal.y = 32767;
  *poly = &floorPoly;
  *bg = dynamicFloor ? 0 : BGCHECK_SCENE;
  return 0;
}
extern "C" u32 SurfaceType_GetSceneExitIndex(CollisionContext *,
                                             CollisionPoly *, s32) {
  return exitFloor;
}
extern "C" s32 WaterBox_GetSurface1_2(PlayState *, CollisionContext *, f32, f32,
                                      f32 *y, WaterBox **) {
  *y = wet ? 10 : -100;
  return true;
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *a, Vec3f *b,
                                       Vec3f *hit, CollisionPoly **poly,
                                       s32 checkWall, s32 checkFloor,
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
/* PRODUCTION */
int main() {
  PlayState play{};
  Player player{};
  CollisionHeader collision{};
  ActorEntry entries[2]{};
  EntranceEntry entrance{};
  entrance.spawn = 1;
  play.setupEntranceList = &entrance;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  play.colCtx.colHeader = &collision;
  ComboRegionalPassagesMm_ReadEntrances(&play, 1);
  gPlayState = &play;
  play.state.running = true;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  for (int route = COMBO_PORTAL_IKANA; route <= COMBO_PORTAL_SNOWHEAD;
       ++route) {
    const int scenes[] = {SCENE_IKANAMAE, SCENE_30GYOSON,
                          SCENE_10YUKIYAMANOMURA};
    play.sceneId = scenes[route - COMBO_PORTAL_IKANA];
    for (int slot = 0; slot < 3; ++slot) {
      gSaveContext.fileNum = slot;
      gSaveContext.save.shipSaveInfo.rando.finalSeed = 1234;
      const auto before = gSaveContext;
      ComboRegionalPassagesMm_ReadSpawns(&play, entries, 2);
      int found = 0;
      Vec3f mouth{};
      s16 yaw;
      assert(ComboRegionalPassagesMm_Get(&play, &found, &mouth, &yaw) &&
             found == route);
      assert(std::fabs(mouth.z - 198) < .1f);
      Vec3f trigger{mouth.x, 0, mouth.z - 28}, approach{};
      assert(!ComboRegionalPassagesMm_AtReturn(&play, &trigger));
      assert(!ComboRegionalPassagesMm_AtReturn(&play, &approach));
      assert(ComboRegionalPassagesMm_AtReturn(&play, &trigger) == route);
      assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
      ++gSaveContext.save.shipSaveInfo.rando.finalSeed;
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
    }
  }
  play.sceneId = SCENE_IKANAMAE;
  gSaveContext.fileNum = 0;
  for (int failure = 0; failure < 6; ++failure) {
    wet = failure == 0;
    exitFloor = failure == 1;
    dynamicFloor = failure == 2;
    missingWall = failure == 3;
    apronObstacle = failure == 4;
    lowCeiling = failure == 5;
    ComboRegionalPassagesMm_ReadSpawns(&play, entries, 2);
    assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
  }
  wet = exitFloor = dynamicFloor = missingWall = apronObstacle = lowCeiling =
      false;
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
    gComboGameMode = mode;
    ComboRegionalPassagesMm_ReadSpawns(&play, entries, 2);
    assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
    const auto before = gSaveContext;
    assert(!ComboRegionalPassagesMm_ApplyArrival(5));
    assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  gComboGameMode = COMBO_GAME_BOTH;
  for (int route = 5; route <= 7; ++route)
    for (int season = 0; season < 2; ++season)
      for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
        WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE >> 8) =
            season ? (WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE & 0xFF) : 0;
        gSaveContext.save.playerForm = form;
        gSaveContext.save.day = 2;
        gSaveContext.save.time = 0x8000;
        gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y = 222;
        const auto progress = gSaveContext.save.saveInfo;
        assert(ComboRegionalPassagesMm_ApplyArrival(route));
        const int wanted = route == 5   ? ENTRANCE(ROAD_TO_IKANA, 0)
                           : route == 6 ? ENTRANCE(GREAT_BAY_COAST, 0)
                           : season     ? ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 0)
                                        : ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 0);
        assert(gSaveContext.save.entrance == wanted &&
               gSaveContext.respawnFlag == 0);
        assert(gSaveContext.save.playerForm == form &&
               gSaveContext.save.day == 2 && gSaveContext.save.time == 0x8000);
        assert(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222);
        assert(!std::memcmp(&progress, &gSaveContext.save.saveInfo,
                            sizeof(progress)));
      }
  Actor globalItem{};
  globalItem.room = -1;
  globalItem.update = +[](Actor *, PlayState *) {};
  globalItem.home.pos = {0, 0, 118};
  globalItem.world.pos = {10000, 0, 10000};
  play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &globalItem;
  assert((NativeWorld{&play}.Occupied({0, 0, 118}, 105)));
  globalItem.world.pos = {-10000, 0, -10000};
  assert((NativeWorld{&play}.Occupied({0, 0, 118}, 105)));
  play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = nullptr;
  play.sceneId=SCENE_IKANAMAE;
  ComboRegionalPassagesMm_ReadSpawns(&play,entries,2);
  assert(ComboRegionalPassagesMm_Get(&play,nullptr,nullptr,nullptr));
  ComboRegionalPassagesMm_Reset();
  assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
  puts("PASS regional MM native endpoint: real spawn decoding, all "
       "routes/slots/forms/seasons, no bounce, dry/static floor, seed/solo "
       "guards and unchanged progression");
}
