#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace WoodsMysteryHexEffect {
constexpr int kHexes = 48;
constexpr int kVertexCount = kHexes * 12;
struct Vertex {
    float x, y, z;
    uint8_t r, g, b, a;
};

// Stateless geometry follows gameplay time, so pausing never advances the effect.
// Each hex is a hollow ring: six outer and six inner vertices, with no filled center.
inline void Build(uint32_t frame, std::array<Vertex, kVertexCount>& out) {
    constexpr float tau = 6.28318530718f;
    for (int h = 0; h < kHexes; ++h) {
        const float phase = ((frame % 192u + h * 37u) % 192u) / 192.0f;
        const float seed = std::fmod(h * 0.61803398875f, 1.0f);
        const float angle = h * 2.39996323f + (frame % 4096u) * tau / 4096.0f;
        const float x = std::cos(angle) * (65.0f + seed * 28.0f);
        const float y = 8.0f + phase * 165.0f;
        const float z = 8.0f + std::sin(h * 1.71f) * 18.0f;
        const float radius = h % 8 == 0 ? 8.5f : 2.5f + seed * 4.0f;
        const float fade = std::sin(phase * 3.14159265359f);
        const auto alpha = static_cast<uint8_t>(fade * 240.0f);
        for (int ring = 0; ring < 2; ++ring) {
            for (int side = 0; side < 6; ++side) {
                const float a = side * tau / 6 + h * 0.3f + (frame % 768u) * tau / 768.0f;
                const float size = radius * (ring ? 0.62f : 1.0f);
                out[h * 12 + ring * 6 + side] = {
                    x + std::cos(a) * size, y + std::sin(a) * size, z, 68, 255, 116, alpha
                };
            }
        }
    }
}
} // namespace WoodsMysteryHexEffect
