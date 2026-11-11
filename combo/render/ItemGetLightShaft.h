#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace ItemGetLightShaft {
inline constexpr const char* kCVar = "gEnhancements.Graphics.ItemGetLightShaft";
inline constexpr size_t kRows = 6, kColumns = 5;
inline constexpr int kTextureWidth = 32, kTextureHeight = 64;

struct Vec3 {
    float x = 0, y = 0, z = 0;
};
struct Presentation {
    bool active = false;
    Vec3 origin;
    float playerHeight = 44, itemHeight = 58;
};

// Values only: receipt actors can be destroyed as soon as their textbox closes.
class Marker {
    Presentation pending;

  public:
    void Begin() {
        pending = {};
    }
    void Mark(const Presentation& presentation) {
        if (presentation.active && (!pending.active || presentation.itemHeight > pending.itemHeight))
            pending = presentation;
    }
    Presentation Consume() {
        const auto result = pending;
        Begin();
        return result;
    }
};

struct Input {
    bool enabled = false, paused = false, transitioning = false;
    uintptr_t playIdentity = 0;
    int scene = -1, room = -1;
    uint32_t frame = 0;
    float seconds = 0.05f;
    Presentation presentation;
};
struct Frame {
    bool visible = false;
    Vec3 origin;
    float height = 0, bottomWidth = 0, topWidth = 0, opacity = 0;
    uint32_t scroll = 0, generation = 0;
    int32_t scrollStep = 0;
};

inline bool Finite(float value) {
    // Native targets enable fast-math; std::isfinite can be optimized away there.
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}
inline bool Valid(const Presentation& value) {
    return value.active && Finite(value.origin.x) && Finite(value.origin.y) && Finite(value.origin.z) &&
           Finite(value.playerHeight) && value.playerHeight > 0 && value.playerHeight <= 2048 &&
           Finite(value.itemHeight) && std::abs(value.itemHeight) <= 4096;
}
inline float Smooth(float value) {
    value = std::clamp(value, 0.f, 1.f);
    return value * value * (3.f - 2.f * value);
}

class State {
    bool initialized = false;
    uintptr_t play = 0;
    int scene = -1, room = -1;
    uint32_t lastFrame = 0, generation = 0;
    float elapsed = 0;

  public:
    void Reset() {
        if (initialized)
            ++generation;
        initialized = false;
        elapsed = 0;
    }
    Frame Step(const Input& input) {
        if (!input.enabled || input.transitioning || !input.playIdentity) {
            Reset();
            return {};
        }
        if (input.paused)
            return {};
        if (!Valid(input.presentation) || !Finite(input.seconds)) {
            Reset();
            return {};
        }
        if (initialized &&
            (play != input.playIdentity || scene != input.scene || room != input.room || input.frame - lastFrame > 1u))
            Reset();
        const float seconds = std::clamp(input.seconds, 0.f, 0.1f);
        if (!initialized || lastFrame != input.frame)
            elapsed += seconds;
        initialized = true;
        play = input.playIdentity;
        scene = input.scene;
        room = input.room;
        lastFrame = input.frame;

        Frame result;
        result.visible = true;
        result.origin = input.presentation.origin;
        const float playerHeight = input.presentation.playerHeight;
        result.height = std::clamp(std::max(playerHeight * 3.8f, input.presentation.itemHeight + playerHeight * 1.6f),
                                   160.f, 8192.f);
        result.bottomWidth = std::clamp(playerHeight * 0.9f, 24.f, 1844.f);
        result.topWidth = result.bottomWidth * 0.62f;
        result.opacity = 0.32f * Smooth(elapsed / 0.2f);
        // Quarter-texel offsets: three texels per second, seamless 64-texel repeat.
        const float scroll = std::fmod(elapsed * 12.f, 256.f);
        result.scroll = static_cast<uint32_t>(scroll);
        result.scrollStep = static_cast<int32_t>(scroll + seconds * 12.f) - static_cast<int32_t>(result.scroll);
        result.generation = generation;
        return result;
    }
};

struct Vertex {
    Vec3 position; // Local to the world-position origin; never inherits the GI model matrix.
    int16_t s = 0, t = 0;
    float alpha = 0;
};
struct Mesh {
    std::array<Vertex, kRows * kColumns> vertices;
};

inline Mesh BuildMesh(const Frame& frame, Vec3 eye) {
    Mesh mesh{};
    float rightX = eye.z - frame.origin.z, rightZ = frame.origin.x - eye.x;
    const float length = std::sqrt(rightX * rightX + rightZ * rightZ);
    if (!Finite(length) || length < 0.001f) {
        rightX = 1;
        rightZ = 0;
    } else {
        rightX /= length;
        rightZ /= length;
    }
    constexpr float heights[kRows] = { 0, 0.16f, 0.36f, 0.66f, 0.9f, 1 };
    constexpr float rowAlpha[kRows] = { 0, 0.35f, 0.85f, 1, 0.65f, 0 };
    constexpr float columnAlpha[kColumns] = { 0, 0.65f, 1, 0.65f, 0 };
    for (size_t row = 0; row < kRows; ++row) {
        const float width = frame.bottomWidth + (frame.topWidth - frame.bottomWidth) * heights[row];
        for (size_t column = 0; column < kColumns; ++column) {
            const float u = float(column) / float(kColumns - 1);
            const float offset = (2.f * u - 1.f) * width;
            mesh.vertices[row * kColumns + column] = {
                { rightX * offset, frame.height * heights[row], rightZ * offset },
                static_cast<int16_t>(std::lround(u * (kTextureWidth - 1) * 32)),
                static_cast<int16_t>(std::lround((1.f - heights[row]) * (kTextureHeight - 1) * 32)),
                frame.opacity * rowAlpha[row] * columnAlpha[column]
            };
        }
    }
    return mesh;
}

inline const std::array<uint8_t, kTextureWidth * kTextureHeight>& Texture() {
    // One original, immutable 2 KiB I8 tile. Native display lists retain its
    // address, and Alt Assets cannot replace or make this resource disappear.
    alignas(8) static const std::array<uint8_t, kTextureWidth* kTextureHeight> texture = [] {
        std::array<uint8_t, kTextureWidth * kTextureHeight> result{};
        constexpr float tau = 6.28318530718f;
        for (int y = 0; y < kTextureHeight; ++y) {
            const float v = tau * y / kTextureHeight;
            for (int x = 0; x < kTextureWidth; ++x) {
                const float u = float(x) / (kTextureWidth - 1);
                const float warp = 0.12f * std::sin(v) + 0.06f * std::sin(2.f * v + 2.f * u);
                const float filaments = std::pow(0.5f + 0.5f * std::cos(tau * (4.f * u + warp)), 4.f);
                const float haze = 0.5f + 0.5f * std::sin(v + 5.f * u) * std::cos(2.f * v - 3.f * u);
                result[y * kTextureWidth + x] =
                    static_cast<uint8_t>(std::lround(255.f * (0.32f + 0.38f * filaments + 0.24f * haze)));
            }
        }
        return result;
    }();
    return texture;
}
} // namespace ItemGetLightShaft
