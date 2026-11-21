#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace ComboZfgWallet {
struct Point {
    float x, y, z;
};

struct Rupee {
    Point center;
    float scale, yaw, roll;
    uint8_t color, alpha;
};

inline constexpr Point kClassicMouth{ 0.f, 26.f, 0.f };
inline constexpr Point kTwilightMouth{ 4.f, 27.f, -1.f };
inline constexpr char kTwilightModel[] = "__OTR__objects/combo_zfg_wallet/tp/Draw";

inline float Fade(float fraction) {
    // Fade throughout the upper part of the rise, reaching zero before the loop resets.
    const float edge = std::min(1.f, std::min(fraction * 6.f, (1.f - fraction) / .70f));
    return edge * edge * (3.f - 2.f * edge);
}

inline std::array<Rupee, 3> SampleRupees(uint32_t frame, Point mouth) {
    constexpr float drift[] = { -14.f, 10.f, 3.f };
    constexpr float spread[] = { 6.f, 4.f, -10.f };
    constexpr float yawStart[] = { -.32f, .18f, -.08f };
    constexpr unsigned cycle = 72;
    std::array<Rupee, 3> rupees;
    for (uint8_t i = 0; i < rupees.size(); ++i) {
        const float phase = float((frame % cycle + i * 24) % cycle) / cycle;
        rupees[i] = {
            { mouth.x + drift[i] * phase, mouth.y + 33.f * phase, mouth.z + spread[i] * phase },
            (5.3f + 1.5f * phase) / 37.f, // Stock and Rupee Redux share the same GI scale.
            yawStart[i] + .75f * phase,
            (i == 1 ? -.24f : .22f) * phase,
            i,
            static_cast<uint8_t>(240.f * Fade(phase)),
        };
    }
    return rupees;
}
} // namespace ComboZfgWallet
