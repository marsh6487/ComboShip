#pragma once

#include "ForestGiParticles.h"
#include "../entrances/LostWoodsEntrance.h"
#include <array>
#include <cstdint>

namespace RegionalPassagePresentation {
enum class Host { Oot, Mm };

inline bool KnownRoute(int route) {
    return route == COMBO_PORTAL_IKANA || route == COMBO_PORTAL_GREAT_BAY || route == COMBO_PORTAL_SNOWHEAD;
}

inline uint32_t OuterColor(int route) {
    switch (route) {
        case COMBO_PORTAL_IKANA:
            return 0xE8A13A;
        case COMBO_PORTAL_GREAT_BAY:
            return 0x174A9C;
        case COMBO_PORTAL_SNOWHEAD:
            return 0xA8DFFF;
        default:
            return 0;
    }
}

inline NeiGi::Mesh Shimmer(uint32_t frame, int route, const NeiGi::Basis& camera) {
    const uint32_t outer = OuterColor(route);
    if (!outer)
        return {};
    auto mesh = ForestGiParticles::Mystery(frame, camera);
    for (size_t i = 0; i < mesh.count; ++i)
        if (mesh.vertices[i].rgb == NeiGi::ColorHex(NeiGi::Kind::Leaf))
            mesh.vertices[i].rgb = outer;
    return mesh;
}

inline NeiGi::Mesh GraveShimmer(uint32_t frame, const NeiGi::Basis& camera) {
    auto mesh = Shimmer(frame, COMBO_PORTAL_IKANA, camera);
    for (size_t first = 0; first < mesh.count; first += ForestGiParticles::kTwinkleVertices) {
        const auto origin = mesh.vertices[first].p;
        // The native grave's aperture has radius22. Scale the entrance-face
        // arrangement onto that plane; keep every camera-facing twinkle intact.
        NeiGi::Point center{ origin.x * (22.f / 60.f), 2.f, (origin.y - 66.f) * (22.f / 66.f) };
        const float radius = std::sqrt(center.x * center.x + center.z * center.z);
        if (radius > 22.f) {
            center.x *= 22.f / radius;
            center.z *= 22.f / radius;
        }
        for (size_t j = 0; j < ForestGiParticles::kTwinkleVertices; ++j)
            mesh.vertices[first + j].p = center + (mesh.vertices[first + j].p - origin);
    }
    return mesh;
}

inline const char* Texture(Host host, int route) {
    if (!KnownRoute(route))
        return nullptr;
    if (host == Host::Oot)
        return route == COMBO_PORTAL_SNOWHEAD ? "__OTR__objects/object_spot08_obj/gZorasFountainIceRampMiddleSlopeTex"
                                              : "__OTR__objects/gameplay_keep/gRockFragment1Tex";
    return route == COMBO_PORTAL_SNOWHEAD ? "__OTR__objects/object_icefloe/gIcefloePlatformTex"
                                          : "__OTR__objects/gameplay_keep/gBoulderFragmentsTex";
}

constexpr size_t kArchSegments = 10;
constexpr size_t kShellQuads = 2 + kArchSegments * 3;
// Includes setup/restore, the texture load, a matrix and native debug packets.
constexpr size_t kShellOpaCommands = kShellQuads * 2 + 32;
// Native shell/setup OPEN_DISPS pairs write XLU debug nodes transiently, even
// when their CLOSE_DISPS later rewinds the unused XLU head (observed peak2).
constexpr size_t kShellXluCommands = 8;
struct ShellVertex {
    NeiGi::Point p;
    uint8_t shade;
};

inline std::array<ShellVertex, kShellQuads * 4> BuildShell() {
    std::array<ShellVertex, kShellQuads * 4> vertices{};
    size_t index = 0;
    const auto quad = [&](NeiGi::Point a, NeiGi::Point b, NeiGi::Point c, NeiGi::Point d, uint8_t shade) {
        for (auto point : { a, b, c, d })
            vertices[index++] = { point, shade };
    };
    quad({ -68, 0, 0 }, { -60, 0, 0 }, { -60, 66, 0 }, { -68, 65, 0 }, 210);
    quad({ 60, 0, 0 }, { 68, 0, 0 }, { 68, 65, 0 }, { 60, 66, 0 }, 210);
    for (size_t i = 0; i < kArchSegments; ++i) {
        const float a = i * NeiGi::Tau * .5f / kArchSegments;
        const float b = (i + 1) * NeiGi::Tau * .5f / kArchSegments;
        const float ax = std::cos(a), ay = std::sin(a), bx = std::cos(b), by = std::sin(b);
        quad({ ax * 60, 66 + ay * 60, 0 }, { ax * 68, 65 + ay * 68, 0 }, { bx * 68, 65 + by * 68, 0 },
             { bx * 60, 66 + by * 60, 0 }, 210);
        quad({ ax * 60, 66 + ay * 60, 0 }, { bx * 60, 66 + by * 60, 0 }, { bx * 60, 66 + by * 60, -45 },
             { ax * 60, 66 + ay * 60, -45 }, 65);
        // Matching trapezoids close only the curved aperture, never the rock
        // outside its arch. The native wall and its collision remain intact.
        quad({ ax * 60, 0, -1 }, { bx * 60, 0, -1 }, { bx * 60, 66 + by * 60, -1 }, { ax * 60, 66 + ay * 60, -1 }, 0);
    }
    return vertices;
}

template <class Vtx> inline std::array<Vtx, kShellQuads * 4> NativeShell() {
    std::array<Vtx, kShellQuads * 4> result{};
    const auto shell = BuildShell();
    for (size_t i = 0; i < result.size(); ++i) {
        auto& native = result[i].v;
        const auto& source = shell[i];
        native.ob[0] = static_cast<int16_t>(std::round(source.p.x));
        native.ob[1] = static_cast<int16_t>(std::round(source.p.y));
        native.ob[2] = static_cast<int16_t>(source.p.z);
        native.tc[0] = static_cast<int16_t>(source.p.x * 16 + source.p.z * 32);
        native.tc[1] = static_cast<int16_t>(source.p.y * -16);
        native.cn[0] = native.cn[1] = native.cn[2] = source.shade;
        native.cn[3] = 255;
    }
    return result;
}

constexpr uint16_t kNativeWave = 0x28F2;
constexpr uint16_t kContinuousWave = kNativeWave - 0x800;
inline bool WavesInRange(int route, float distanceSquared, float height) {
    return route == COMBO_PORTAL_GREAT_BAY && std::isfinite(distanceSquared) && distanceSquared >= 0 &&
           distanceSquared <= 650.f * 650.f && std::isfinite(height) && std::fabs(height) <= 220.f;
}
} // namespace RegionalPassagePresentation
