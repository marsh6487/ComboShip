#pragma once

#include "../../soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"

namespace ForestGiParticles {
constexpr int kSariaMotes = 10;
constexpr int kPortalMotes = 20;
constexpr int kMysteryMotes = 20;
constexpr size_t kTwinkleVertices = 72; // The existing NEI glow and four-point star.
static_assert(kPortalMotes * kTwinkleVertices <= NeiGi::Mesh{}.vertices.size());
static_assert(kMysteryMotes * kTwinkleVertices <= NeiGi::Mesh{}.vertices.size());

// Sample the actual optional NEI GI shimmer. Move each complete twinkle as a
// rigid unit, preserving its shape, pulse, spin, green palette and alpha.
// Five source twinkles are sampled together; no new particle primitive exists.
template <class Placement>
inline NeiGi::Mesh Emit(uint32_t frame, int motes, float scale, const NeiGi::Basis& camera, Placement place) {
    NeiGi::Mesh mesh;
    for (int group = 0; group * 5 < motes; ++group) {
        const auto sample = NeiGi::SampleShimmer(frame + uint32_t(group) * 29u, true, camera, NeiGi::Kind::Leaf);
        for (int slot = 0; slot < 5 && group * 5 + slot < motes; ++slot) {
            const size_t first = slot * kTwinkleVertices;
            const auto origin = sample.vertices[first].p;
            const auto center = place(group * 5 + slot, origin);
            if (mesh.count + kTwinkleVertices > mesh.vertices.size())
                return mesh;
            for (size_t j = 0; j < kTwinkleVertices; ++j) {
                auto vertex = sample.vertices[first + j];
                vertex.p = center + (vertex.p - origin) * scale;
                mesh.vertices[mesh.count++] = vertex;
            }
        }
    }
    return mesh;
}

inline NeiGi::Mesh Saria(uint32_t frame, bool seated, const NeiGi::Basis& camera = {}) {
    const float time = NeiGi::Time(frame);
    return Emit(frame, kSariaMotes, .4f, camera, [&](int i, NeiGi::Point) {
        const float phase = ((frame % 180u + i * 37u) % 180u) / 180.f;
        const float angle = i * 2.39996323f + time * .15f;
        const float radius = 14.f + (i % 3) * 3.f;
        return NeiGi::Point{ std::cos(angle) * radius, 6.f + phase * (seated ? 24.f : 44.f), std::sin(angle) * radius };
    });
}

inline NeiGi::Mesh Entrance(uint32_t frame, float radius, float centerY, float front, const NeiGi::Basis& camera) {
    return Emit(frame, kPortalMotes, .8f, camera, [&](int i, NeiGi::Point) {
        // A staggered five-by-four field covers the full black opening.
        // The arched ceiling clips emitter centers rather than outlining it.
        const float x = (((i * 3) % 5 + .5f) / 5.f * 2.f - 1.f) * (radius - 5.f);
        const float height = centerY + std::sqrt(radius * radius - x * x);
        const float row = (i / 5 + .5f) / 4.f;
        return NeiGi::Point{ x, 6.f + row * (height - 12.f), front };
    });
}

inline NeiGi::Mesh Portal(uint32_t frame, bool packed, const NeiGi::Basis& camera = {}) {
    return Entrance(frame, packed ? 68.f : 44.f, packed ? 66.f : 65.f, packed ? 14.f : 8.f, camera);
}

inline NeiGi::Mesh Mystery(uint32_t frame, const NeiGi::Basis& camera = {}) {
    return Entrance(frame, 60.f, 66.f, 10.f, camera);
}

inline NeiGi::Mesh Mirror(uint32_t frame, const NeiGi::Basis& camera = {}) {
    return Emit(frame, 5, .5f, camera, [](int, NeiGi::Point original) { return original * .55f; });
}
} // namespace ForestGiParticles
