#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using s32 = int32_t;
using u8 = uint8_t;
using s8 = int8_t;
using u32 = uint32_t;
struct Color_RGB8 { uint8_t r, g, b; };
struct Vec3s {
  int16_t x = 0, y = 0, z = 0;
};
struct Vec3f {
  float x = 0, y = 0, z = 0;
};
struct Mtx {
  float x = 0, y = 0, z = 0;
  float sx = 1, sy = 1, sz = 1;
};
struct Gfx {
  int kind = 0, seg = 0;
  uintptr_t target = 0;
};
struct Actor {};
struct GraphicsContext {
  Gfx opa[4096]{}, xlu[4096]{};
  Gfx *op = opa;
  Gfx *xp = xlu;
  std::vector<void *> allocations;
  ~GraphicsContext() {
    for (auto p : allocations)
      std::free(p);
  }
};
struct PlayState {
  struct {
    GraphicsContext *gfxCtx;
    uint32_t frames = 1;
  } state;
  uint32_t gameplayFrames = 1;
  Mtx billboardMtxF;
  Mtx **flexLimbOverrideMTX = nullptr;
};
struct StandardLimb {
  Vec3s jointPos;
  u8 child = 255, sibling = 255;
  Gfx *dList = nullptr;
};
#ifdef HOST_MM
struct SkeletonHeader {
  void **segment;
  u8 limbCount;
};
#else
struct SkeletonHeader {
  void **segment;
  u8 limbCount, skeletonType;
};
#endif
struct FlexSkeletonHeader {
  SkeletonHeader sh;
  u8 dListCount;
};
struct AnimationHeader {};
struct SkelAnime {
  void **skeleton = nullptr;
  Vec3s *jointTable = nullptr;
  u8 dListCount = 0;
  float playSpeed = 1;
};
#define OPEN_DISPS(ctx) {
#define CLOSE_DISPS(ctx) }
#define POLY_OPA_DISP play->state.gfxCtx->op
#define POLY_XLU_DISP play->state.gfxCtx->xp
#define SEGMENTED_TO_VIRTUAL(p) ((StandardLimb *)(p))
#define MATRIX_NEWMTX(c) Matrix_NewMtx(c)
#define MATRIX_TOMTX(p) Matrix_ToMtx(p)
#define osSyncPrintf(...) ((void)0)
#define LIMB_DONE 255
#define LIMB_ROOT_POS 0
#define LIMB_ROOT_ROT 1
#define MTXMODE_APPLY 0
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 0
#define G_MTX_MODELVIEW 0
#define AA_EN 0
#define Z_CMP 0
#define IM_RD 0
#define CLR_ON_CVG 0
#define CVG_DST_WRAP 0
#define ZMODE_XLU 0
#define FORCE_BL 0
#define GBL_c1(...) 0
#define G_RM_AA_ZB_XLU_SURF2 0
#define gDPPipeSync(p) ((p)->kind = 0)
void gDPSetPrimColor(Gfx* p, int, int, int r, int g, int b, int a) {
  *p = {4, 0, (uintptr_t)(((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a)};
}
void gDPSetEnvColor(Gfx* p, int r, int g, int b, int a) {
  *p = {5, 0, (uintptr_t)(((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a)};
}
#define gDPSetRenderMode(p, ...) ((p)->kind = 0)
void gDPSetGrayscaleColor(Gfx* p, int r, int g, int b, int a) {
  *p = {6, 0, (uintptr_t)(((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a)};
}
void gSPGrayscale(Gfx* p, bool enable) { *p = {7, 0, (uintptr_t)enable}; }
void gSPComboRMPush(Gfx* p, const char* game) { *p = {8, 0, (uintptr_t)game}; }
void gSPComboRMPop(Gfx* p) { *p = {9, 0, 0}; }
#define gSPEndDisplayList(p) ((p)->kind = 0)
void gSPSegment(Gfx *p, int seg, uintptr_t target) { *p = {1, seg, target}; }
void gSPSegment(Gfx *p, int seg, void *target) {
  gSPSegment(p, seg, (uintptr_t)target);
}
void gSPMatrix(Gfx *p, Mtx *target, int) { *p = {2, 0, (uintptr_t)target}; }
void gSPDisplayList(Gfx *p, Gfx *target) { *p = {3, 0, (uintptr_t)target}; }
void *Graph_Alloc(GraphicsContext *c, size_t n) {
  auto p = std::calloc(1, n);
  assert(p);
  c->allocations.push_back(p);
  return p;
}
#define GRAPH_ALLOC(c, n) Graph_Alloc(c, n)
static Mtx current;
static std::vector<Mtx> matrices;
void Matrix_Push() { matrices.push_back(current); }
void Matrix_Pop() {
  current = matrices.back();
  matrices.pop_back();
}
void Matrix_Translate(float x, float y, float z, int) {
  current.x += x;
  current.y += y;
  current.z += z;
}
void Matrix_Scale(float x, float y, float z, int) {
  current.sx *= x; current.sy *= y; current.sz *= z;
}
void Matrix_ReplaceRotation(Mtx *) {}
void Matrix_TranslateRotateZYX(Vec3f *p, Vec3s *) {
  Matrix_Translate(p->x, p->y, p->z, 0);
}
Mtx *Matrix_ToMtx(Mtx *p) {
  *p = current;
  return p;
}
Mtx *Matrix_NewMtx(GraphicsContext *c, char * = nullptr, int = 0) {
  return Matrix_ToMtx((Mtx *)Graph_Alloc(c, sizeof(Mtx)));
}
#define MATRIX_FINALIZE_AND_LOAD(p, c) gSPMatrix(p, Matrix_NewMtx(c), 0)
template <class T> T *Lib_SegmentedToVirtual(T *p) { return p; }
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
void Gfx_SetupDL_25Opa(GraphicsContext *) {}
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
template <class... T> Gfx *Gfx_TwoTexScrollEx(T...) {
  static Gfx p;
  return &p;
}
template <class... T> Gfx *Gfx_TexScrollEx(T...) {
  static Gfx p;
  return &p;
}
#ifdef HOST_MM
float Rand_ZeroOne() { return 0.0f; }
static const char gameplay_keep_DL_01ACF0[] = "__OTR__objects/gameplay_keep/gameplay_keep_DL_01ACF0";
#include "native_soul.inc"
using LimbArg = Actor *;
#define COMBO_FOREIGN_ANIM_HOST_MM 1
#else
using LimbArg = void *;
#endif
using OverrideLimbDrawOpa = s32 (*)(PlayState *, s32, Gfx **, Vec3f *, Vec3s *,
                                    LimbArg);
using PostLimbDrawOpa = void (*)(PlayState *, s32, Gfx **, Vec3s *, LimbArg);
#include "native_draw.inc"
static int normalInits = 0, flexInits = 0, updates = 0;
void SkelAnime_Init(PlayState *, SkelAnime *a, SkeletonHeader *h,
                    AnimationHeader *, Vec3s *j, Vec3s *, int) {
  normalInits++;
  a->skeleton = h->segment;
  a->jointTable = j;
  j[0] = {10, 0, 0};
}
void SkelAnime_InitFlex(PlayState *, SkelAnime *a, FlexSkeletonHeader *h,
                        AnimationHeader *, Vec3s *j, Vec3s *, int) {
  flexInits++;
  a->skeleton = h->sh.segment;
  a->jointTable = j;
  a->dListCount = h->dListCount;
  j[0] = {10, 0, 0};
}
s32 SkelAnime_Update(SkelAnime *) {
  updates++;
  return 0;
}
template <class... T>
Gfx *SkelAnime_DrawFlex(PlayState *, void **, Vec3s *, int, T...) {
  assert(false);
  return nullptr;
}
namespace Ship {
struct IResource {
  virtual ~IResource() = default;
  virtual void *GetRawPointer() = 0;
};
struct ResourceManager {
  bool alt = false;
  std::unordered_map<std::string, std::shared_ptr<IResource>> normal,
      replacement;
  bool IsAltAssetsEnabled() { return alt; }
  std::shared_ptr<IResource> LoadResource(const char *p) {
    auto &m = alt ? replacement : normal;
    return m[p];
  }
};
struct Context {
  std::shared_ptr<ResourceManager> rm;
  static Context *GetRawInstance() {
    static Context c;
    return &c;
  }
  std::shared_ptr<ResourceManager> GetResourceManager() { return rm; }
};
struct ResourceManagerScope {
  std::shared_ptr<ResourceManager> before;
  ResourceManagerScope(std::shared_ptr<ResourceManager> r) {
    auto c = Context::GetRawInstance();
    before = c->rm;
    c->rm = r;
  }
  ~ResourceManagerScope() { Context::GetRawInstance()->rm = before; }
};
struct CrossRMRegistry {
  inline static std::unordered_map<std::string,
                                   std::shared_ptr<ResourceManager>>
      owners;
  static std::shared_ptr<ResourceManager> Get(const char *s) {
    return owners[s];
  }
  static void RegisterTeardownListener(void (*)()){};
};
} // namespace Ship
extern "C" void FrameInterpolation_RecordOpenChild(const void *, int) {}
extern "C" void FrameInterpolation_RecordCloseChild() {}
#include "foreign_anim.h"
struct SkeletonResource : Ship::IResource {
  CfaLoadedFlexSkeletonHeader h{};
  StandardLimb limbs[2]{};
  void *pointers[2]{};
  std::string head = "__OTR__head", jaw = "__OTR__jaw";
  SkeletonResource(uint8_t type) {
    h.sh = {pointers, 2, type};
    h.dListCount = 2;
    limbs[0].child = 1;
    limbs[0].dList = (Gfx *)head.c_str();
    limbs[1].jointPos = {3, 0, 0};
    limbs[1].dList = (Gfx *)jaw.c_str();
    pointers[0] = &limbs[0];
    pointers[1] = &limbs[1];
  }
  void *GetRawPointer() override { return &h; }
};
struct AnimationResource : Ship::IResource {
  AnimationHeader h;
  void *GetRawPointer() override { return &h; }
};
// Replay the recorded native draw stream after all matrices have been written,
// as the renderer does. The jaw samples root matrix 0 plus its own matrix 1
// through segment 13.
void checkDraw(GraphicsContext &c, bool flex, const char *game) {
  uintptr_t seg13 = 0;
  Mtx *model = nullptr;
  int meshes = 0, bindings = 0;
  bool jaw = false;
  for (Gfx *p = c.opa; p < c.op; p++) {
    if (p->kind == 1 && p->seg == 13) {
      seg13 = p->target;
      bindings++;
    }
    if (p->kind == 2)
      model = (Mtx *)p->target;
    if (p->kind == 3) {
      meshes++;
      const char *path = (const char *)p->target;
      assert(
          std::string(path).starts_with(std::string("__OTR__@") + game + ":"));
      if (std::string(path).ends_with(":jaw")) {
        jaw = true;
        assert(model && model->x == 13);
        if (flex) {
          assert(seg13);
          auto m = (Mtx *)seg13;
          assert(m[0].x == 10 && m[1].x == 13);
        }
      }
    }
  }
  assert(meshes == 2 && jaw);
  assert(bindings == (flex ? 2 : 0));
  assert(matrices.empty());
}
// Replay with deliberately transparent inherited shelf state. A replacement flame
// that omits local prim/env commands must still receive an opaque neutral base,
// its boss tint, the owner RM and a valid scroll without perturbing the jaw.
void checkFlame(GraphicsContext& c, const char* game) {
  uintptr_t prim = 0, env = 0, tint = 0, segment8 = 0;
  bool grayscale = false, saw = false;
  std::vector<std::string> owners;
  for (Gfx* p = c.xlu; p < c.xp; ++p) {
    if (p->kind == 1 && p->seg == 8) segment8 = p->target;
    if (p->kind == 4) prim = p->target;
    if (p->kind == 5) env = p->target;
    if (p->kind == 6) tint = p->target;
    if (p->kind == 7) grayscale = p->target;
    if (p->kind == 8) owners.emplace_back((const char*)p->target);
    if (p->kind == 9) { assert(!owners.empty()); owners.pop_back(); }
    if (p->kind == 3) {
      assert(std::string((const char*)p->target) == std::string("__OTR__@") + game + ":flame");
      assert(prim == 0xffffffff && env == 0xffffffff);
      assert(grayscale && tint == 0xed5f5fff && segment8);
      assert(!owners.empty() && owners.back() == game);
      saw = true;
    }
  }
  assert(saw && !grayscale && owners.empty());
}
int main() {
  auto oot = std::make_shared<Ship::ResourceManager>(),
       mm = std::make_shared<Ship::ResourceManager>();
  Ship::CrossRMRegistry::owners = {{"oot", oot}, {"mm", mm}};
  Ship::Context::GetRawInstance()->rm = mm;
  auto anim = std::make_shared<AnimationResource>();
  for (auto owner : {oot, mm}) {
    owner->normal = {{"__OTR__skel", std::make_shared<SkeletonResource>(0)},
                     {"__OTR__anim", anim}};
    owner->replacement = {
        {"__OTR__skel", std::make_shared<SkeletonResource>(1)},
        {"__OTR__anim", anim}};
  }
  CwItemAnimDrawInfo info{};
  info.opa = 1;
  info.skelPath = "__OTR__skel";
  info.animPath = "__OTR__anim";
  info.limbCount = 3;
  info.hiddenLimb = -1;
  info.nonFlexSkeleton = 1;
  // Start directly on the replacement: this isolates loaded-type dispatch from
  // stale caching.
  oot->alt = true;
  {
    GraphicsContext c;
    PlayState p{{&c, 3}, 3, {}};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    checkDraw(c, true, "oot");
  }
  CfaClearCaches();
  normalInits = flexInits = updates = 0;
  for (const char *game : {"oot", "mm"})
    for (bool alt : {false, true, false, true}) {
      auto owner = Ship::CrossRMRegistry::Get(game);
      owner->alt = alt;
      GraphicsContext c;
      PlayState play{{&c, 4}, 4, {}};
      current = {};
      assert(ComboForeignAnim_Draw(&info, game, &play) == 1);
      checkDraw(c, alt, game);
      assert(Ship::Context::GetRawInstance()->rm == mm);
    }
  assert(normalInits == 2 && flexInits == 2 &&
         updates == 4); // owners and selections cache independently
  // Full composite path with the flame present, before the model, in both
  // directions and repeatedly toggled between rigid vanilla and flex Alt.
  info.flameDlPath = "__OTR__flame";
  info.flameGrayscale = 1;
  info.flameColor[0] = 237; info.flameColor[1] = 95; info.flameColor[2] = 95;
  info.flameHasSeg = 1;
  info.flameSeg.kind = CW_ANIM_SEG_TEXSCROLL;
  info.flameSeg.segment = 8; info.flameSeg.onXlu = 1;
  info.flameSeg.width1 = info.flameSeg.width2 = 16;
  info.flameSeg.height1 = info.flameSeg.height2 = 32;
  info.flameSeg.xStep2 = 1; info.flameSeg.yStep2 = -8;
  for (const char* game : {"oot", "mm"}) {
    for (bool alt : {false, true, false}) {
      Ship::CrossRMRegistry::Get(game)->alt = alt;
      GraphicsContext c;
      PlayState p{{&c, 7}, 7, {}};
      current = {};
      assert(ComboForeignAnim_Draw(&info, game, &p));
      checkDraw(c, alt, game);
      checkFlame(c, game);
    }
  }
  info.flameDlPath = nullptr;
#ifdef HOST_MM
  // The actual native MM soul renderer surrounds the owner-routed OoT model.
  // Repeated OoT Alt switches must change the jaw dispatch, never effect ownership.
  info.flameDlPath = "__OTR__objects/object_gi_fire/gGiBlueFireFlameDL";
  info.flameTranslate[1] = -70.0f;
  info.flameScale[0] = info.flameScale[1] = info.flameScale[2] = 5.0f;
  for (bool alt : {false, true, false}) {
    oot->alt = alt;
    GraphicsContext c;
    PlayState p{{&c, 8}, 8, {}};
    current = {};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    checkDraw(c, alt, "oot");
    std::vector<std::string> owners;
    uintptr_t prim = 0, env = 0, seg8 = 0;
    bool grayscale = true, sawNative = false;
    Mtx* flameMatrix = nullptr;
    for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) {
      if (cmd->kind == 8) owners.emplace_back((const char*)cmd->target);
      if (cmd->kind == 9) { assert(!owners.empty()); owners.pop_back(); }
      if (cmd->kind == 4) prim = cmd->target;
      if (cmd->kind == 5) env = cmd->target;
      if (cmd->kind == 7) grayscale = cmd->target;
      if (cmd->kind == 1 && cmd->seg == 8) seg8 = cmd->target;
      if (cmd->kind == 2) flameMatrix = (Mtx*)cmd->target;
      if (cmd->kind == 3) {
        assert(std::string((const char*)cmd->target) == gameplay_keep_DL_01ACF0);
        assert(owners.back() == "mm" && seg8 && !grayscale);
        assert(prim == 0xed5f5f00 && env == 0xed5f5f00);
        assert(flameMatrix && fabs(flameMatrix->sx * 400 - 60) < .001f);
        assert(fabs(flameMatrix->y - flameMatrix->sy * 480 + 60) < .001f);
        assert(fabs(flameMatrix->y + flameMatrix->sy * 1440 - 140) < .001f);
        assert(fabs(flameMatrix->sz - .15f) < .001f);
        sawNative = true;
      }
    }
    assert(sawNative && owners.empty() && !grayscale);
    assert(prim == 0xffffffff && env == 0xffffffff);
    assert(((Gfx*)seg8)[0].kind == 0 && matrices.empty());
  }
  info.flameDlPath = nullptr;
#endif
  // Recipe flag intentionally disagrees in the other direction too: loaded
  // normal wins.
  CfaClearCaches();
  info.nonFlexSkeleton = 0;
  oot->alt = false;
  {
    GraphicsContext c;
    PlayState p{{&c, 5}, 5, {}};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    checkDraw(c, false, "oot");
  }
  // Unsupported curve, mismatched limb count, absent owner and missing
  // animation emit nothing.
  for (int failure = 0; failure < 4; failure++) {
    CfaClearCaches();
    auto bad = std::make_shared<SkeletonResource>(failure == 0 ? 2 : 0);
    oot->normal["__OTR__skel"] = bad;
    info.limbCount = failure == 1 ? 4 : 3;
    oot->normal["__OTR__anim"] = failure == 3 ? nullptr : anim;
    GraphicsContext c;
    PlayState p{{&c, 6}, 6, {}};
    assert(!ComboForeignAnim_Draw(&info, failure == 2 ? "absent" : "oot", &p));
    assert(c.op == c.opa && c.xp == c.xlu);
  }
  CfaClearCaches();
  std::cout << "Foreign soul loaded-type dispatch, native flex matrix/jaw "
               "replay, Alt/cache and fallback checks passed\n";
}
