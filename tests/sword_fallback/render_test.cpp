#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
extern "C" {
#include "functions.h"
#include "macros.h"
#include "z64.h"
}
#include "ComboItemDrawABI.h"
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
/* PRODUCTION_INFO */
static Gfx opa[1024], xlu[1024];
static GraphicsContext gfx;
static PlayState play;
struct Pose {
  float scale = 2, x = 11, y = 17, z = -9, rx = 0, ry = 0, rz = 0;
  bool billboard = false;
  bool operator==(const Pose &) const = default;
};
static Pose pose;
static std::vector<Pose> stack;
static std::deque<Mtx> matrices;
static std::map<Mtx *, Pose> captured;
static std::vector<std::vector<Gfx>> arena;
static Gfx scrolls[4][12];
static std::vector<std::vector<int32_t>> scrollParams;
static bool flameAvailable = true;
static int interpolation, shimmers, sentinels, identityDraws;
static NeiGi::Mesh identityMesh;
static ComboForeignDrawInfo recipe;
void FrameInterpolation_RecordOpenChild(const void *, int) { ++interpolation; }
void FrameInterpolation_RecordCloseChild() {
  assert(interpolation > 0);
  --interpolation;
}
extern "C" {
void gSPSegment(void *cmd, int segment, uintptr_t address) {
  __gSPSegment((Gfx *)cmd, segment, address);
}
void gSPDisplayList(Gfx *cmd, Gfx *path) { __gSPDisplayList(cmd, path); }
void Matrix_Push() { stack.push_back(pose); }
void Matrix_Pop() {
  assert(!stack.empty());
  pose = stack.back();
  stack.pop_back();
}
void Matrix_Scale(float x, float y, float z, u8) {
  assert(x == y && y == z);
  pose.scale *= x;
}
void Matrix_Translate(float x, float y, float z, u8) {
  pose.x += x;
  pose.y += y;
  pose.z += z;
}
void Matrix_RotateX(float r, u8) { pose.rx += r; }
void Matrix_RotateY(float r, u8) { pose.ry += r; }
void Matrix_RotateZ(float r, u8) { pose.rz += r; }
void Matrix_ReplaceRotation(MtxF *) { pose.billboard = true; }
Mtx *Matrix_NewMtx(GraphicsContext *, char *, s32) {
  matrices.emplace_back();
  captured[&matrices.back()] = pose;
  return &matrices.back();
}
void Gfx_SetupDL_25Opa(GraphicsContext *) {}
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
void Graph_OpenDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void *Graph_Alloc(GraphicsContext *, size_t bytes) {
  arena.emplace_back((bytes + sizeof(Gfx) - 1) / sizeof(Gfx));
  return arena.back().data();
}
Gfx *Gfx_TwoTexScrollEx(GraphicsContext *, s32, u32 x1, u32 y1, s32 w1, s32 h1,
                        s32, u32 x2, u32 y2, s32 w2, s32 h2, s32 sx1, s32 sy1,
                        s32 sx2, s32 sy2) {
  assert(scrollParams.size() < 4);
  scrollParams.push_back({int32_t(x1), int32_t(y1), w1, h1, int32_t(x2),
                          int32_t(y2), w2, h2, sx1, sy1, sx2, sy2});
  return scrolls[scrollParams.size() - 1];
}
void GetItem_Draw(PlayState *, s16) { ++sentinels; }
}
static uint8_t ResourceMgr_FileExists(const char *path) {
  assert(strstr(path, "gGiBlueFireFlameDL"));
  return flameAvailable;
}
static uint8_t ResourceMgr_FileAltExists(const char *) { return false; }
static bool ResourceMgr_IsAltAssetsEnabled() { return false; }
static RandomizerCheck OOT_GetQueuedDrawCheck() { return RandomizerCheck(1); }
static const ComboForeignDrawInfo *
ComboResolveForeignDrawInfo(RandomizerCheck) {
  return &recipe;
}
static bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo *, const char *,
                                  PlayState *) {
  assert(false);
  return false;
}
static int CVarGetInteger(const char *, int) { return 0; }
static void ComboDrawMaskShimmer(PlayState *, const char *,
                                 const uint8_t *color, const char *owner) {
  assert(pose == Pose{} && !strcmp(owner, "mm"));
  assert(color[0] == 220 && color[1] == 225 && color[2] == 240);
  ++shimmers;
}
template <class... Args> static void NeiGi_DrawPresentation(Args...) {
  assert(false);
}
template <class... Args> static void ComboDrawSpinAttackGi(Args...) {
  assert(false);
}
#define UNUSED_HANDLER(name)                                                   \
  static void name(PlayState *, const ComboForeignDrawInfo *) { assert(false); }
UNUSED_HANDLER(OOT_DrawForeignDekuNuts)
UNUSED_HANDLER(OOT_DrawForeignRecoveryHeart)
UNUSED_HANDLER(OOT_DrawForeignFish)
UNUSED_HANDLER(OOT_DrawForeignPotion)
UNUSED_HANDLER(OOT_DrawForeignBlueFire)
UNUSED_HANDLER(OOT_DrawForeignPoes)
UNUSED_HANDLER(OOT_DrawForeignFairyBottle)
UNUSED_HANDLER(OOT_DrawForeignSoulFlame)
UNUSED_HANDLER(OOT_DrawForeignOps)
UNUSED_HANDLER(OOT_DrawForeignSimple)
static void NeiGi_DrawSeasonOverlay(PlayState*,int,const char*) {
  assert(false && "sword fixture must not select weather");
}
static bool OOT_DrawForeignFairyContainer(PlayState*,const ComboForeignDrawInfo*) {assert(false);return false;}
static void NeiGi_DrawSongOverlay(PlayState*,int,const char*) {assert(false);}
static NeiGi::Basis NeiGi_CameraBasis(PlayState*) {return {};}
static void NeiGi_DrawMesh(PlayState*,const NeiGi::Mesh& mesh) {assert(pose==Pose{});identityMesh=mesh;++identityDraws;}
/* PRODUCTION_HANDLERS */

struct Draw {
  std::string path;
  Pose pose;
  bool gray;
  uint32_t color;
  int owner;
};
static std::vector<Draw> CheckStream(Gfx *begin, Gfx *end, bool restore) {
  Pose gpu;
  int owner = 0;
  bool gray = false;
  uint32_t color = 0;
  uintptr_t segment8 = 0;
  std::vector<Draw> draws;
  for (auto *cmd = begin; cmd < end; ++cmd) {
    auto op = cmd->words.w0 >> 24;
    if (op == G_COMBO_RM_PUSH) {
      assert(!strcmp((const char *)cmd->words.w1, "oot"));
      ++owner;
    }
    if (op == G_COMBO_RM_POP) {
      assert(owner > 0);
      --owner;
    }
    if (op == G_SETGRAYSCALE)
      gray = cmd->words.w1;
    if (op == G_SETINTENSITY)
      color = cmd->words.w1;
    if (op == G_MTX)
      gpu = captured.at((Mtx *)cmd->words.w1);
    if (op == G_MOVEWORD && ((cmd->words.w0 >> 16) & 255) == G_MW_SEGMENT &&
        (cmd->words.w0 & 65535) == 8 * 4)
      segment8 = cmd->words.w1;
    if (op == G_DL || op == G_DL_OTR_FILEPATH)
      draws.push_back({(const char *)cmd->words.w1, gpu, gray, color, owner});
  }
  assert(owner == 0 && !gray);
  if (!draws.empty())
    assert(gpu == Pose{});
  if (restore && segment8)
    assert(((Gfx *)segment8)->words.w0 >> 24 == G_ENDDL);
  return draws;
}
static void Reset(int kind, bool trueTier, bool shimmer) {
  memset(opa, 0, sizeof(opa));
  memset(xlu, 0, sizeof(xlu));
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = opa + 1024;
  gfx.polyXlu.p = xlu;
  gfx.polyXlu.d = xlu + 1024;
  play.state.gfxCtx = &gfx;
  play.state.frames = 47;
  play.gameplayFrames = 47;
  pose = {};
  stack.clear();
  captured.clear();
  matrices.clear();
  arena.clear();
  scrollParams.clear();
  interpolation = shimmers = sentinels = identityDraws = 0;
  flameAvailable = true;
  recipe = {};
  recipe.count = 1;
  recipe.xluStart = -1;
  recipe.drawKind = kind;
  recipe.itemShimmer = shimmer;
  const uint8_t color[4] = {220, 225, 240, 255};
  memcpy(recipe.itemShimmerColor, color, 4);
  recipe.dls[0] =
      kind == CW_DRAW_KIND_CUSTOM_GI
          ? "__OTR__@oot:alt/objects/object_custom_equip/gCustomMasterSwordDL"
          : "__OTR__@oot:objects/object_toki_objects/"
            "object_toki_objects_DL_001BD0";
  if (kind == CW_DRAW_KIND_CUSTOM_GI) {
    recipe.scale = .04f;
    recipe.opCount = 1;
    recipe.ops[0] = {CW_OP_ROTATE_Z, 18774.682f, 0, 0, {}};
  }
  if (trueTier) {
    const uint8_t flame[4] = {120, 180, 255, 255};
    memcpy(recipe.primColorXlu, flame, 4);
    if (kind == CW_DRAW_KIND_MASTER_SWORD) {
      const uint8_t gold[4] = {255, 215, 110, 255};
      memcpy(recipe.primColorOpa, gold, 4);
    }
  }
}
static void Dispatch() {
  GetItemEntry entry{};
  entry.comboForeignCheck = 1;
  OOT_DrawComboForeign(&play, &entry);
  assert(pose == Pose{} && stack.empty() && interpolation == 0 && !sentinels);
}
int main() {
  for (int kind : {CW_DRAW_KIND_MASTER_SWORD, CW_DRAW_KIND_CUSTOM_GI})
    for (bool trueTier : {false, true})
      for (bool shimmer : {false, true}) {
        Reset(kind, trueTier, shimmer);
        Dispatch();
        auto body = CheckStream(opa, gfx.polyOpa.p, true),
             flame = CheckStream(xlu, gfx.polyXlu.p, true);
        assert(body.size() == 1 && body[0].path == recipe.dls[0] &&
               body[0].owner == 1);
        assert(body[0].gray == (trueTier && kind == CW_DRAW_KIND_MASTER_SWORD));
        if (body[0].gray)
          assert(body[0].color == 0xFFD76EFFu);
        assert(std::abs(body[0].pose.scale -
                        (kind == CW_DRAW_KIND_MASTER_SWORD ? .1f : .08f)) <
               .00001f);
        assert(std::abs(body[0].pose.rz -
                        (kind == CW_DRAW_KIND_MASTER_SWORD ? 2.1f : 1.8f)) <
               .0001f);
        if (kind == CW_DRAW_KIND_CUSTOM_GI)
          assert(body[0].pose.rx == 0 &&
                 body[0].pose.ry == .94f);
        assert(flame.size() == size_t(trueTier) && shimmers == int(shimmer));
        if (trueTier) {
          assert(flame[0].gray && flame[0].color == 0x78B4FFFFu &&
                 flame[0].owner == 1);
          assert(flame[0].pose.billboard && flame[0].pose.y == -53.f &&
                 flame[0].pose.scale == 10.f);
          assert(scrollParams[0] ==
                 std::vector<int32_t>(
                     {0, 0, 16, 32, 47, -376, 16, 32, 0, 0, 1, -8}));
        }
        if (kind == CW_DRAW_KIND_MASTER_SWORD)
          assert(
              scrollParams.back() ==
              std::vector<int32_t>({47, 0, 32, 32, 0, 0, 32, 32, 1, 0, 0, 0}));
      }
  for(auto kind:{NeiGi::Kind::KokiriSword,NeiGi::Kind::RazorSword,NeiGi::Kind::GildedSword,
                 NeiGi::Kind::MasterSword,NeiGi::Kind::SwordAura,NeiGi::Kind::BiggoronSword,NeiGi::Kind::GreatFairySword}) {
    Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);recipe.neiShimmer=int(kind)+1;
    Dispatch();assert(identityDraws==1 && shimmers==0);
    const auto wanted=NeiGi::SampleShimmer(play.gameplayFrames,true,{},kind);
    assert(identityMesh.count==wanted.count);
    for(size_t i=0;i<wanted.count;++i)assert(identityMesh.vertices[i].rgb==wanted.vertices[i].rgb &&
        identityMesh.vertices[i].alpha==wanted.vertices[i].alpha && identityMesh.vertices[i].p.x==wanted.vertices[i].p.x);
    assert(CheckStream(opa,gfx.polyOpa.p,false).size()==1 && CheckStream(xlu,gfx.polyXlu.p,false).empty());
  }
  Reset(CW_DRAW_KIND_CUSTOM_GI, true, true);
  flameAvailable = false;
  Dispatch();
  assert(CheckStream(xlu, gfx.polyXlu.p, true).empty());
  assert(CheckStream(opa, gfx.polyOpa.p, true).size() == 1 && shimmers == 1);
  Reset(CW_DRAW_KIND_CUSTOM_GI, false, true);
  recipe.count = 2;
  recipe.xluStart = 1;
  recipe.dls[1] = "__OTR__@oot:objects/test/skinDL";
  Dispatch();
  assert(CheckStream(opa, gfx.polyOpa.p, false).size() == 1 &&
         CheckStream(xlu, gfx.polyXlu.p, false).size() == 1);
  std::cout << "PASS actual OoT sword fallback dispatch: owner scopes, signed "
               "transforms, independent blade/flame, shimmer pose, stream "
               "cursors and segment cleanup\n";
}
