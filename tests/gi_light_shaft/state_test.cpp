#include "combo/render/ItemGetLightShaft.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace ItemGetLightShaft;

static Input Receipt() {
  Input input;
  input.enabled = true;
  input.playIdentity = 1;
  input.scene = 4;
  input.room = 2;
  input.frame = 100;
  input.seconds = 0.05f;
  input.presentation = {true, {100, 20, 300}, 44, 58};
  return input;
}

int main() {
  State state;
  auto input = Receipt();
  input.enabled = false;
  assert(!state.Step(input)
              .visible); // An opted-out receipt cannot allocate a draw.
  input.enabled = true;
  auto first = state.Step(input);
  assert(first.visible && first.opacity > 0 && first.opacity < 0.4f);
  assert(first.scrollStep ==
         1); // Next native tick crosses a quarter-texel boundary.
  auto repeated = state.Step(input);
  assert(repeated.opacity == first.opacity && repeated.scroll == first.scroll);
  for (unsigned i = 0; i < 20; ++i) {
    ++input.frame;
    repeated = state.Step(input);
  }
  assert(repeated.opacity > first.opacity && repeated.scroll != first.scroll);
  assert(repeated.height > input.presentation.itemHeight);
  const auto phase = repeated.scroll;
  input.paused = true;
  assert(!state.Step(input).visible);
  input.paused = false;
  assert(state.Step(input).scroll == phase);
  input.transitioning = true;
  assert(!state.Step(input).visible);
  input.transitioning = false;
  assert(state.Step(input).opacity == first.opacity);
  input.presentation.active = false;
  assert(!state.Step(input).visible);
  input.presentation.active = true;
  assert(state.Step(input).opacity == first.opacity);
  ++input.scene;
  assert(state.Step(input).scroll == first.scroll);
  input.frame = 2;
  assert(state.Step(input).opacity == first.opacity);
  input.frame = 1000;
  assert(state.Step(input).opacity == first.opacity);

  Marker marker;
  marker.Begin();
  marker.Mark(input.presentation);
  marker.Mark(input.presentation);
  assert(marker.Consume().active);
  assert(!marker.Consume()
              .active); // Duplicate receipt paths still yield one beam.
  marker.Mark(input.presentation);
  marker.Begin();
  assert(!marker.Consume()
              .active); // An interrupted world cannot retain an old mark.

  auto frame = state.Step(input);
  const auto mesh = BuildMesh(frame, {100, 70, 500});
  const auto turned = BuildMesh(frame, {300, 70, 300});
  assert(mesh.vertices.size() == turned.vertices.size());
  bool interiorAlpha = false;
  for (size_t i = 0; i < mesh.vertices.size(); ++i) {
    const auto &a = mesh.vertices[i];
    const auto &b = turned.vertices[i];
    assert(std::isfinite(a.position.x) && std::isfinite(a.position.y) &&
           std::isfinite(a.position.z));
    assert(a.position.y == b.position.y); // Orbiting cannot tilt the long axis.
    assert(a.alpha >= 0 && a.alpha <= 0.4f);
    interiorAlpha |= a.alpha > 0;
  }
  assert(interiorAlpha);
  for (size_t col = 0; col < kColumns; ++col) {
    assert(mesh.vertices[col].alpha == 0);
    assert(mesh.vertices[(kRows - 1) * kColumns + col].alpha == 0);
  }
  for (size_t row = 0; row < kRows; ++row) {
    assert(mesh.vertices[row * kColumns].alpha == 0);
    assert(mesh.vertices[row * kColumns + kColumns - 1].alpha == 0);
  }
  for (const auto &vertex : BuildMesh(frame, frame.origin).vertices) {
    assert(std::isfinite(vertex.position.x) &&
           std::isfinite(vertex.position.z));
  }
  const auto *texture = Texture().data();
  assert(texture == Texture().data());
  bool varies = false;
  for (size_t i = 1; i < Texture().size(); ++i)
    varies |= texture[i] != texture[0];
  assert(varies);

  for (float height : {36.f, 44.f, 68.f, 80.f, 124.f, 680.f}) {
    state.Reset();
    input.presentation.playerHeight = height;
    input.presentation.itemHeight = height + 20;
    const auto scaled = state.Step(input);
    assert(scaled.visible && scaled.height > height + 20);
    assert(scaled.bottomWidth > 0 && scaled.topWidth > 0);
  }
  input.presentation.origin.x = std::numeric_limits<float>::quiet_NaN();
  assert(!state.Step(input).visible);
  input = Receipt();
  input.presentation.playerHeight = std::numeric_limits<float>::infinity();
  assert(!state.Step(input).visible);
  std::puts(
      "PASS GI shaft opt-in, timing, lifecycle, marker, mesh and form sizing");
}
