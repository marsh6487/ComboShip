#pragma once
#include "ComboCollectibleShimmerDraw.h"
#include "ComboMaskShimmer.h"
#include <array>
#include <bit>

// Hand-derived phases catch a frozen scroll, incorrect quarter-texel scale,
// signed overflow and backwards interpolation at the repeating texture seam.
inline constexpr std::array<std::pair<uint32_t, float>, 10> ClockFrames = {{
    {0u, 0.f},
    {1u, 1.f},
    {42u, 42.f},
    {126u, 126.f},
    {127u, 127.f},
    {128u, 0.f},
    {129u, 1.f},
    {16384u, 0.f},
    {UINT32_MAX - 1, 126.f},
    {UINT32_MAX, 127.f},
}};

static void CheckGlassCommands(Gfx *begin, Gfx *end, const char *texture,
                               const char *glass, float phase,
                               const char *nativeOwner = nullptr) {
  int textures = 0, scrolls = 0, lenses = 0;
  std::vector<std::string> owners;
  for (Gfx *p = begin; p < end; ++p) {
    const auto op = p->words.w0 >> 24;
    assert(!(op == G_MOVEWORD && ((p->words.w0 >> 16) & 255) == G_MW_SEGMENT) &&
           "the Clock must leave every scene segment binding untouched");
    if (op == G_COMBO_RM_PUSH)
      owners.emplace_back(reinterpret_cast<const char *>(p->words.w1));
    if (op == G_COMBO_RM_POP) {
      assert(!owners.empty());
      owners.pop_back();
    }
    if (op == G_DL_OTR_FILEPATH) {
      const char *path = reinterpret_cast<const char *>(p->words.w1);
      if (!std::strcmp(path, texture)) {
        assert(!textures && !scrolls && !lenses);
        if (nativeOwner)
          assert(!owners.empty() && owners.back() == nativeOwner);
        ++textures;
      }
      if (!std::strcmp(path, glass)) {
        assert(textures == 1 && scrolls == 1 && lenses == 0);
        if (nativeOwner)
          assert(!owners.empty() && owners.back() == nativeOwner);
        ++lenses;
      }
    }
    if (op == G_SETTILESIZE_LERP) {
      assert(textures == 1 && !scrolls && !lenses && p + 5 <= end);
      assert(((p->words.w1 >> 24) & 7) == G_TX_RENDERTILE);
      const auto value = [](uintptr_t word) {
        return std::bit_cast<float>(uint32_t(word));
      };
      const std::array<float, 8> actual = {
          value(p[1].words.w0), value(p[1].words.w1), value(p[2].words.w0),
          value(p[2].words.w1), value(p[3].words.w0), value(p[3].words.w1),
          value(p[4].words.w0), value(p[4].words.w1),
      };
      const std::array<float, 8> expected = {
          phase,       0.f, phase + 124.f, 124.f,
          phase + 1.f, 0.f, phase + 125.f, 124.f};
      assert(actual == expected);
      ++scrolls;
      p += 4;
    }
  }
  assert(owners.empty() && textures == 1 && scrolls == 1 && lenses == 1);
}

static void CheckNoGlassCommands(Gfx *begin, Gfx *end) {
  for (Gfx *p = begin; p < end; ++p) {
    assert(p->words.w0 >> 24 != G_SETTILESIZE_LERP);
    if (p->words.w0 >> 24 == G_DL_OTR_FILEPATH)
      assert(!std::strstr(reinterpret_cast<const char *>(p->words.w1),
                          "gNeiClockGlass"));
  }
}

// These are unrelated compositor branches. Reaching one means the actual
// Clock recipe/dispatch is wrong; they never fabricate Clock output.
template <class... A> static bool ComboForeignAnim_Draw(A &&...) {
  assert(false);
  return false;
}
template <class... A> static bool CfaIsOtrPath(A &&...) {
  assert(false);
  return false;
}
template <class... A> static void ComboDrawSpinAttackGi(A &&...) {
  assert(false);
}
template <class... A> static int ComboBottleContents_Profile(A &&...) {
  assert(false);
  return 0;
}
template <class... A> static bool ComboBottleContents_Draw(A &&...) {
  assert(false);
  return false;
}
static bool ComboDrawMagicMirrorForeign(PlayState *, int kind) {
  assert(kind != CW_DRAW_KIND_MAGIC_MIRROR);
  return false;
}
namespace ComboMmKeyRing {
template <class... A> static void Draw(A &&...) { assert(false); }
} // namespace ComboMmKeyRing
