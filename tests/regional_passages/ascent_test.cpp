#include "entrances/FountainSnowheadBridge.h"
#include "entrances/RegionalPassages.h"
#include <cassert>
#include <cmath>
int main() {
  using namespace ComboFountainSnowhead;
  assert(kStations[0].x > 900 && kStations[0].z > 700 && kStations[0].y == 0);
  auto mesh = BuildMesh();
  for (size_t i = 0; i < mesh.count; ++i)
    assert(!mesh.triangles[i].cave);
  for (size_t i = 1; i < kStationCount; ++i) {
    auto a = kStations[i - 1], b = kStations[i];
    assert(std::abs(b.y - a.y) / std::hypot(b.x - a.x, b.z - a.z) < .55);
  }
  float y = 0;
  assert(ComboFountainValley::FloorAt(kMouth.x, kMouth.z, &y));
  assert(std::abs(kMouth.y - y) < .01);
  assert(ComboFountainValley::FloorAt(kReturn.x, kReturn.z, &y));
  assert(std::abs(kReturn.y - y) < .01);
  assert(kMouth.x - kReturn.x >= 90);
  ComboRegional::Pose mouth{
      {kMouth.x, kMouth.y, kMouth.z}, kYaw, ComboRegional::Forward(kYaw)};
  for (float z : {-85, 0, 85}) {
    float floor;
    assert(ComboFountainValley::FloorAt(1820, z, &floor));
    assert(ComboRegional::InSnowheadMouth(mouth, {1820, floor, z}));
    assert(!ComboRegional::InSnowheadMouth(mouth, {1820, floor + 21, z}));
  }
  ComboRegional::Pose returned;
  assert(ComboRegional::SnowheadReturn(&returned));
  assert(!ComboRegional::InSnowheadMouth(mouth, returned.pos));
  // Final packed contact spans the whole passage and lands on shared ground.
  for (size_t i = mesh.count - 6; i < mesh.count - 4; ++i)
    for (auto p :
         {mesh.triangles[i].a, mesh.triangles[i].b, mesh.triangles[i].c})
      if (p.x >= 1599) {
        assert(ComboFountainValley::FloorAt(p.x, p.z, &y));
        assert(std::abs(p.y - y - 1) < .01);
      }
}
