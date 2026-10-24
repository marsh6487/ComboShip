#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
extern "C" {
#ifdef HOST_MM
#include "global.h"
#else
#include "functions.h"
#include "macros.h"
#include "z64.h"
#endif
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
}
#include "combo/render/SpinAttackNativeMaterial.h"
#include <libultraship/bridge/consolevariablebridge.h>

static Gfx xlu[2048], opa[32];
alignas(16) static uint8_t allocation[32768];
static size_t allocated;
static size_t allocationCursor, submittedVertices;
static uint32_t particleColor;
static Gfx nativeList[8], privateList[2], brokenList[2];
static uint8_t textureByte;
static bool missing, malformed, cosmeticEdited;
static GraphicsContext gfx;
static PlayState play;
extern "C" {
void *Graph_Alloc(GraphicsContext *, size_t size) {
  allocated = size;
  assert(size <= sizeof(allocation));
  size_t at = allocationCursor;
  allocationCursor += (size + 15) & ~(size_t)15;
  assert(allocationCursor <= sizeof(allocation));
  return allocation + at;
}
void gSPVertex(Gfx *p, uintptr_t vtx, int n, int v0) {
  submittedVertices += n;
  const auto *first = (const Vtx *)vtx;
  particleColor = (uint32_t)first->v.cn[0] << 16 |
                  (uint32_t)first->v.cn[1] << 8 | first->v.cn[2];
  __gSPVertex(p, vtx, n, v0);
}
void gSPDisplayList(Gfx *p, Gfx *dl) { __gSPDisplayList(p, dl); }
int32_t CVarGetInteger(const char *key, int32_t fallback) {
  return cosmeticEdited && strstr(key, ".Changed") ? 1 : fallback;
}
Color_RGB8 CVarGetColor24(const char *name, Color_RGB8 fallback) {
  return cosmeticEdited ? (strstr(name, "Secondary") ? Color_RGB8{70, 80, 90}
                                                     : Color_RGB8{10, 20, 30})
                        : fallback;
}
void *ResourceGetDataByName(const char *name) {
  if (missing && strstr(name, "cylinder"))
    return nullptr;
  if (!strncmp(name, "native", 6) || strstr(name, "gSpinAttackChargingDL"))
    return nativeList;
  if (strstr(name, "burst0") || strstr(name, "burst1") ||
      strstr(name, "charge0") || strstr(name, "charge1"))
    return &textureByte;
  return malformed ? brokenList : privateList;
}
size_t ResourceGetSizeByName(const char *name) {
  return (!strncmp(name, "native", 6) || strstr(name, "gSpinAttackChargingDL"))
             ? sizeof(nativeList)
             : sizeof(privateList);
}
uint64_t ResourceGetCrcByName(const char *name) {
  return name[strlen(name) - 1] == '0' ? 0x1111222233334444ULL
                                       : 0x5555666677778888ULL;
}
#ifdef HOST_MM
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a,
                                           const char *) {
  return cosmeticEdited ? Color_RGBA8{10, 20, 30, a} : Color_RGBA8{r, g, b, a};
}
#endif
#ifdef HOST_MM
void Graph_OpenDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
#else
void Graph_OpenDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
#endif
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild() {}
}
#include "combo/render/SpinAttackAccentsDraw.h"
#include "combo/render/SpinAttackDraw.h"
#include "spin_draw_production.inc"

int main() {
  // Catch accidental global patching, lost geometry, bad expanded-op lengths,
  // unsupported branch handling, and overwriting the caller's alpha.
  Gfx source[8] = {}, copy[8], output[20];
  source[0].words.w0 = 0xFB000000;
  source[0].words.w1 = 0x1122335b;
  source[1].words.w0 = 0x20900000;
  source[2].words.w0 = 0x1234;
  source[2].words.w1 = 0x5678;
  source[3].words.w0 = 0x32003006;
  source[4].words.w0 = 0x1357;
  source[4].words.w1 = 0x2468;
  source[5].words.w0 = 0x05000204;
  source[6].words.w0 = 0xFD900000;
  source[7].words.w0 = 0xDF000000;
  std::memcpy(copy, source, sizeof(source));
  assert(CwSpinMaterial_Clone(output, 20, source, 8, 0x1111222233334444ULL,
                              0x5555666677778888ULL, 0xaabbcc, 0x778899) == 9);
  assert(std::memcmp(copy, source, sizeof(source)) == 0);
  assert(output[0].words.w1 == 0x7788995b);
  assert(output[2].words.w0 == 0x11112222 && output[2].words.w1 == 0x33334444);
  assert(std::memcmp(output + 3, source + 3, 3 * sizeof(Gfx)) == 0);
  assert(output[7].words.w0 == 0x55556666 && output[7].words.w1 == 0x77778888);
  assert(CwSpinMaterial_Clone(output, 1, source, 8, 1, 2, 0, 0) == 0);
  source[3].words.w0 =
      0x31000000; // Uninspected child DL must retain native fallback.
  assert(CwSpinMaterial_Clone(output, 20, source, 8, 1, 2, 0, 0) == 0);
  source[0].words.w0 = 0x20900000;
  assert(CwSpinMaterial_Clone(output, 20, source, 1, 1, 2, 0, 0) == 0);
  Gfx flatTint[3] = {};
  flatTint[0].words.w0 = 0xFD900000;
  flatTint[1].words.w0 = 0xFCFF95FF;
  flatTint[1].words.w1 = 0xFF0DFEFF;
  flatTint[2].words.w0 = 0xDF000000;
  assert(CwSpinMaterial_Clone(output, 20, flatTint, 3, 1, 2, 0x0A141E,
                              0x46505A) == 4);
  assert(((output[2].words.w0 >> 20) & 15) == 3);
  assert(((output[2].words.w1 >> 28) & 15) == 5);
  play.state.gfxCtx = &gfx;
  gfx.polyXlu.p = xlu;
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
  CwSpinAccents_Draw(&play, 2, 1, .3f, .5f);
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p > xlu);
#ifdef HOST_MM
  allocated = allocation + sizeof(allocation) - (uint8_t *)gfx.polyOpa.d;
#endif
  assert(allocated > 0 && allocated <= 24 * 9 * sizeof(Vtx));
  const auto end = gfx.polyXlu.p;
  CwSpinAccents_Draw(&play, 0, 1, .3f, .5f);
  assert(gfx.polyXlu.p == end);
  std::memcpy(nativeList, copy, sizeof(nativeList));
  privateList[0].words.w0 = 0xE7000000;
  privateList[1].words.w0 = 0xDF000000;
  const auto beforeFallback = gfx.polyXlu.p;
  auto noLayer = CwSpinStyle_Resolve(0, 1, 0, 1);
  assert(!CwSpinDraw_Burst(&play, noLayer, 0, .5f, "native0", "native1"));
  assert(gfx.polyXlu.p == beforeFallback);
  missing = true;
  auto arcane = CwSpinStyle_Resolve(5, 1, 2, 1);
  assert(!CwSpinDraw_Burst(&play, arcane, 0, .5f, "native0", "native1"));
  assert(gfx.polyXlu.p == beforeFallback);
  missing = false;
  malformed = true;
  assert(!CwSpinDraw_Burst(&play, arcane, 0, .5f, "native0", "native1"));
  assert(gfx.polyXlu.p == beforeFallback);
  malformed = false;
  assert(CwSpinDraw_Burst(&play, arcane, 0, .5f, "native0", "native1"));
  assert(CwSpinDraw_Burst(&play, CwSpinStyle_Resolve(0, 1, 2, 1), 0, .5f,
                          "native0", "native1"));
  cosmeticEdited = true;
  for (int style = 0; style < CW_SPIN_STYLE_COUNT; ++style) {
    for (int great = 0; great <= 1; ++great) {
      gfx.polyXlu.p = xlu;
      gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
      submittedVertices = allocationCursor = 0;
      auto visual = CwSpinStyle_Resolve(style, 1, 2, 1);
      assert(CwSpinDraw_Charge(&play, visual, great, .25f, "native0"));
      assert((xlu[0].words.w1 >> 8) == 0x0A141E);
#ifdef HOST_MM
      assert((xlu[1].words.w1 >> 8) == 0x0A141E);
#else
      assert((xlu[1].words.w1 >> 8) == 0x46505A);
#endif
      assert(submittedVertices > 0 &&
             submittedVertices <= (great ? 8u : 4u) * 9);
      gfx.polyXlu.p = xlu;
      gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
      allocationCursor = 0;
      assert(CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1"));
      assert((xlu[0].words.w1 >> 8) == 0x0A141E);
#ifdef HOST_MM
      assert((xlu[1].words.w1 >> 8) == 0x0A141E);
#else
      assert((xlu[1].words.w1 >> 8) == 0x46505A);
#endif
    }
  }
  Player owner = {};
#ifdef HOST_MM
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &owner.actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &owner.actor;
#endif
  EnMThunder thunder = {};
#ifdef HOST_MM
  thunder.type = ENMTHUNDER_TYPE_KOKIRI_SWORD;
  thunder.subtype = ENMTHUNDER_SUBTYPE_SPIN_REGULAR;
  thunder.unk1B0 = .5f;
  thunder.chargingAlpha = 128;
  const int scene = play.sceneId;
#else
  thunder.attackStrength = 1;
  thunder.spinChargePercent = .5f;
  thunder.chargeAlpha = 128;
  const int scene = play.sceneNum;
#endif
  CwSpinAttack_Init(&thunder.spinStyle, 5, 1, 1, (uintptr_t)&owner,
                    (uintptr_t)&play, scene);
  for (int step = 0; step < 3; ++step) {
    CwSpinAttack_Preview(&thunder.spinStyle, step == 0 ? 1 : 2, 1, step == 1);
    gfx.polyXlu.p = xlu;
    gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
    submittedVertices = allocationCursor = 0;
    assert(EnMThunder_DrawChargeStyle(&thunder, &play));
    assert(submittedVertices == 4 * 5);
    assert(particleColor == (step < 2 ? 0xFF8C2D : 0x91E1FF));
  }
  CwSpinAttack_Release(&thunder.spinStyle, 2, 1, 0x01000000, 0x800, 0x1000,
                       0x2000);
  CwSpinAttack_Preview(&thunder.spinStyle, 3, 1, 0);
  submittedVertices = allocationCursor = 0;
  gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
  assert(EnMThunder_DrawChargeStyle(&thunder, &play));
  assert(thunder.spinStyle.element == 2 && submittedVertices == 0);
  assert(CwSpinDraw_Charge(&play, arcane, 1, .25f, "native0"));
  for (int element = 0; element <= 3; ++element) {
    submittedVertices = allocationCursor = 0;
    gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
    CwSpinDraw_Accents(&play, CwSpinStyle_Resolve(7, 1, element, 1), 1, .4f,
                       .6f);
    assert(submittedVertices > 0 && submittedVertices <= 24 * 9);
    assert(gfx.polyOpa.p == opa);
  }
  std::puts("PASS real-header native material clones, alpha preservation, "
            "bounded XLU accent submission");
}
