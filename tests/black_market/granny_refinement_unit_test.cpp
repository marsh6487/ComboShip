#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>
using namespace BlackMarketGranny;
int main() {
  // Hand-authored split-triangle control, not donor geometry. It proves the
  // production filter preserves a neighboring face and rejects mixed edges;
  // only the external-donor runner proves the complete compatible room.
  const auto path = Exact(std::string(kOpaque) + "_tri_0");
  const auto vertexPath = Exact(std::string(kOpaque) + "_vtx_0");
  auto source = std::make_shared<std::array<Vtx, 6>>();
  (*source)[0].v.ob[0] = 49;
  (*source)[0].v.ob[1] = 37;
  (*source)[0].v.ob[2] = 24;
  (*source)[1].v.ob[0] = 97;
  (*source)[1].v.ob[1] = 37;
  (*source)[1].v.ob[2] = 72;
  (*source)[2].v.ob[0] = 73;
  (*source)[2].v.ob[1] = 37;
  (*source)[2].v.ob[2] = 48;
  for (int i = 3; i < 6; ++i)
    (*source)[i].v.ob[0] = -i;
  const auto before = *source;
  Vertices vertices{source->data(), source->size(), source};
  std::array<int, 32> cache;
  cache.fill(-1);
  RearPots filter;
  filter.Reset(true);
  filter.VerticesFor(path, vertexPath, vertices, 6, 0, 0, cache);
  Gfx both = {gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0)};
  const auto result = filter.Filter(path, both, cache);
  Gfx retained = {gsSP1Triangle(3, 4, 5, 0)};
  assert(result.words.w0 == retained.words.w0 &&
         result.words.w1 == retained.words.w1);
  Gfx removed = {gsSP1Triangle(0, 1, 2, 0)};
  assert((filter.Filter(path, removed, cache).words.w0 >> 24) == G_SPNOOP);
  Gfx crossing = {gsSP1Triangle(0, 1, 3, 0)};
  bool rejected = false;
  try {
    filter.Filter(path, crossing, cache);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  assert(rejected);
  rejected = false;
  try {
    filter.Validate();
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  assert(rejected &&
         "incomplete geometry must never masquerade as a compatible donor");
  filter.Reset();
  assert(filter.Filter(path, both, cache).words.w0 == both.words.w0);
  assert(!std::memcmp(source->data(), before.data(), sizeof(before)));
  Vapor invalid;
  assert(!invalid.Load({}, {}, {}) && !invalid.Root());
  std::puts("PASS Granny production filter: split-face preservation, exact "
            "no-op removal, mixed-component rejection, incomplete-donor "
            "rejection, disabled control and source immutability");
}
