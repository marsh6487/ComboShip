#pragma once
#ifdef __cplusplus
#include <algorithm>
#include <cmath>
#include <vector>

// Authored world-space geometry. No scene/actor/triangle search chooses this pose.
// spot06 collision631/632: x=-2245,z3637..3797,y=-1313..-1033.
// The cave attaches to that marked bank below the laboratory, wholly lakeward
// of the protected wall. The outer-only collar overlaps the original bank;
// the deep hollow and travel path never penetrate the laboratory mesh.
namespace ComboLakeHylia {
struct Point {
    float x, y, z;
};
struct Triangle {
    Point a, b, c;
};
struct Quad {
    Point a, b, c, d;
    unsigned char shade;
    bool collides;
};
struct Geometry {
    std::vector<Quad> solid, water;
};
inline constexpr Point kMouth{ -2185, -1305, 3717 };
inline constexpr short kYaw = 16384; // +X: the approach, return, shimmer and waves.
inline constexpr Point kBank{ -2310, -1033, 3860 };
inline constexpr Point kApron{ -1875, -1305, 3717 };
inline constexpr float kFacadeX = -1915, kBackX = -2237;
// Outer-only rear collar overlaps the measured straight/angled bank wall.
// The walkable vault, back cap and shared mouth retain their authored positions.
inline constexpr float kRockJoinX = -2270;
inline constexpr float kPlinthBaseY = -1405;
// One exact hollow profile is shared by the vault, black back cap and shimmer.
inline constexpr float kOpeningBaseY = 0, kOpeningHalfWidth = 90, kOpeningSideHeight = 150;
inline constexpr float kOpeningShoulderHalfWidth = 52, kOpeningShoulderHeight = 190, kOpeningHeight = 216;
inline constexpr float OpeningTopAt(float across) {
    const float width = across < 0 ? -across : across;
    if (width > kOpeningHalfWidth)
        return 0;
    if (width >= kOpeningShoulderHalfWidth)
        return kOpeningSideHeight + (kOpeningHalfWidth - width) * ((kOpeningShoulderHeight - kOpeningSideHeight) /
                                                                   (kOpeningHalfWidth - kOpeningShoulderHalfWidth));
    return kOpeningHeight - width * ((kOpeningHeight - kOpeningShoulderHeight) / kOpeningShoulderHalfWidth);
}
inline constexpr Point kOpeningSection[] = { { 0, kOpeningBaseY, -kOpeningHalfWidth },
                                             { 0, OpeningTopAt(-kOpeningHalfWidth), -kOpeningHalfWidth },
                                             { 0, OpeningTopAt(-kOpeningShoulderHalfWidth),
                                               -kOpeningShoulderHalfWidth },
                                             { 0, OpeningTopAt(0), 0 },
                                             { 0, OpeningTopAt(kOpeningShoulderHalfWidth), kOpeningShoulderHalfWidth },
                                             { 0, OpeningTopAt(kOpeningHalfWidth), kOpeningHalfWidth },
                                             { 0, kOpeningBaseY, kOpeningHalfWidth } };
// Local +Z faces lakeward after kYaw. Place the field against the actual black
// back cap, ten units in front, without moving the departure/return mouth.
inline constexpr float kShimmerFront = kBackX - kMouth.x + 10;
inline constexpr float kFullWaterY = -1313, kDrainedWaterY = -1993, kChannelFloorY = -1333;
inline constexpr float kStreamHalfWidth = 10;
// The native lake feeds this visual inlet only while it reaches the trough.
// Drained setups have a dry cave: native waterboxes and progression stay intact.
inline bool HasInlet(float lakeSurface) {
    return std::isfinite(lakeSurface) && lakeSurface >= kChannelFloorY + 2;
}
inline float StreamHeight(float lakeSurface) {
    return std::clamp(lakeSurface + 1, kChannelFloorY + 3, kFullWaterY + 1);
}
inline Geometry BuildGeometry() {
    Geometry mesh;
    auto quad = [&](Point a, Point b, Point c, Point d, unsigned char shade = 170, bool collision = true) {
        mesh.solid.push_back({ a, b, c, d, shade, collision });
    };
    // Collision is directional: inner surfaces face hollow free space; outer
    // surfaces face away from the solid rock. Drawing's no-cull mode is unrelated.
    auto reverseQuad = [&](Point a, Point b, Point c, Point d, unsigned char shade = 170, bool collision = true) {
        quad(a, d, c, b, shade, collision);
    };
    auto top = [&](float x0, float x1, float z0, float z1, float y, unsigned char shade = 160) {
        quad({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, shade);
    };
    // Two solid shelves, central walking floor, and a narrow recessed stream
    // on the south side. Every travel/return point uses the central dry floor.
    for (float x0 = kBackX; x0 < kFacadeX; x0 += 46) {
        const float x1 = std::min(x0 + 46, kFacadeX);
        top(x0, x1, 3627, 3747, kMouth.y);
        top(x0, x1, 3767, 3807, kMouth.y);
        top(x0, x1, 3747, 3767, kChannelFloorY, 80);
        reverseQuad({ x0, kMouth.y, 3747 }, { x1, kMouth.y, 3747 }, { x1, kChannelFloorY, 3747 },
                    { x0, kChannelFloorY, 3747 }, 90);
        reverseQuad({ x0, kChannelFloorY, 3767 }, { x1, kChannelFloorY, 3767 }, { x1, kMouth.y, 3767 },
                    { x0, kMouth.y, 3767 }, 90);
    }
    // A 322-unit hollow rock vault, progressively darker at its deep end.
    for (int ring = 0; ring < 7; ++ring) {
        float x0 = kBackX + (kFacadeX - kBackX) * ring / 7;
        float x1 = kBackX + (kFacadeX - kBackX) * (ring + 1) / 7;
        for (int side = 0; side < 6; ++side) {
            auto a = kOpeningSection[side], b = kOpeningSection[side + 1];
            reverseQuad({ x0, kMouth.y + a.y, kMouth.z + a.z }, { x0, kMouth.y + b.y, kMouth.z + b.z },
                        { x1, kMouth.y + b.y, kMouth.z + b.z }, { x1, kMouth.y + a.y, kMouth.z + a.z }, 30 + ring * 20);
        }
    }
    // Recessed cap is behind the rear trigger; the front remains truly hollow.
    reverseQuad({ kBackX, kMouth.y, kMouth.z - kOpeningHalfWidth }, { kBackX, kMouth.y, kMouth.z + kOpeningHalfWidth },
                { kBackX, kMouth.y + kOpeningHeight, kMouth.z + kOpeningHalfWidth },
                { kBackX, kMouth.y + kOpeningHeight, kMouth.z - kOpeningHalfWidth }, 0);
    // Jagged rock reveal around the facade, with thickness rather than a flat arch.
    for (int side = 0; side < 6; ++side) {
        auto a = kOpeningSection[side], b = kOpeningSection[side + 1];
        auto outer = [](Point p) {
            p.z *= 1.28f;
            p.y = p.y * 1.13f + 6;
            return p;
        };
        auto oa = outer(a), ob = outer(b);
        reverseQuad({ kFacadeX, kMouth.y + a.y, kMouth.z + a.z }, { kFacadeX, kMouth.y + b.y, kMouth.z + b.z },
                    { kFacadeX + 18, kMouth.y + ob.y, kMouth.z + ob.z },
                    { kFacadeX + 18, kMouth.y + oa.y, kMouth.z + oa.z }, 175);
        // Outer roof joins the bank. It is visible from above and from the lab.
        reverseQuad(
            { kRockJoinX, kMouth.y + oa.y, kMouth.z + oa.z }, { kFacadeX + 18, kMouth.y + oa.y, kMouth.z + oa.z },
            { kFacadeX + 18, kMouth.y + ob.y, kMouth.z + ob.z }, { kRockJoinX, kMouth.y + ob.y, kMouth.z + ob.z }, 140);
    }
    // Modest shore apron and vertical rock skirt. The native basin rises to
    // within20 units of its north lip; its embedded base never forms a ramp.
    top(kFacadeX, -1825, 3615, 3747, kMouth.y);
    top(kFacadeX, -1825, 3767, 3815, kMouth.y);
    top(kFacadeX, -1825, 3747, 3767, kChannelFloorY, 90);
    for (float z : { 3615.f, 3815.f }) {
        const Point a{ kRockJoinX, kPlinthBaseY, z }, b{ -1825, kPlinthBaseY, z }, c{ -1825, kMouth.y, z },
            d{ kRockJoinX, kMouth.y, z };
        if (z < kMouth.z)
            reverseQuad(a, b, c, d, 125);
        else
            quad(a, b, c, d, 125);
    }
    reverseQuad({ -1825, kMouth.y, 3615 }, { -1825, kPlinthBaseY, 3615 }, { -1825, kPlinthBaseY, 3815 },
                { -1825, kMouth.y, 3815 }, 120);
    // The native lab bank stays untouched. Access uses the existing lake/shore
    // and standard ledge behavior; no switchback, landing or raised support slab.
    // Stream quads are rebuilt at the actual level by the native provider.
    for (int i = 0; i < 8; ++i) {
        float x0 = kBackX + float(i) * (-1825 - kBackX) / 8;
        float x1 = kBackX + float(i + 1) * (-1825 - kBackX) / 8;
        mesh.water.push_back({ { x0, kFullWaterY + 1, 3747 },
                               { x0, kFullWaterY + 1, 3767 },
                               { x1, kFullWaterY + 1, 3767 },
                               { x1, kFullWaterY + 1, 3747 },
                               170,
                               false });
    }
    return mesh;
}
} // namespace ComboLakeHylia
#endif
