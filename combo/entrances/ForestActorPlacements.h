#pragma once
#include <cmath>

namespace ForestActorPlacements {
// Clear of the stock tree at the portal's side, mouth and forward arrival.
inline constexpr float kSariaLateral = -180.0f;
struct Offset {
    float x, z;
};
inline Offset SariaOffset(int portalYaw) {
    const float angle = portalYaw * (6.28318530718f / 65536.0f);
    return { kSariaLateral * std::cos(angle), -kSariaLateral * std::sin(angle) };
}
} // namespace ForestActorPlacements
