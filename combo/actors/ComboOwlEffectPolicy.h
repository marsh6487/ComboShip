#pragma once
#include "../../soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "ComboOwlPageGlow.h"

namespace ComboOwlEffects {
// 120 native gameplay ticks = six seconds. Atlas samples represent every
// second tick; no mutable animation state survives scene or game transitions.
constexpr uint32_t kCycle = 120;
constexpr size_t kFrames = 60;
constexpr char kMarker[] = "__OTR__objects/owl_effects_poc4/Enabled";

inline size_t TextureFrame(uint32_t frame) {
    return (frame % kCycle) / 2;
}

// /100 keeps the shared renderer's 16x fixed-point packing in range. The
// caller scales its matrix by 100, preserving the accepted raw actor pose.
inline NeiGi::Mesh Flame(bool right) {
    NeiGi::Mesh mesh;
    const float x = right ? 5.15f : -5.15f;
    const NeiGi::EffectVertex a{ { x - 3.45f, 40.5f, 12.5f }, 0xFFFFFF, 255, 0, 1 };
    const NeiGi::EffectVertex b{ { x + 3.45f, 40.5f, 12.5f }, 0xFFFFFF, 255, 1, 1 };
    const NeiGi::EffectVertex c{ { x - 3.45f, 51.5f, 13.15f }, 0xFFFFFF, 255, 0, 0 };
    const NeiGi::EffectVertex d{ { x + 3.45f, 51.5f, 13.15f }, 0xFFFFFF, 255, 1, 0 };
    mesh.Tri(a, b, c);
    mesh.Tri(b, d, c);
    return mesh;
}

inline NeiGi::Mesh Pages(uint32_t frame) {
    NeiGi::Mesh mesh;
    static_assert(ComboOwlPageGlow::kVertexCount <= mesh.vertices.size());
    const float time = (frame % kCycle) * (NeiGi::Tau / kCycle);
    for (const auto& v : ComboOwlPageGlow::kVertices) {
        const float strength = .88f + .24f * (.5f + .5f * std::sin(NeiGi::Tau * v.y / 2400.f - time));
        uint32_t rgb = 0;
        for (int shift : { 16, 8, 0 }) {
            const float channel = float((v.rgb >> shift) & 255) * strength;
            rgb |= uint32_t(channel > 255.f ? 255.f : channel) << shift;
        }
        mesh.vertices[mesh.count++] = { { v.x / 100.f, v.y / 100.f, v.z / 100.f }, rgb, v.alpha };
    }
    return mesh;
}
} // namespace ComboOwlEffects
