#pragma once

#include "../../soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"

namespace ForestGiParticles {
constexpr int kSariaMotes = 10;
constexpr int kPortalMotes = 16;
constexpr int kMysteryMotes = 16;
constexpr size_t kHexVertices = 48 + 18 + 24; // GI glow, filled hex and tiny GI star glint.
static_assert(kMysteryMotes * kHexVertices <= NeiGi::Mesh{}.vertices.size());
static_assert(kPortalMotes * kHexVertices <= NeiGi::Mesh{}.vertices.size());

// Retain the green hex identity at particle scale. A filled, bright center
// replaces the old large hollow rings; the soft skirt is the existing GI glow.
inline void Mote(NeiGi::Mesh& mesh, NeiGi::Point center, float radius, float phase, float spin,
                 const NeiGi::Basis& camera) {
    constexpr uint32_t hue = 0x44FF74, core = 0xB0FFB0;
    const size_t first = mesh.count;
    NeiGi::Glow(mesh, center, radius * 1.9f, hue, 110, camera);
    for (int side = 0; side < 6; ++side) {
        const float a = spin + side * NeiGi::Tau / 6, b = spin + (side + 1) * NeiGi::Tau / 6;
        mesh.Tri({ center, core, 230 },
                 { center + NeiGi::Plane(camera, std::cos(a) * radius, std::sin(a) * radius), hue, 90 },
                 { center + NeiGi::Plane(camera, std::cos(b) * radius, std::sin(b) * radius), hue, 90 });
    }
    // The existing GI star remains inside the hex, adding its bright glint
    // without replacing the established six-sided silhouette or green core.
    NeiGi::Star(mesh, center, radius * .45f, spin * 1.5f, hue, camera, core);
    const float fade = std::sin(phase * NeiGi::Tau * .5f);
    for (size_t i = first; i < mesh.count; ++i)
        mesh.vertices[i].alpha = uint8_t(mesh.vertices[i].alpha * fade * fade);
}

inline NeiGi::Mesh Saria(uint32_t frame, bool seated, const NeiGi::Basis& camera = {}) {
    NeiGi::Mesh mesh;
    const float time = NeiGi::Time(frame);
    for (int i = 0; i < kSariaMotes; ++i) {
        const float phase = ((frame % 144u + i * 37u) % 144u) / 144.f;
        const float seed = std::fmod(i * .61803398875f, 1.f);
        const float angle = i * 2.39996323f;
        const float radius = 15.f + seed * 8.f;
        const NeiGi::Point center{ std::cos(angle) * radius + std::sin(time + i) * 1.2f,
                                   5.f + phase * (seated ? 22.f : 44.f),
                                   std::sin(angle) * radius + std::cos(time + i) * 1.2f };
        Mote(mesh, center, .65f + seed * .5f, phase, time / 6.f + i, camera);
    }
    return mesh;
}

inline NeiGi::Point PortalRim(float position, bool packed) {
    const float radius = packed ? 68.f : 44.f, center = packed ? 66.f : 65.f;
    auto point = [&](int index) {
        if (index == 0)
            return NeiGi::Point{ -radius, 0.f, 0.f };
        if (index == 26)
            return NeiGi::Point{ radius, 0.f, 0.f };
        const float angle = NeiGi::Tau * .5f * (1.f - (index - 1) / 24.f);
        return NeiGi::Point{ radius * std::cos(angle), center + radius * std::sin(angle), 0.f };
    };
    const int segment = int(position);
    return point(segment) + (point(segment + 1) - point(segment)) * (position - segment);
}

inline NeiGi::Mesh Portal(uint32_t frame, bool packed, const NeiGi::Basis& camera = {}) {
    NeiGi::Mesh mesh;
    const float time = NeiGi::Time(frame);
    for (int i = 0; i < kPortalMotes; ++i) {
        const float phase = ((frame % 160u + i * 47u) % 160u) / 160.f;
        // Each emitter stays at its doorway edge; no contour scroll or band.
        const float position = (i + .5f) * 26.f / kPortalMotes;
        auto center = PortalRim(position, packed);
        center.x += std::sin(time + i) * .6f;
        center.y += std::sin(phase * NeiGi::Tau * .5f) * 2.f;
        center.z = packed ? 10.f : 1.f;
        Mote(mesh, center, .8f + (i % 3) * .2f, phase, time / 6.f + i, camera);
    }
    return mesh;
}

inline NeiGi::Mesh Mystery(uint32_t frame, const NeiGi::Basis& camera = {}) {
    NeiGi::Mesh mesh;
    // The accepted outer wood edge and its curved depth remain the emitter
    // anchors. No model, doorway, scene resource or collision is changed.
    constexpr float rim[][3] = { { -50.205f, 1, 57.276f },
                                 { -61.518f, 15, 57.276f },
                                 { -66.468f, 48, 45.255f },
                                 { -60.811f, 80, 32.527f },
                                 { -46.669f, 102, 16.971f },
                                 { -28.284f, 120, 4.243f },
                                 { 0, 128, 0 },
                                 { 29.698f, 120, 4.243f },
                                 { 50.912f, 104, 16.971f },
                                 { 60.811f, 80, 32.527f },
                                 { 66.468f, 48, 45.255f },
                                 { 61.518f, 15, 57.276f },
                                 { 50.205f, 1, 57.276f } };
    const float time = NeiGi::Time(frame);
    for (int i = 0; i < kMysteryMotes; ++i) {
        const float phase = ((frame % 192u + i * 37u) % 192u) / 192.f;
        const float position = (i + .5f) * 12.f / kMysteryMotes;
        const int segment = int(position);
        const float amount = position - segment;
        auto interpolate = [&](int axis) {
            return rim[segment][axis] + (rim[segment + 1][axis] - rim[segment][axis]) * amount;
        };
        const NeiGi::Point center{ interpolate(0) * 1.05f,
                                   interpolate(1) + 2.f + std::sin(phase * NeiGi::Tau * .5f) * 2.f,
                                   -interpolate(2) + 8.f + std::sin(time + i * 1.71f) * 2.f };
        Mote(mesh, center, .65f + (i % 3) * .2f, phase, time / 6.f + i, camera);
    }
    return mesh;
}
} // namespace ForestGiParticles
