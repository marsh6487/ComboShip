#include "ComboGameMode.h"
#include "actors/RegionalActorPlacement.h"
#include "entrances/LakeHyliaBridge.h"
#include "global.h"
extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"
}
#include <cassert>
#include <cstring>
static Actor spawned[16];
static unsigned spawnCount;
static bool ready, floorAvailable = true, blocked;
static int endpointRoute = 7;
extern "C" {
PlayState *gPlayState;
SaveContext gSaveContext;
int gComboGameMode = COMBO_GAME_BOTH;
s32 Player_InCsMode(PlayState *) { return 0; }
int ComboRegionalPassagesOot_Get(PlayState *, int *route, Vec3f *p, s16 *y) {
  if (!ready)
    return 0;
  *route = endpointRoute;
  *p = {1840, 280, 0};
  *y = -16384;
  if (endpointRoute == 6) {
    *p = {ComboLakeHylia::kMouth.x, ComboLakeHylia::kMouth.y,
          ComboLakeHylia::kMouth.z};
    *y = ComboLakeHylia::kYaw;
  }
  return 1;
}
f32 BgCheck_EntityRaycastFloor5(PlayState *, CollisionContext *,
                                CollisionPoly **p, s32 *bg, Actor *,
                                Vec3f *pos) {
  static CollisionPoly floor;
  *p = floorAvailable ? &floor : nullptr;
  *bg = BGCHECK_SCENE;
  return pos->y - 200;
}
u32 SurfaceType_GetSceneExitIndex(CollisionContext *, CollisionPoly *, s32) {
  return 0;
}
s32 BgCheck_EntityLineTest1(CollisionContext *, Vec3f *, Vec3f *, Vec3f *,
                            CollisionPoly **, s32, s32, s32, s32, s32 *) {
  return blocked;
}
void Actor_Kill(Actor *a) { a->update = nullptr; }
void Alive(Actor *, PlayState *) {}
Actor *Actor_Spawn(ActorContext *ctx, PlayState *, s16 id, f32 x, f32 y, f32 z,
                   s16 rx, s16 ry, s16 rz, s16 params) {
  assert(spawnCount < 16);
  auto *a = &spawned[spawnCount++];
  a->id = id;
  a->params = params;
  a->world.pos = {x, y, z};
  a->update = Alive;
  a->room = 0;
  a->next = ctx->actorLists[ACTORCAT_NPC].head;
  ctx->actorLists[ACTORCAT_NPC].head = a;
  return a;
}
}
/* PRODUCTION_RUNTIME */
int main() {
  PlayState p{};
  gPlayState = &p;
  p.state.running = true;
  p.roomCtx.curRoom.segment = (void *)1;
  Player player{};
  p.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 0);
  ready = true;
  floorAvailable = false;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 0);
  floorAvailable = true;
  blocked = true;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 0);
  blocked = false;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 1 && spawned[0].params == 0x7D02);
  assert(spawned[0].world.pos.x == 1740 && spawned[0].world.pos.y == 292 &&
         spawned[0].world.pos.z == 150);
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 1);
  p.pauseCtx.state = 1;
  ComboRegionalActorsOot_Update();
  assert(spawned[0].update);
  p.pauseCtx.state = 0;
  Actor authored{};
  authored.id = ACTOR_EN_VIEWER;
  authored.params = 0x7D12;
  authored.update = Alive;
  authored.next = p.actorCtx.actorLists[ACTORCAT_NPC].head;
  p.actorCtx.actorLists[ACTORCAT_NPC].head = &authored;
  ComboRegionalActorsOot_Update();
  assert(!spawned[0].update && spawnCount == 1 && authored.update);
  endpointRoute = 6;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 3);
  assert(spawned[1].params == 0x7D11 && spawned[2].params == 0x7E0C);
  assert(spawned[1].world.pos.x == -2005 && spawned[1].world.pos.y == -1305 &&
         spawned[1].world.pos.z == 3673);
  assert(spawned[2].world.pos.x == -2105 && spawned[2].world.pos.z == 3653);
  gComboGameMode = COMBO_GAME_OOT;
  ComboRegionalActorsOot_Update();
  assert(!spawned[1].update && !spawned[2].update && authored.update);
  gComboGameMode = COMBO_GAME_MM;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 3);
  gComboGameMode = COMBO_GAME_BOTH;
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 5);
  Actor_Kill(&spawned[3]);
  ComboRegionalActorsOot_Update();
  assert(spawnCount == 5); // no repeated missing-resource spawns
  puts("PASS native OoT placement update: delayed endpoint/floor, obstruction, "
       "authored override, paired Lulu, pause, modes and no respawn churn");
}
