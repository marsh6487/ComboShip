#pragma once
#include "NeiGiEffectPolicy.h"
#include <algorithm>

// Receipt-only energy. The native/selected arrow core keeps its own resource;
// these meshes never bind a projectile actor's material or dynamic segment.
namespace NeiArrowGi {
using NeiGi::Basis;
using NeiGi::Mesh;
using NeiGi::Point;
constexpr float HeadY = 9.f;
struct Layers {
    Mesh veil, energy, shimmer;
};
inline bool Valid(int profile) {
    return profile >= 1 && profile <= 3;
}
inline NeiGi::Kind Kind(int profile) {
    return profile == 1 ? NeiGi::Kind::Fire : profile == 2 ? NeiGi::Kind::Ice : NeiGi::Kind::Light;
}
inline const char* Texture(int profile) {
    return profile == 1   ? "__OTR__objects/nei_elemental_arrow_gi/fire"
           : profile == 2 ? "__OTR__objects/nei_elemental_arrow_gi/ice"
                          : "__OTR__objects/nei_elemental_arrow_gi/light";
}
inline void Sheet(Mesh& mesh, float t, float yaw, float low, float height, float width, uint32_t color,
                  uint8_t opacity) {
    // Ten curved strips keep the textured field wrapped around the shaft.
    // Separate planes move at different phases without rewriting cached texels.
    const Point across{ std::cos(yaw), 0, std::sin(yaw) };
    const Point depth{ -std::sin(yaw), 0, std::cos(yaw) };
    auto vertex = [&](int row, bool right) {
        const float s = row / 10.f;
        const float bend = std::sin(s * 4.4f - t * 2 + yaw) * (1.2f + s * 1.4f);
        const float half = width * (.8f + .2f * std::sin(s * 3.1f + t + yaw));
        const Point p = across * ((right ? half : -half) + bend) +
                        depth * (2.2f + 1.4f * std::sin(s * 4.1f + t + yaw)) + Point{ 0, low + height * s, 0 };
        const float fade = std::min(1.f, std::min(s * 6, (1 - s) * 5));
        return NeiGi::EffectVertex{ p, color, uint8_t(opacity * fade), right ? 1.f : 0.f, 1.f - s };
    };
    for (int row = 0; row < 10; ++row) {
        const auto a = vertex(row, false), b = vertex(row, true);
        const auto c = vertex(row + 1, true), d = vertex(row + 1, false);
        mesh.Tri(a, b, c);
        mesh.Tri(a, c, d);
    }
}
inline void Spiral(Mesh& mesh, float t, float phase, float radius, float low, float height, float width, uint32_t edge,
                   uint32_t core, const Basis& camera, uint8_t alpha) {
    auto point = [&](int j) {
        const float s = j / 12.f, a = phase + s * NeiGi::Tau * 1.1f - t;
        const float r = radius * (.75f + .25f * std::sin(s * 3.14f));
        return Point{ std::cos(a) * r, low + height * s, std::sin(a) * r };
    };
    for (int j = 0; j < 12; ++j) {
        const float taper = std::sin((j + .5f) * 3.14159265f / 12);
        NeiGi::Band(mesh, point(j), point(j + 1), width * taper, edge, core, camera, uint8_t(alpha * taper));
    }
}
inline Layers Sample(int profile, uint32_t frame, const Basis& camera = {}) {
    Layers layers;
    if (!Valid(profile))
        return layers;
    const float t = (frame % 360u) * (NeiGi::Tau / 360.f);
    const auto head = Point{ 0, HeadY, 0 };
    if (profile == 1) {
        for (int sheet = 0; sheet < 3; ++sheet)
            Sheet(layers.veil, t + sheet * 1.3f, sheet * 1.0472f + .14f * std::sin(t), -6.f,
                  35.f + 2.f * std::sin(t * 2 + sheet), 7.2f, 0xFFB12B, 142);
        for (int ribbon = 0; ribbon < 2; ++ribbon)
            Spiral(layers.energy, t * 2, ribbon * 3.14159265f, 5.5f, -22, 42, 2.7f, 0xF35716, 0xFFEBA1, camera, 185);
        NeiGi::Glow(layers.energy, head, 7, 0xFF8C20, 75, camera);
        for (int i = 0; i < 5; ++i) {
            const float f = std::fmod((frame % 360u) / 90.f + i / 5.f, 1.f);
            const float fade = std::sin(f * NeiGi::Tau * .5f);
            const auto p = Point{ std::sin(t * 3 + i * 1.8f) * 11, 7 + f * 25, std::cos(t * 3 + i * 1.8f) * 9 };
            NeiGi::Band(layers.energy, p, p + Point{ .5f, 1.6f, 0 }, .55f * fade, 0xFF9F32, 0xFFF1B9, camera,
                        uint8_t(210 * fade));
        }
    } else if (profile == 2) {
        for (int sheet = 0; sheet < 2; ++sheet)
            Sheet(layers.veil, t + sheet, sheet * 1.5708f - .4f * std::sin(t), -23, 48, 5.3f, 0x76D9FF, 125);
        Spiral(layers.energy, -t, .4f, 5, -22, 39, 1.5f, 0x4BAFFF, 0xE6FFFF, camera, 100);
        NeiGi::Glow(layers.energy, head, 11, 0x70BCF5, 55, camera);
        for (int i = 0; i < 8; ++i) {
            const float f = std::fmod((frame % 360u) / 180.f + i / 8.f, 1.f);
            const float fade = std::sin(f * NeiGi::Tau * .5f);
            const float a = i * 2.39996f + t;
            const float roll = t * (i % 2 ? -2 : 2) + i;
            const auto center = Point{ std::cos(a) * (8 + f * 4), 21 - 44 * f, std::sin(a) * (8 + f * 4) };
            NeiGi::IceCrystal(layers.energy, center, { std::sin(roll), 1, std::cos(roll) }, 5.2f, 1.6f, roll,
                              uint8_t(220 * fade * fade), camera);
        }
    } else {
        for (int sheet = 0; sheet < 2; ++sheet)
            Sheet(layers.veil, t + sheet, sheet * 1.5708f + t, -12, 44, 8.5f, 0xFFF2A1, 112);
        NeiGi::Glow(layers.energy, head, 10, 0xFFD34D, 110, camera);
        NeiGi::Glow(layers.energy, head, 4, 0xFFFFE3, 190, camera);
        for (int ring = 0; ring < 2; ++ring) {
            auto p = [&](int j) {
                const float a = NeiGi::Tau * j / 16 + t * (ring ? -1 : 1);
                return head + Point{ 12 * std::cos(a), (ring ? 7 : 3) * std::sin(a), (ring ? 4 : 10) * std::sin(a) };
            };
            for (int j = 0; j < 14; ++j) {
                const float arc = std::sin((j + .5f) * 3.14159265f / 14);
                NeiGi::Band(layers.energy, p(j), p(j + 1), .9f, 0xECAF32, 0xFFFFD4, camera, uint8_t(170 * arc));
            }
        }
        for (int ray = 0; ray < 8; ++ray) {
            const float a = NeiGi::Tau * ray / 8 + t;
            const float pulse = .75f + .25f * std::sin(t * 2 + ray);
            const Point direction = NeiGi::Plane(camera, std::cos(a), std::sin(a));
            NeiGi::Band(layers.energy, head + direction * 6, head + direction * (18 * pulse), .9f * pulse, 0xFFCF59,
                        0xFFFBE1, camera, 90);
        }
    }
    layers.shimmer = NeiGi::SampleShimmer(frame, true, camera, Kind(profile));
    // The shared shimmer retains its visual recipe, with a compact arrow envelope.
    for (size_t i = 0; i < layers.shimmer.count; ++i) {
        auto& p = layers.shimmer.vertices[i].p;
        p.x *= .58f;
        p.z *= .58f;
        p.y *= .68f;
    }
    return layers;
}
} // namespace NeiArrowGi
