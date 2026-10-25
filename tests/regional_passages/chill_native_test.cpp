#include "ComboGameMode.h"
#include "entrances/FountainSnowheadBridge.h"
#include "entrances/LostWoodsEntrance.h"
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/effects/ovl_Effect_Ss_Ice_Smoke/z_eff_ss_ice_smoke.h"
#include "render/RegionalSnow.h"
#include "render/RegionalBridgeMaterials.h"
#include "fast/lus_gbi.h"
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
const Mesh sMesh = BuildMesh();
static std::array<Vtx, 192 * 3> sDrawVertices{};
static bool available = true, room = true;
static unsigned matrices;
static std::array<Vec3f, 128> poses;
extern "C" u8 ResourceMgr_FileExists(const char *path) {
  if (std::strcmp(path, dgDust5Tex + 7) == 0)
    return available;
  assert(std::strncmp(path, "textures/comboship/regional/", 27) == 0);
  return false;
}
extern "C" bool NeiGi_CanDrawLayers(PlayState *, size_t, size_t, size_t) {
  return room;
}
extern "C" int ResourceMgr_OTRSigCheck(char *path) {
  return path && std::memcmp(path, "__OTR__", 7) == 0;
}
extern "C" Gfx *PakLoader_GetDLOverride(const char *) {
  assert(false);
  return nullptr;
}
extern "C" Gfx *ResourceMgr_LoadGfxByName(char *) {
  assert(false);
  return nullptr;
}
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
/* NATIVE_SETUP */
/* PRODUCTION */

using F3DGfx = Gfx;
/* NATIVE_COMBINER */
struct MaskShader {
  struct State {
    uint64_t combine_mode = 0;
    u32 other_mode_l = 0;
    RGBA prim_color{}, env_color{}, key_center{}, key_scale{};
    u8 prim_lod_fraction = 0, convert_k[6]{};
  } state;
  State *mRdp = &state;
  float mBufVbo[28]{};
  size_t mBufVboLen = 0;
  void GfxDpSetCombineMode(u32, u32, u32, u32);
  static void GenerateCC(ColorCombiner *, const ColorCombinerKey &);
  void PackInputs(ColorCombiner *comb, u8 fogFactor) {
    struct Vertex { RGBA color; float w; } vertex{{180, 180, 180, fogFactor}, 1};
    auto *v1 = &vertex;
    Vertex *v_arr[] = {&vertex};
    const int i = 0, numInputs = 7;
    const bool use_alpha = true, use_fog = true, use_blend_color = false;
    mBufVboLen = 0;
    /* NATIVE_PACKED_INPUTS */
  }
  float Alpha(const ColorCombiner &comb, float textureAlpha) const {
    float combined = 0;
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
      float terms[4]{};
      for (unsigned j = 0; j < 4; ++j) {
        const auto input = (comb.shader_id0 >> (cycle * 32 + 16 + j * 4)) & 15;
        if (input >= SHADER_INPUT_1 && input <= SHADER_INPUT_7)
          terms[j] = mBufVbo[(input - SHADER_INPUT_1) * 4 + 3];
        else if (input == SHADER_TEXEL0)
          terms[j] = textureAlpha;
        else if (input == SHADER_COMBINED)
          terms[j] = combined;
        else {
          assert(input == SHADER_0 || input == SHADER_1);
          terms[j] = input == SHADER_1 ? 1.f : 0.f;
        }
      }
      combined = (terms[0] - terms[1]) * terms[2] + terms[3];
    }
    return combined;
  }
  bool PreservesMask() {
    ColorCombinerKey key{};
    key.options = SHADER_OPT(_2CYC) | SHADER_OPT(ALPHA) | SHADER_OPT(FOG);
    key.combine_mode = state.combine_mode;
    ColorCombiner comb{};
    GenerateCC(&comb, key);
    assert(comb.usedTextures[0] && !comb.usedTextures[1]);
    for (u8 fog : {u8(0), u8(64), u8(255)}) {
      PackInputs(&comb, fog);
      for (float texel : {0.f, 128.f / 255.f, 1.f}) {
        const float alpha = Alpha(comb, texel);
        const float expected = texel * state.prim_color.a / 255.f;
        if (std::fabs(alpha - expected) > .000001f)
          return false;
        if (texel == 0.f) {
          const float background = .27f, foreground = .83f;
          assert(foreground * alpha + background * (1.f - alpha) == background);
        }
      }
    }
    return true;
  }
};
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << (width)) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << (width)) - 1))
/* NATIVE_SHADER */

// Follow the real SETUPDL_25 sublist; submitting it to polyXlu does not make
// its opaque render mode translucent. Inspect the state at each actual draw.
struct ParticleBlendState {
  u32 high = 0, low = 0, color = 0;
  unsigned snow = 0, chill = 0;
  bool snowValid = true, chillValid = true;
  MaskShader shader;
  void Walk(Gfx *begin, Gfx *end, unsigned depth = 0) {
    assert(depth < 2);
    for (auto *c = begin; c < end; ++c) {
      switch (c->words.w0 >> 24) {
      case G_DL: {
        auto *target = reinterpret_cast<Gfx *>(c->words.w1);
        assert(target == sSetupDL[SETUPDL_25]);
        Walk(target, target + 6, depth + 1);
        break;
      }
      case G_RDPSETOTHERMODE:
        high = c->words.w0 & 0xffffffu;
        low = c->words.w1;
        break;
      case G_SETOTHERMODE_L: {
        const u32 bits = (c->words.w0 & 255) + 1;
        const u32 shift = 32 - ((c->words.w0 >> 8) & 255) - bits;
        const u32 mask = u32(((u64(1) << bits) - 1) << shift);
        low = (low & ~mask) | u32(c->words.w1);
        break;
      }
      case G_SETPRIMCOLOR:
        color = c->words.w1;
        shader.state.prim_color = {u8(color >> 24), u8(color >> 16),
                                   u8(color >> 8), u8(color)};
        break;
      case G_SETCOMBINE: {
        auto *packet = c;
        gfx_set_combine_handler_rdp(&shader, &packet);
        break;
      }
      case G_TRI2: {
        // The interpreter enables alpha blending only for a memory-color /
        // one-minus-alpha blender. A translucent queue alone is insufficient.
        const bool blends =
            (((low >> 20) & 3) == G_BL_CLR_MEM &&
             ((low >> 16) & 3) == G_BL_1MA) ||
            (((low >> 22) & 3) == G_BL_CLR_MEM &&
             ((low >> 18) & 3) == G_BL_1MA);
        const bool valid = blends && (low & FORCE_BL) && (low & Z_CMP) &&
                           !(low & Z_UPD) &&
                           (low >> 30) == G_BL_CLR_FOG &&
                           (high & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE &&
                           shader.PreservesMask();
        if ((color & 0xffffff00u) == 0xd2edff00u) {
          ++snow;
          snowValid &= valid;
        } else {
          assert((color & 0xffffff00u) == 0x82cdff00u);
          ++chill;
          chillValid &= valid;
        }
        break;
      }
      }
    }
  }
};

static void Alive(Actor *, PlayState *) {}
int main() {
  PlayState play{};
  GraphicsContext gfx{};
  Player player{};
  SnowheadActor actor{};
  static Gfx commands[1024];
  static Gfx opaqueCommands[8192];
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
  gfx.polyOpa.p = opaqueCommands;
  gfx.polyXlu.p = commands;
  Draw(&actor.dyna.actor, &play);
  ParticleBlendState blend;
  blend.Walk(commands, gfx.polyXlu.p);
  assert(blend.snow && blend.chill);
  std::printf("%s snow particle blending/depth/fog; %s chill particle "
              "blending/depth/fog\n",
              blend.snowValid ? "PASS" : "FAIL",
              blend.chillValid ? "PASS" : "FAIL");
  std::fflush(stdout);
  assert(blend.snowValid && blend.chillValid &&
         "particle cards must alpha-blend and must not write depth");
  assert((blend.low & Z_UPD) && "the final native setup must be restored");
  // Reject an otherwise valid single-texture material that forces primitive
  // opacity across the whole card instead of multiplying the texture mask.
  Gfx opaqueMask{};
  gDPSetCombineMode(&opaqueMask, G_CC_MODULATEI_PRIM, G_CC_PASS2);
  auto *badPacket = &opaqueMask;
  gfx_set_combine_handler_rdp(&blend.shader, &badPacket);
  assert(!blend.shader.PreservesMask());
  puts("PASS zero/mid/full texture alpha, primitive fade, fog alpha retention "
       "and transparent background; opaque-mask negative control rejected");
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
