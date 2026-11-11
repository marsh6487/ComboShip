// Command/CPU boundary test. run_renderer_tests.py compiles exact production
// allocation, matrix, setup, scroll and debug functions against native headers.
// The only substitutes below are resource lookups, logging and interpolation
// recording. This does not emulate the GPU or establish in-game appearance.
#include "combo/render/ItemGetLightShaft.h"
#include "z64.h"
extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
void RendererFixture_ResetNativeStack(const MtxF *, int);
MtxF *RendererFixture_CurrentNativeMatrix();
int RendererFixture_NativeStackDepth();
}
#ifdef SHAFT_TEST_MM
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#define GI_SHAFT_ALLOC(gfx, bytes) GRAPH_ALLOC(gfx, bytes)
#define GI_SHAFT_SETUP_XLU(gfx) Gfx_SetupDL25_Xlu(gfx)
#define GI_SHAFT_LOAD_MATRIX(gfx) MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfx)
extern "C" Gfx gSetupDLs[SETUPDL_MAX][6];
static Gfx *NativeSetup() { return gSetupDLs[SETUPDL_25]; }
#else
#include "soh/frame_interpolation.h"
#define GI_SHAFT_ALLOC(gfx, bytes) Graph_Alloc(gfx, bytes)
#define GI_SHAFT_SETUP_XLU(gfx) Gfx_SetupDL_25Xlu(gfx)
#define GI_SHAFT_LOAD_MATRIX(gfx)                                              \
  gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfx),                               \
            G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH)
extern "C" Gfx sSetupDL[SETUPDL_MAX][6];
static Gfx *NativeSetup() { return sSetupDL[SETUPDL_25]; }
#endif
#include "combo/render/ItemGetLightShaftRenderer.inc"

#include <bit>
#include <cassert>
#include <cstdio>
#include <set>
#include <tuple>
#include <vector>

static_assert(sizeof(Vtx{}.v.ob[0]) == sizeof(int32_t),
              "test the real 32-bit vertex ABI");
static_assert(sizeof(Mtx{}.mf[0][0]) == sizeof(float),
              "test the real float matrix ABI");
static_assert(sizeof(Gfx{}.words.w1) == sizeof(uintptr_t),
              "display lists must retain native pointers");

namespace Fixture {
struct Child {
  const void *identity;
  int generation;
};
static std::vector<Child> children, openChildren;
static int childCloses, matrixPushes, matrixPops, matrixTranslations,
    matrixWrites, archiveChecks;
static std::vector<Mtx *> writtenMatrices;
static size_t maximumOpaBytes, maximumXluCommands;

struct alignas(16) Buffer {
  std::array<Gfx, 4> prefix;
  std::array<Gfx, 512> commands;
  std::array<Gfx, 4> suffix;
};
static Buffer opa, xlu, overlay;
static GraphicsContext gfx;
static PlayState play;
static MtxF caller;

static unsigned Opcode(const Gfx &command) {
  return uint32_t(command.words.w0) >> 24;
}

static void Reset(int depth = 0) {
  std::memset(&gfx, 0, sizeof(gfx));
  std::memset(&play, 0, sizeof(play));
  std::memset(&opa, 0xA5, sizeof(opa));
  std::memset(&xlu, 0xA5, sizeof(xlu));
  std::memset(&overlay, 0xA5, sizeof(overlay));
  gfx.polyOpa.p = opa.commands.data();
  gfx.polyOpa.d = opa.commands.data() + opa.commands.size();
  gfx.polyXlu.p = xlu.commands.data();
  gfx.polyXlu.d = xlu.commands.data() + xlu.commands.size();
  gfx.overlay.p = overlay.commands.data();
  gfx.overlay.d = overlay.commands.data() + overlay.commands.size();
  play.state.gfxCtx = &gfx;
  play.view.eye = {110, 95, 600};
  // A rotated/scaled/sheared caller exposes accidentally inherited GI poses.
  std::memset(&caller, 0, sizeof(caller));
  caller.xx = 2;
  caller.xy = 0.25f;
  caller.xz = 0.5f;
  caller.xw = 101;
  caller.yx = 0.75f;
  caller.yy = 3;
  caller.yz = -0.25f;
  caller.yw = -22;
  caller.zx = 0.5f;
  caller.zy = -0.75f;
  caller.zz = 4;
  caller.zw = 303;
  caller.ww = 1;
  RendererFixture_ResetNativeStack(&caller, depth);
  children.clear();
  openChildren.clear();
  writtenMatrices.clear();
  childCloses = matrixPushes = matrixPops = matrixTranslations = matrixWrites =
      archiveChecks = 0;
}

static ItemGetLightShaft::Frame Visible(float height = 44,
                                        float itemHeight = 58) {
  ItemGetLightShaft::State state;
  ItemGetLightShaft::Input input;
  input.enabled = true;
  input.playIdentity = 1;
  input.frame = 100;
  input.seconds = 0.1f;
  input.presentation = {true, {100, 20, -300}, height, itemHeight};
  state.Step(input);
  ++input.frame;
  return state.Step(input);
}

static void CheckNoDraw(PlayState *target,
                        const ItemGetLightShaft::Frame &frame) {
  const GraphicsContext beforeGfx = gfx;
  const Buffer beforeOpa = opa, beforeXlu = xlu, beforeOverlay = overlay;
  const int depth = RendererFixture_NativeStackDepth();
  const MtxF before = *RendererFixture_CurrentNativeMatrix();
  ItemGetLightShaft_Draw(target, frame);
  assert(!std::memcmp(&beforeGfx, &gfx, sizeof(gfx)));
  assert(!std::memcmp(&beforeOpa, &opa, sizeof(opa)));
  assert(!std::memcmp(&beforeXlu, &xlu, sizeof(xlu)));
  assert(!std::memcmp(&beforeOverlay, &overlay, sizeof(overlay)));
  assert(!std::memcmp(&before, RendererFixture_CurrentNativeMatrix(),
                      sizeof(before)));
  assert(RendererFixture_NativeStackDepth() == depth);
  assert(children.empty() && openChildren.empty() && writtenMatrices.empty());
  assert(childCloses == 0 && matrixPushes == 0 && matrixPops == 0 &&
         matrixTranslations == 0 && archiveChecks == 0);
}

static bool InOpa(const void *pointer, size_t bytes) {
  const uintptr_t value = reinterpret_cast<uintptr_t>(pointer);
  return value >= reinterpret_cast<uintptr_t>(gfx.polyOpa.d) &&
         value + bytes <= reinterpret_cast<uintptr_t>(opa.commands.data() +
                                                      opa.commands.size());
}

struct Material {
  uint32_t geometry = 0, high = 0, low = 0;
  uint32_t primitive = 0x01020304, environment = 0x05060708;
  uintptr_t combine0 = 0, combine1 = 0;
  bool grayscale = true;
};

static void ApplyMaterial(Material &state, const Gfx &command) {
  const uint32_t w0 = command.words.w0, w1 = command.words.w1;
  switch (Opcode(command)) {
  case G_GEOMETRYMODE:
    state.geometry = (state.geometry & (w0 & 0xFFFFFF)) | w1;
    break;
  case G_RDPSETOTHERMODE:
    state.high = w0 & 0xFFFFFF;
    state.low = w1;
    break;
  case G_SETOTHERMODE_H:
  case G_SETOTHERMODE_L: {
    const unsigned length = (w0 & 255) + 1,
                   shift = 32 - ((w0 >> 8) & 255) - length;
    assert(length <= 32 && shift < 32);
    const uint32_t mask = uint32_t((uint64_t(1) << length) - 1) << shift;
    auto &field = Opcode(command) == G_SETOTHERMODE_H ? state.high : state.low;
    field = (field & ~mask) | (w1 & mask);
    break;
  }
  case G_SETPRIMCOLOR:
    state.primitive = w1;
    break;
  case G_SETENVCOLOR:
    state.environment = w1;
    break;
  case G_SETCOMBINE:
    state.combine0 = command.words.w0;
    state.combine1 = command.words.w1;
    break;
  case G_SETGRAYSCALE:
    state.grayscale = w1 != 0;
    break;
  }
}

static void CheckLightColorMux(const Material &material) {
  // Decode the actual submitted G_SETCOMBINE fields. In both cycles RGB must
  // be (0 - 0) * 0 + SHADE, while alpha is (TEXEL0 - 0) * SHADE + 0.
  // Thus the I8 intensity controls coverage without making ivory light gray
  // or brown. Checking selectors avoids adding a replica shader to the test.
  const uint32_t w0 = material.combine0, w1 = material.combine1;
  const std::array<unsigned, 8> cycles[] = {
      {(w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7,
       (w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7},
      {(w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7, (w1 >> 21) & 7,
       (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7},
  };
  const std::array<unsigned, 8> expected = {
      G_CCMUX_0 & 15, G_CCMUX_0 & 15, G_CCMUX_0,     G_CCMUX_SHADE,
      G_ACMUX_TEXEL0, G_ACMUX_0,      G_ACMUX_SHADE, G_ACMUX_0};
  for (const auto &cycle : cycles) {
    if (cycle != expected)
      std::fprintf(stderr,
                   "light color mux: RGB=(%u-%u)*%u+%u, alpha=(%u-%u)*%u+%u; "
                   "expected constant SHADE RGB and TEXEL0*SHADE alpha\n",
                   cycle[0], cycle[1], cycle[2], cycle[3], cycle[4], cycle[5],
                   cycle[6], cycle[7]);
    assert(cycle == expected && "shaft texture intensity must modulate alpha "
                                "only, preserving warm SHADE RGB");
  }
}

static void CheckScroll(const Gfx *list, uint32_t scroll, int32_t scrollStep) {
  assert(InOpa(list, 7 * sizeof(Gfx)));
  assert(Opcode(list[0]) == G_RDPTILESYNC &&
         Opcode(list[1]) == G_SETTILESIZE_LERP);
  assert(Opcode(list[6]) == G_ENDDL);
  const auto floatWord = [](uintptr_t word) {
    return std::bit_cast<float>(uint32_t(word));
  };
  assert(floatWord(list[2].words.w0) == 0 &&
         floatWord(list[2].words.w1) == float(scroll));
  assert(floatWord(list[3].words.w0) == 124 &&
         floatWord(list[3].words.w1) == float(scroll + 252));
  assert(floatWord(list[4].words.w0) == 0 &&
         floatWord(list[4].words.w1) == float(int32_t(scroll) + scrollStep));
  assert(floatWord(list[5].words.w0) == 124 &&
         floatWord(list[5].words.w1) ==
             float(int32_t(scroll) + scrollStep + 252));
}

static void CheckVertices(const Vtx *vertices, size_t count,
                          const ItemGetLightShaft::Frame &frame) {
  assert(count == 30 && InOpa(vertices, count * sizeof(Vtx)));
  bool interiorAlpha = false;
  for (size_t i = 0; i < count; ++i) {
    const auto &vertex = vertices[i].v;
    const size_t row = i / 5, column = i % 5;
    assert(vertex.cn[0] == 255 && vertex.cn[1] == 244 && vertex.cn[2] == 214);
    assert(vertex.cn[3] <= 82);
    if (row == 0 || row == 5 || column == 0 || column == 4)
      assert(vertex.cn[3] == 0);
    else
      interiorAlpha |= vertex.cn[3] > 0;
    assert(vertex.ob[1] >= 0 && vertex.ob[1] <= std::ceil(frame.height));
    assert(std::hypot(float(vertex.ob[0]), float(vertex.ob[2])) <=
           frame.bottomWidth + 1.5f);
    assert(vertex.tc[0] >= 0 && vertex.tc[0] <= 31 * 32);
    assert(vertex.tc[1] >= 0 && vertex.tc[1] <= 63 * 32);
    if (column == 2)
      assert(vertex.ob[0] == 0 && vertex.ob[2] == 0);
    if (row == 0)
      assert(vertex.ob[1] == 0);
    if (row == 5)
      assert(vertex.ob[1] == std::lround(frame.height));
    // Every row is horizontal even when camera pitch/yaw changes.
    if (column)
      assert(vertex.ob[1] == vertices[i - 1].v.ob[1]);
  }
  assert(interiorAlpha);
}

static void CheckGuards(const Buffer &buffer) {
  const auto *first =
      reinterpret_cast<const unsigned char *>(buffer.prefix.data());
  const auto *last =
      reinterpret_cast<const unsigned char *>(buffer.suffix.data());
  for (size_t i = 0; i < sizeof(buffer.prefix); ++i)
    assert(first[i] == 0xA5 && last[i] == 0xA5);
}

static void CheckVisible(const ItemGetLightShaft::Frame &frame) {
  const int depth = RendererFixture_NativeStackDepth();
  const auto tail = gfx.polyOpa.d;
  const auto xluHead = gfx.polyXlu.p;
  const auto xluTail = gfx.polyXlu.d, overlayTail = gfx.overlay.d;
  const auto beforeTexture = ItemGetLightShaft::Texture();
  const auto *texturePointer = ItemGetLightShaft::Texture().data();
  ItemGetLightShaft_Draw(&play, frame);
  assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
         gfx.overlay.p <= gfx.overlay.d);
  assert(gfx.polyXlu.d == xluTail && gfx.overlay.d == overlayTail);
  assert(gfx.polyOpa.p == opa.commands.data() &&
         gfx.overlay.p == overlay.commands.data());
  assert(RendererFixture_NativeStackDepth() == depth);
  assert(!std::memcmp(&caller, RendererFixture_CurrentNativeMatrix(),
                      sizeof(caller)));
  assert(matrixPushes == 1 && matrixPops == 1 && matrixTranslations == 1 &&
         matrixWrites == 2);
  assert(writtenMatrices.size() == 2 && openChildren.empty() &&
         children.size() == size_t(childCloses));
  assert(children.size() == 4 &&
         children.front().generation == int(frame.generation));
  assert(ItemGetLightShaft::Texture().data() == texturePointer &&
         ItemGetLightShaft::Texture() == beforeTexture);
  CheckGuards(opa);
  CheckGuards(xlu);
  CheckGuards(overlay);

  Material material;
  size_t setupCalls = 0, scrollCalls = 0, matrices = 0, vertexLoads = 0,
         triangles = 0, textures = 0, renderTiles = 0;
  const Vtx *loadedVertices = nullptr;
  size_t loadedCount = 0;
  std::set<std::tuple<unsigned, unsigned, unsigned>> uniqueTriangles;
  for (const Gfx *command = xluHead; command < gfx.polyXlu.p; ++command) {
    ApplyMaterial(material, *command);
    switch (Opcode(*command)) {
    case G_DL: {
      const auto *list = reinterpret_cast<const Gfx *>(command->words.w1);
      if (list == NativeSetup()) {
        ++setupCalls;
        for (size_t i = 0; i < 6; ++i)
          ApplyMaterial(material, list[i]);
      } else {
        ++scrollCalls;
        CheckScroll(list, frame.scroll, frame.scrollStep);
      }
      break;
    }
    case G_MTX: {
      const auto *matrix = reinterpret_cast<const Mtx *>(command->words.w1);
      assert(matrices < 2 && matrix == writtenMatrices[matrices] &&
             InOpa(matrix, sizeof(Mtx)));
      if (matrices++ == 0) {
        for (int column = 0; column < 4; ++column)
          for (int row = 0; row < 4; ++row) {
            const float expected = column == 3 && row < 3
                                       ? (row == 0   ? frame.origin.x
                                          : row == 1 ? frame.origin.y
                                                     : frame.origin.z)
                                       : float(column == row);
            assert(matrix->mf[column][row] == expected);
          }
      } else
        assert(!std::memcmp(matrix->mf, caller.mf, sizeof(caller.mf)));
      break;
    }
    case G_VTX:
      ++vertexLoads;
      loadedCount = (command->words.w0 >> 12) & 255;
      loadedVertices = reinterpret_cast<const Vtx *>(command->words.w1);
      CheckVertices(loadedVertices, loadedCount, frame);
      break;
    case G_TRI2: {
      assert(loadedVertices && matrices == 1 && setupCalls == 1 &&
             scrollCalls == 1);
      assert((material.geometry & (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH)) ==
             (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH));
      assert(!(material.geometry &
               (G_LIGHTING | G_FOG | G_CULL_BACK | G_CULL_FRONT |
                G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR)));
      assert((material.low & Z_CMP) && !(material.low & Z_UPD) &&
             (material.low & FORCE_BL));
      assert((material.high & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_1CYCLE);
      assert((material.low & 3) == G_AC_NONE && !material.grayscale);
      assert(material.primitive == 0xFFFFFFFF &&
             material.environment == 0xFFFFFFFF);
      CheckLightColorMux(material);
      for (uintptr_t word : {command->words.w0, command->words.w1}) {
        const unsigned a = ((word >> 16) & 255) / 2,
                       b = ((word >> 8) & 255) / 2, c = (word & 255) / 2;
        assert(a < loadedCount && b < loadedCount && c < loadedCount &&
               a != b && a != c && b != c);
        const unsigned minRow = std::min({a / 5, b / 5, c / 5});
        const unsigned maxRow = std::max({a / 5, b / 5, c / 5});
        const unsigned minCol = std::min({a % 5, b % 5, c % 5});
        const unsigned maxCol = std::max({a % 5, b % 5, c % 5});
        assert(maxRow == minRow + 1 && maxCol == minCol + 1);
        assert(uniqueTriangles.emplace(a, b, c).second);
        ++triangles;
      }
      break;
    }
    case G_SETTIMG:
      ++textures;
      assert(command->words.w1 == reinterpret_cast<uintptr_t>(texturePointer));
      assert(((command->words.w0 >> 21) & 7) == G_IM_FMT_I);
      break;
    case G_SETTILE:
      if (((command->words.w1 >> 24) & 7) == G_TX_RENDERTILE) {
        ++renderTiles;
        assert(((command->words.w0 >> 21) & 7) == G_IM_FMT_I);
        assert(((command->words.w0 >> 19) & 3) == G_IM_SIZ_8b);
        assert(((command->words.w1 >> 8) & 3) == G_TX_CLAMP);
        assert(((command->words.w1 >> 18) & 3) == G_TX_WRAP);
        assert(((command->words.w1 >> 4) & 15) == 5 &&
               ((command->words.w1 >> 14) & 15) == 6);
      }
      break;
    case G_MOVEWORD:
      assert(false && "shaft must not commandeer a dynamic segment");
      break;
    }
  }
  assert(setupCalls == 2 && scrollCalls == 1 && matrices == 2 &&
         vertexLoads == 1 && triangles == 40);
  assert(textures == 1 && renderTiles == 1);
  Material baseline;
  for (size_t i = 0; i < 6; ++i)
    ApplyMaterial(baseline, NativeSetup()[i]);
  assert(material.geometry == baseline.geometry &&
         material.high == baseline.high && material.low == baseline.low);
  assert(material.combine0 == baseline.combine0 &&
         material.combine1 == baseline.combine1);
  assert(material.primitive == 0xFFFFFFFF &&
         material.environment == 0xFFFFFFFF && !material.grayscale);
  const size_t bytes = reinterpret_cast<uintptr_t>(tail) -
                       reinterpret_cast<uintptr_t>(gfx.polyOpa.d);
  const size_t commands = gfx.polyXlu.p - xluHead;
  assert(bytes <= 2048 && commands <= 112);
  maximumOpaBytes = std::max(maximumOpaBytes, bytes);
  maximumXluCommands = std::max(maximumXluCommands, commands);
}
} // namespace Fixture

// External boundaries deliberately fail if the asset-free renderer attempts a
// resource load. Signature checks are native wrappers' benign raw-pointer
// probes.
extern "C" int ResourceMgr_OTRSigCheck(char *pointer) {
  ++Fixture::archiveChecks;
  assert(pointer && std::memcmp(pointer, "__OTR__", 7));
  return 0;
}
extern "C" Gfx *ResourceMgr_LoadGfxByName(char *) {
  assert(false);
  return nullptr;
}
extern "C" Vtx *ResourceMgr_LoadVtxByName(char *) {
  assert(false);
  return nullptr;
}
extern "C" Gfx *PakLoader_GetDLOverride(const char *) {
  assert(false);
  return nullptr;
}
extern "C" void osSyncPrintfUnused(const char *, ...) {}
extern "C" void FrameInterpolation_RecordOpenChild(const void *identity,
                                                   int generation) {
  Fixture::children.push_back({identity, generation});
  Fixture::openChildren.push_back({identity, generation});
}
extern "C" void FrameInterpolation_RecordCloseChild() {
  assert(!Fixture::openChildren.empty());
  Fixture::openChildren.pop_back();
  ++Fixture::childCloses;
}
extern "C" void FrameInterpolation_RecordMatrixPush() {
  ++Fixture::matrixPushes;
}
extern "C" void FrameInterpolation_RecordMatrixPop() { ++Fixture::matrixPops; }
extern "C" void FrameInterpolation_RecordMatrixTranslate(f32, f32, f32,
                                                         u8 mode) {
  assert(mode == MTXMODE_NEW);
  ++Fixture::matrixTranslations;
}
extern "C" void FrameInterpolation_RecordMatrixToMtx(Mtx *matrix, char *, s32) {
  ++Fixture::matrixWrites;
  Fixture::writtenMatrices.push_back(matrix);
}

int main() {
  using namespace Fixture;
  ItemGetLightShaft::State state;
  ItemGetLightShaft::Input input; // enabled=false is the production default.
  input.playIdentity = 1;
  input.presentation = {true, {100, 20, -300}, 44, 58};
  Reset();
  CheckNoDraw(&play, state.Step(input));
  Reset();
  CheckNoDraw(&play, {});
  auto frame = Visible();
  frame.visible = false;
  Reset();
  CheckNoDraw(&play, frame);
  frame = Visible();
  frame.opacity = 0;
  Reset();
  CheckNoDraw(&play, frame);
  frame = Visible();
  Reset();
  CheckNoDraw(nullptr, frame);
  Reset();
  play.state.gfxCtx = nullptr;
  CheckNoDraw(&play, frame);

  // Find the native admission threshold separately for every arena, then test
  // every smaller aligned size. Every declined call must leave the complete
  // context, all buffer bytes and stack/interpolation boundaries untouched.
  for (int buffer = 0; buffer < 3; ++buffer) {
    size_t threshold = 0;
    for (size_t capacity = 0; capacity < 512; ++capacity) {
      Reset();
      auto &arena = buffer == 0   ? gfx.polyOpa
                    : buffer == 1 ? gfx.polyXlu
                                  : gfx.overlay;
      arena.d = arena.p + capacity;
      if (ItemGetLightShaft_ArenaHasRoom(&gfx)) {
        threshold = capacity;
        break;
      }
      CheckNoDraw(&play, frame);
    }
    assert(threshold > 0 && threshold < 512);
    Reset();
    auto &admitted = buffer == 0   ? gfx.polyOpa
                     : buffer == 1 ? gfx.polyXlu
                                   : gfx.overlay;
    admitted.d = admitted.p + threshold;
    CheckVisible(frame);
    Reset();
    auto &reversed = buffer == 0   ? gfx.polyOpa
                     : buffer == 1 ? gfx.polyXlu
                                   : gfx.overlay;
    reversed.p += 2;
    reversed.d = reversed.p - 1;
    CheckNoDraw(&play, frame);
    Reset();
    auto &absent = buffer == 0   ? gfx.polyOpa
                   : buffer == 1 ? gfx.polyXlu
                                 : gfx.overlay;
    absent.p = nullptr;
    CheckNoDraw(&play, frame);
  }

  const void *identity = nullptr;
  size_t draws = 0;
  for (int depth : {0, 7, 18})
    for (float height : {24.f, 44.f, 68.f, 120.f, 2048.f})
      for (uint32_t scroll : {0u, 1u, 7u, 127u, 255u})
        for (int camera = 0; camera < 4; ++camera) {
          Reset(depth);
          frame = Visible(height, height == 2048.f ? 4096.f : height * 1.8f);
          frame.scroll = scroll;
          frame.generation = uint32_t(draws / 12);
          if (camera == 1)
            play.view.eye = {-700, 400, -300};
          if (camera == 2)
            play.view.eye = {100, 800, -300}; // exactly overhead fallback
          if (camera == 3)
            play.view.eye = {100, -500, -900};
          CheckVisible(frame);
          if (!identity)
            identity = children.front().identity;
          assert(identity == children.front().identity);
          ++draws;
        }
#ifdef SHAFT_TEST_MM
  const char *host = "MM";
#else
  const char *host = "OoT";
#endif
#ifdef COMBO_BUILD
  const char *mode = "combo";
#else
  const char *mode = "original";
#endif
  std::printf("PASS %s %s renderer: default-off/invisible no-op, all arena "
              "shortfalls, %zu native draws; "
              "30 feathered vertices/40 triangles, I8 scrolling, "
              "depth-test/no-write, material + matrix restore, "
              "balanced interpolation; peak %zu OPA bytes/%zu XLU commands\n",
              host, mode, draws, maximumOpaBytes, maximumXluCommands);
}
