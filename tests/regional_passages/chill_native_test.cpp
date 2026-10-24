#include "ComboGameMode.h"
#include "entrances/FountainSnowheadBridge.h"
#include "entrances/LostWoodsEntrance.h"
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/effects/ovl_Effect_Ss_Ice_Smoke/z_eff_ss_ice_smoke.h"
#include "render/RegionalSnow.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
SaveContext gSaveContext{};
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
uintptr_t gSegments[16]{};
u32 EffectSsIceSmoke_Init(PlayState *, u32, EffectSs *, void *);
void EffectSsIceSmoke_Update(PlayState *, u32, EffectSs *);
}
using namespace ComboFountainSnowhead;
struct SnowheadActor {
  DynaPolyActor dyna;
  uint32_t snowTick;
};
SnowheadActor *sActor = nullptr;
PlayState *sOwner = nullptr;
static bool available = true, room = true;
static unsigned matrices;
static std::array<Vec3f, 128> poses;
extern "C" u8 ResourceMgr_FileExists(const char *path) {
  assert(std::strcmp(path, dgDust5Tex + 7) == 0);
  return available;
}
extern "C" bool NeiGi_CanDrawLayers(PlayState *, size_t, size_t, size_t) {
  return room;
}
extern "C" void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, u8) {
  poses[matrices] = {x, y, z};
}
extern "C" void Matrix_Mult(MtxF *, u8) {}
extern "C" Mtx *Matrix_NewMtx(GraphicsContext *, char *, s32) {
  static Mtx m[128];
  return &m[matrices++];
}
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(ctx)                                                        \
  {                                                                            \
    GraphicsContext *__gfxCtx = (ctx)
#define CLOSE_DISPS(ctx) }
#define gSPVertex __gSPVertex
/* PRODUCTION */
static void Alive(Actor *, PlayState *) {}
int main() {
  PlayState play{};
  GraphicsContext gfx{};
  Player player{};
  SnowheadActor actor{};
  static Gfx commands[1024];
  play.state.gfxCtx = &gfx;
  play.state.running = true;
  play.sceneNum = SCENE_ZORAS_FOUNTAIN;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  sOwner = &play;
  sActor = &actor;
  actor.dyna.actor.update = Alive;
  actor.snowTick = 50;
  player.actor.world.pos = {1840, kMouth.y, 0};
  // Actual native Fountain object lists, decoded independently in review.
  const std::initializer_list<s16> lists[] = {
      {0x74, 0x40, 0x12f, 0x12c, 0xa4, 0x170, 0x7c, 0x188, 0x163, 0x24},
      {0x74, 0x12f, 7, 0x16, 0x11f, 0x188, 0x24, 0x154, 0x12c, 0x163},
      {0x74, 0x40, 0x13, 0x12f, 0x12c, 0xa4, 0x170, 0x7c, 0x188, 0x24, 0x163}};
  for (auto list : lists) {
    play.objectCtx = {};
    for (s16 id : list)
      play.objectCtx.status[play.objectCtx.num++].id = id;
    auto before = play.objectCtx;
    EffectSs old{};
    EffectSsIceSmokeInitParams init{};
    assert(!EffectSsIceSmoke_Init(&play, 0, &old, &init));
    gfx.polyXlu.p = commands;
    matrices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    unsigned triangles = 0, colors = 0, textures = 0, pushes = 0, pops = 0;
    for (auto *c = commands; c < gfx.polyXlu.p; ++c) {
      auto op = c->words.w0 >> 24;
      if (op == G_TRI2)
        ++triangles;
      if (op == G_SETPRIMCOLOR) {
        assert((c->words.w1 & 0xffffff00u) == 0x82cdff00u);
        colors += (c->words.w1 & 255) > 0;
      }
      if (op == G_SETTIMG) {
        assert(std::strcmp((const char *)c->words.w1, dgDust5Tex) == 0);
        ++textures;
      }
      pushes += op == G_COMBO_RM_PUSH;
      pops += op == G_COMBO_RM_POP;
    }
    assert(triangles > 0 && colors == triangles && matrices == triangles &&
           textures == 1 && pushes == 1 && pops == 1);
    assert(!std::memcmp(&before, &play.objectCtx, sizeof(before)));
    auto first = poses;
    unsigned count = matrices;
    gfx.polyXlu.p = commands;
    matrices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    assert(count == matrices &&
           !std::memcmp(first.data(), poses.data(), count * sizeof(Vec3f)));
    ++actor.snowTick;
    gfx.polyXlu.p = commands;
    matrices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    assert(std::memcmp(first.data(), poses.data(), count * sizeof(Vec3f)) != 0);
  }
  auto none = [&] {
    gfx.polyXlu.p = commands;
    matrices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    assert(gfx.polyXlu.p == commands && matrices == 0);
  };
  available = false;
  none();
  available = true;
  room = false;
  none();
  room = true;
  gfx.polyXlu.p = commands;
  matrices = 0;
  DrawLocalChill(&actor.dyna.actor, &play);
  assert(gfx.polyXlu.p > commands && matrices > 0);
  player.actor.world.pos = {0, 0, 4000};
  none();
  player.actor.world.pos = {1840, kMouth.y, 0};
  sOwner = nullptr;
  none();
  sOwner = &play;
  sActor = nullptr;
  none();
  sActor = &actor;
  actor.dyna.actor.update = nullptr;
  none();
  actor.dyna.actor.update = Alive;
  gComboGameMode = COMBO_GAME_OOT;
  none();
  gComboGameMode = COMBO_GAME_BOTH;
  play.sceneNum = SCENE_LAKE_HYLIA;
  none();
  // Native positive/control cleanup proves this is real effect initialization,
  // rather than an attempted-spawn counter or an unconditional failure double.
  play.objectCtx = {};
  play.objectCtx.num = 1;
  play.objectCtx.status[0].id = -OBJECT_FZ;
  play.objectCtx.status[0].segment = (void *)uintptr_t(0x80001000);
  EffectSs old{};
  EffectSsIceSmokeInitParams init{};
  assert(!EffectSsIceSmoke_Init(&play, 0, &old, &init));
  play.objectCtx.status[0].id = OBJECT_FZ;
  assert(EffectSsIceSmoke_Init(&play, 0, &old, &init));
  assert(old.life == 50 && old.draw && old.update);
  play.objectCtx.num = 0;
  EffectSsIceSmoke_Update(&play, 0, &old);
  assert(old.life == -1);
  puts("PASS native Fountain FZ initialization negative/positive controls; "
       "actual actor chill packets without FZ, resource loss/recovery, epoch "
       "motion/freeze, owner/mode/scene cleanup and untouched object context");
}
