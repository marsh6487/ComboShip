#include "ComboGameMode.h"
#include "entrances/MmRegionalBridgePlacements.h"
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
// Deliberately unavailable search geometry: fixed production endpoints must
// remain available without opportunistic collision discovery.
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState *, CollisionContext *,
                                             CollisionPoly **, s32 *, Actor *,
                                             Vec3f *) {
  return BGCHECK_Y_MIN;
}
extern "C" u32 SurfaceType_GetSceneExitIndex(CollisionContext *,
                                             CollisionPoly *, s32) {
  return 0;
}
extern "C" s32 WaterBox_GetSurface1_2(PlayState *, CollisionContext *, f32, f32,
                                      f32 *, WaterBox **) {
  return 0;
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *, Vec3f *,
                                       Vec3f *, CollisionPoly **, s32, s32, s32,
                                       s32, s32 *) {
  return 0;
}
/* PRODUCTION */
/* NATIVE_START */
int main() {
  using namespace ComboMmRegional;
  PlayState play{};
  Player player{};
  CollisionHeader collision{};
  ActorEntry entries[9]{};
  EntranceEntry entrances[10]{};
  for (int i = 0; i < 9; ++i) {
    entrances[i].spawn = i;
    entrances[i].room = 0;
  }
  play.setupEntranceList = entrances;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  play.colCtx.colHeader = &collision;
  play.state.running = true;
  gPlayState = &play;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  const int scenes[] = {SCENE_IKANAMAE, SCENE_30GYOSON, SCENE_10YUKIYAMANOMURA,
                        SCENE_10YUKIYAMANOMURA2};
  for (int scene : scenes)
    for (int slot = 0; slot < 3; ++slot) {
      play.sceneId = scene;
      play.roomCtx.curRoom.num = 0;
      gSaveContext.fileNum = slot;
      gSaveContext.save.shipSaveInfo.rando.finalSeed = 1234;
      const auto *profile = ForScene(scene);
      assert(profile);
      // A modified spawn must not move the authored mouth or start another
      // search.
      entries[profile->entrance].pos = {30000, 500, 30000};
      ComboRegionalPassagesMm_ReadEntrances(&play, 10);
      ComboRegionalPassagesMm_ReadSpawns(&play, entries, 9);
      int route = 0;
      Vec3f mouth{};
      s16 yaw = 0;
      const auto before = gSaveContext;
      assert(ComboRegionalPassagesMm_Get(&play, &route, &mouth, &yaw));
      assert(route == profile->route && mouth.x == profile->mouth.pos.x &&
             mouth.y == profile->mouth.pos.y &&
             mouth.z == profile->mouth.pos.z && yaw == profile->mouth.yaw);
      auto front = ComboRegional::Forward(yaw);
      Vec3f trigger{mouth.x + front.x * 28, mouth.y, mouth.z + front.z * 28};
      assert(!ComboRegionalPassagesMm_AtReturn(&play, &trigger));
      Vec3f arrival{profile->nativeArrival.pos.x, profile->nativeArrival.pos.y,
                    profile->nativeArrival.pos.z};
      assert(!ComboRegionalPassagesMm_AtReturn(&play, &arrival));
      assert(ComboRegionalPassagesMm_AtReturn(&play, &trigger) == route);
      Vec3f beside{trigger.x + front.z * 80, trigger.y,
                   trigger.z - front.x * 80};
      assert(!ComboRegionalPassagesMm_AtReturn(&play, &beside));
      assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
      ++gSaveContext.save.shipSaveInfo.rando.finalSeed;
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
      --gSaveContext.save.shipSaveInfo.rando.finalSeed;
      gSaveContext.fileNum = (slot + 1) % 3;
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
      gSaveContext.fileNum = slot;
      play.roomCtx.curRoom.num = 1;
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
      play.roomCtx.curRoom.num = 0;
      // Header re-entry destroys stale arming even when the arena pointer
      // repeats.
      ComboRegionalPassagesMm_ReadEntrances(&play, 10);
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
      ComboRegionalPassagesMm_ReadSpawns(&play, entries, 9);
      assert(!ComboRegionalPassagesMm_AtReturn(&play, &trigger));
      ComboRegionalPassagesMm_Reset();
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
      ComboRegionalPassagesMm_ReadSpawns(&play, entries, 9);
      assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
    }
  // Missing entrance/spawn records fail boundedly; they cannot bind stale data.
  play.sceneId = SCENE_10YUKIYAMANOMURA;
  gSaveContext.fileNum = 0;
  ComboRegionalPassagesMm_ReadEntrances(&play, 8);
  ComboRegionalPassagesMm_ReadSpawns(&play, entries, 9);
  assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
  ComboRegionalPassagesMm_ReadEntrances(&play, 10);
  ComboRegionalPassagesMm_ReadSpawns(&play, entries, 8);
  assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
  ComboRegionalPassagesMm_ReadEntrances(&play, 10);
  ComboRegionalPassagesMm_ReadSpawns(&play, nullptr, 9);
  assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
  assert(!ComboRegionalPassagesMm_AtReturn(&play, nullptr));
  gSaveContext.fileNum = 0;
  for (int route = 5; route <= 7; ++route)
    for (int season = 0; season < 2; ++season)
      for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
        WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE >> 8) =
            season ? (WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE & 0xFF) : 0;
        gSaveContext.save.playerForm = form;
        gSaveContext.save.day = 2;
        gSaveContext.save.time = 0x8000;
        gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y = 222;
        auto progress = gSaveContext.save.saveInfo;
        const auto returnRespawn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
        assert(ComboRegionalPassagesMm_ApplyArrival(route));
        int wanted = route == 5   ? ENTRANCE(ROAD_TO_IKANA, 0)
                     : route == 6 ? ENTRANCE(GREAT_BAY_COAST, 0)
                     : season     ? ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8)
                                  : ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8);
        assert(gSaveContext.save.entrance == wanted &&
               gSaveContext.respawnFlag == 0);
        assert(gSaveContext.save.playerForm == form &&
               gSaveContext.save.day == 2 && gSaveContext.save.time == 0x8000 &&
               gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222);
        assert(!std::memcmp(&progress, &gSaveContext.save.saveInfo,
                            sizeof(progress)));
        assert(!std::memcmp(&returnRespawn,
                            &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                            sizeof(returnRespawn)));
      }
#ifdef COMBO_MM_NATIVE_START_TEST
  // Controls execute the actual native Player_Init selection block: a pause
  // save is stationary; an explicit regional arrival must keep its entrance
  // start mode instead of inheriting that saved-session marker.
  player.actor.params = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_OWL);
  gSaveContext.save.cutsceneIndex = 0;
  gSaveContext.save.isOwlSave = true;
  gSaveContext.save.shipSaveInfo.pauseSaveEntrance =
      ENTRANCE(CLOCK_TOWER_INTERIOR, 0);
  assert(NativeStartMode(&player) == PLAYER_START_MODE_D);
  assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance == -1 &&
         !gSaveContext.save.isOwlSave);
  gSaveContext.save.cutsceneIndex = 0xFFF0;
  assert(NativeStartMode(&player) == PLAYER_START_MODE_D);
  for (int slot = 0; slot < 3; ++slot)
    for (int route = 5; route <= 7; ++route)
      for (int season = 0; season < 2; ++season)
        for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
          gSaveContext.fileNum = slot;
          gSaveContext.save.playerForm = form;
          WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE >> 8) =
              season ? (WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE & 0xFF) : 0;
          gSaveContext.save.shipSaveInfo.pauseSaveEntrance =
              ENTRANCE(CLOCK_TOWER_INTERIOR, 0);
          gSaveContext.save.isOwlSave = true;
          gSaveContext.save.cutsceneIndex = 0xFFF0;
          const auto top = gSaveContext.respawn[RESPAWN_MODE_TOP],
                     ret = gSaveContext.respawn[RESPAWN_MODE_RETURN];
          assert(ComboRegionalPassagesMm_ApplyArrival(route));
          player.actor.params = PLAYER_PARAMS(
              0xFF, route == 7 ? PLAYER_START_MODE_OWL : PLAYER_START_MODE_F);
          assert(NativeStartMode(&player) ==
                 (route == 7 ? PLAYER_START_MODE_OWL : PLAYER_START_MODE_F));
          assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance == -1 &&
                 gSaveContext.save.cutsceneIndex == 0);
          assert(!std::memcmp(&top, &gSaveContext.respawn[RESPAWN_MODE_TOP],
                              sizeof(top)));
          assert(!std::memcmp(&ret, &gSaveContext.respawn[RESPAWN_MODE_RETURN],
                              sizeof(ret)));
          assert(gSaveContext.save.playerForm == form);
        }
  puts("PASS extracted native Player_Init start decision: ordinary pause "
       "resume/cutscene controls; explicit regional checkpoint arrivals "
       "dispatch native F/OWL for all routes, slots, forms and seasons");
#endif
  for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
    gComboGameMode = mode;
    auto before = gSaveContext;
    assert(!ComboRegionalPassagesMm_ApplyArrival(5));
    assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
    ComboRegionalPassagesMm_ReadEntrances(&play, 10);
    ComboRegionalPassagesMm_ReadSpawns(&play, entries, 9);
    assert(!ComboRegionalPassagesMm_Get(&play, nullptr, nullptr, nullptr));
  }
  gComboGameMode = COMBO_GAME_BOTH;
  for (int invalid : {-1, 3, 255}) {
    gSaveContext.fileNum = invalid;
    auto before = gSaveContext;
    assert(!ComboRegionalPassagesMm_ApplyArrival(7));
    assert(!std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  gSaveContext.fileNum = 0;
  assert(!ComboRegionalPassagesMm_ApplyArrival(4));
  assert(!ComboRegionalPassagesMm_ApplyArrival(8));
  puts("PASS fixed MM regional endpoints: authored poses independent of "
       "spawn/geometry search; native season entrances, all slots/forms, "
       "lifecycle/seed/solo guards and no bounce");
}
