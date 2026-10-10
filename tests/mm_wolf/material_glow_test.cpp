#include <cassert>
#include <cmath>
#include <cstdio>
#include <libultraship/libultra/gbi.h>
#include <vector>

static std::vector<Gfx> sMaterialDl;
static int cullBack = 1;
static int CVarGetInteger(const char *, int) { return cullBack; }
#include "wolf-material.inc"

// Evaluate the packed native RDP combiner, rather than restating the proposed
// material. Changing either host's RGB or opacity selectors changes this
// result.
static double input(unsigned mux, unsigned slot, double texture,
                    double textureAlpha, double shade, double combined,
                    bool alpha) {
  if (alpha) {
    switch (mux) {
    case 0:
      return combined;
    case 1:
      return textureAlpha;
    case 3:
      return 1.0;
    case 4:
      return shade;
    case 6:
      return slot == 2 ? 0.0 : 1.0;
    case 7:
      return 0.0;
    }
  } else {
    switch (mux) {
    case 0:
      return combined;
    case 1:
      return texture;
    case 3:
      return 1.0;
    case 4:
      return shade;
    case 6:
      assert(slot == 0 || slot == 3);
      return 1.0;
    case 7:
      assert(slot == 3);
      return 0.0;
    case 8:
      assert(slot == 2);
      return textureAlpha;
    case 15:
      assert(slot == 0 || slot == 1);
      return 0.0;
    case 31:
      assert(slot == 2);
      return 0.0;
    }
  }
  assert(false && "unexpected material mux");
  return 0.0;
}

static double evaluate(const Gfx &command, double texture, double textureAlpha,
                       double shade, bool alpha) {
  const u32 a = command.words.w0, b = command.words.w1;
  unsigned mux[2][4];
  if (alpha) {
    mux[0][0] = (a >> 12) & 7;
    mux[0][1] = (b >> 12) & 7;
    mux[0][2] = (a >> 9) & 7;
    mux[0][3] = (b >> 9) & 7;
    mux[1][0] = (b >> 21) & 7;
    mux[1][1] = (b >> 3) & 7;
    mux[1][2] = (b >> 18) & 7;
    mux[1][3] = b & 7;
  } else {
    mux[0][0] = (a >> 20) & 15;
    mux[0][1] = (b >> 28) & 15;
    mux[0][2] = (a >> 15) & 31;
    mux[0][3] = (b >> 15) & 7;
    mux[1][0] = (a >> 5) & 15;
    mux[1][1] = (b >> 24) & 15;
    mux[1][2] = a & 31;
    mux[1][3] = (b >> 6) & 7;
  }
  double combined = 0;
  for (unsigned cycle = 0; cycle < 2; ++cycle) {
    double v[4];
    for (unsigned slot = 0; slot < 4; ++slot)
      v[slot] = input(mux[cycle][slot], slot, texture, textureAlpha, shade,
                      combined, alpha);
    combined = (v[0] - v[1]) * v[2] + v[3];
  }
  return combined;
}

int main() {
  u16 texture[8 * 8]{};
  for (cullBack = 0; cullBack <= 1; ++cullBack) {
    BuildMaterialDisplayList(texture, 8, 8);
    assert(sMaterialDl.size() <= 24);
    const Gfx *combine = nullptr;
    for (const auto &command : sMaterialDl)
      if ((command.words.w0 >> 24) == G_SETCOMBINE)
        combine = &command;
    assert(combine);
    for (double texel : {0.0, 0.125, 0.42, 0.75, 1.0}) {
      for (double shade : {0.0, 0.15, 0.42, 0.8, 1.0}) {
        assert(std::abs(evaluate(*combine, texel, 1.0, shade, false) -
                        texel * shade) < 1e-12);
        const double glowing = evaluate(*combine, texel, 0.0, shade, false);
        if (std::abs(glowing - texel) > 1e-12) {
          std::fprintf(stderr,
                       "Eye glow still follows lighting: texture=%g shade=%g "
                       "result=%g\n",
                       texel, shade, glowing);
          return 1;
        }
        for (double alpha : {0.0, 0.25, 0.5, 0.75, 1.0}) {
          assert(std::abs(evaluate(*combine, texel, alpha, shade, true) - 1.0) <
                 1e-12);
          const double expected = texel * (alpha * shade + 1.0 - alpha);
          assert(std::abs(evaluate(*combine, texel, alpha, shade, false) -
                          expected) < 1e-12);
        }
      }
    }
  }
  std::puts("Native Wolf material: lit body, full-bright masked eyes, filtered "
            "edges and opaque output pass");
}
