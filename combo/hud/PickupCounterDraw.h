#pragma once

#include "PickupCounter.h"
#include <imgui.h>
#include <cstdio>

namespace PickupCounter {
struct Palette {
    int r, g, b;
};
inline Palette Accent(Kind kind) {
    switch (kind) {
        case Kind::Rupees:
            return { 112, 236, 157 };
        case Kind::Arrows:
            return { 135, 214, 242 };
        case Kind::Bombs:
        case Kind::Bombchus:
        case Kind::PowderKeg:
            return { 151, 177, 255 };
        case Kind::Seeds:
        case Kind::Nuts:
            return { 239, 200, 117 };
        default:
            return { 182, 224, 146 };
    }
}
inline ImU32 Ink(int r, int g, int b, float opacity) {
    return IM_COL32(r, g, b, static_cast<int>(std::clamp(opacity, 0.0f, 1.0f) * 255));
}
inline ImU32 Ink(Palette color, float opacity) {
    return Ink(color.r, color.g, color.b, opacity);
}
inline void Text(ImDrawList* draw, ImFont* font, float size, ImVec2 pos, ImU32 color, const char* text, float alpha) {
    // A fine shadow keeps the floating digits readable over bright scenery.
    const ImU32 shadow = Ink(0, 0, 0, alpha * 0.8f);
    for (const ImVec2 offset : { ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1) }) {
        draw->AddText(font, size, ImVec2(pos.x + offset.x, pos.y + offset.y), shadow, text);
    }
    draw->AddText(font, size, pos, color, text);
}
inline void Spark(ImDrawList* draw, ImVec2 pos, float radius, ImU32 color) {
    draw->AddQuadFilled(ImVec2(pos.x, pos.y - radius), ImVec2(pos.x + radius * 0.28f, pos.y),
                        ImVec2(pos.x, pos.y + radius), ImVec2(pos.x - radius * 0.28f, pos.y), color);
    draw->AddQuadFilled(ImVec2(pos.x - radius * 0.65f, pos.y), ImVec2(pos.x, pos.y - radius * 0.2f),
                        ImVec2(pos.x + radius * 0.65f, pos.y), ImVec2(pos.x, pos.y + radius * 0.2f), color);
}

inline void DrawItemTexture(ImDrawList* draw, ImTextureID texture, ImVec2 center, float scale, float alpha) {
    const float radius = 15.0f * scale;
    draw->AddImage(texture, ImVec2(center.x - radius, center.y - radius), ImVec2(center.x + radius, center.y + radius),
                   ImVec2(0, 0), ImVec2(1, 1), Ink(255, 255, 255, alpha));
}

// Small faceted gem, independent of any optional texture pack. Ammo uses the
// host's actual inventory icon; these silhouettes also cover a missing icon.
inline void DrawIcon(ImDrawList* draw, Kind kind, ImVec2 center, float scale, float alpha, ImTextureID texture) {
    const auto p = [=](float x, float y) { return ImVec2(center.x + x * scale, center.y + y * scale); };
    const Palette accent = Accent(kind);
    if (texture) {
        DrawItemTexture(draw, texture, center, scale, alpha);
        return;
    }
    if (kind == Kind::Rupees) {
        const ImVec2 outside[] = { p(0, -16), p(10, -8), p(10, 7), p(0, 17), p(-10, 7), p(-10, -8) };
        draw->AddConvexPolyFilled(outside, 6, Ink(33, 97, 72, alpha));
        draw->AddQuadFilled(p(0, -12), p(6, -6), p(6, 6), p(0, 12), Ink(121, 246, 164, alpha));
        draw->AddQuadFilled(p(0, -12), p(0, 12), p(-6, 6), p(-6, -6), Ink(47, 179, 104, alpha));
        draw->AddTriangleFilled(p(-10, -8), p(0, -16), p(0, -12), Ink(213, 255, 217, alpha));
        draw->AddTriangleFilled(p(0, -12), p(0, -16), p(10, -8), Ink(159, 246, 185, alpha));
        draw->AddPolyline(outside, 6, Ink(197, 255, 209, alpha * 0.8f), ImDrawFlags_Closed, scale);
        draw->AddLine(p(-6, -6), p(0, -12), Ink(237, 255, 239, alpha), 1.3f * scale);
    } else if (kind == Kind::Arrows || kind == Kind::Sticks) {
        draw->AddLine(p(-8, 12), p(8, -12), Ink(209, 175, 111, alpha), 3 * scale);
        if (kind == Kind::Arrows) {
            draw->AddTriangleFilled(p(10, -15), p(0, -9), p(9, -5), Ink(204, 235, 247, alpha));
            draw->AddQuadFilled(p(-8, 7), p(-13, 7), p(-9, 14), p(-4, 14), Ink(accent, alpha));
        } else {
            draw->AddLine(p(8, -12), p(3, -15), Ink(159, 198, 116, alpha), 3 * scale);
        }
    } else if (kind == Kind::Bombchus) {
        // A little clockwork mouse: blue shell, yellow ears/underside, pointed
        // snout and two visible wheels. Keep its silhouette distinct from a bomb.
        draw->AddBezierCubic(p(-9, 3), p(-15, 3), p(-15, -9), p(-10, -10), Ink(228, 184, 74, alpha), 1.4f * scale);
        draw->AddCircleFilled(p(-6, 9), 3 * scale, Ink(24, 31, 49, alpha), 16);
        draw->AddCircleFilled(p(7, 9), 3 * scale, Ink(24, 31, 49, alpha), 16);
        draw->AddCircleFilled(p(-6, 9), 1.4f * scale, Ink(218, 163, 53, alpha), 12);
        draw->AddCircleFilled(p(7, 9), 1.4f * scale, Ink(218, 163, 53, alpha), 12);
        draw->AddTriangleFilled(p(-2, -2), p(-5, -11), p(2, -7), Ink(45, 77, 146, alpha));
        draw->AddTriangleFilled(p(-2, -3), p(-3, -8), p(0, -6), Ink(225, 183, 60, alpha));
        draw->AddEllipseFilled(p(-2, 2), ImVec2(10 * scale, 7.5f * scale), Ink(36, 67, 135, alpha), -0.18f, 28);
        draw->AddEllipse(p(-2, 2), ImVec2(10 * scale, 7.5f * scale), Ink(124, 165, 230, alpha), -0.18f, 28, scale);
        draw->AddQuadFilled(p(-9, 6), p(8, 5), p(11, 8), p(-6, 9), Ink(218, 165, 48, alpha));
        draw->AddCircleFilled(p(6, 1), 5.5f * scale, Ink(47, 83, 160, alpha), 20);
        draw->AddTriangleFilled(p(2, -2), p(3, -13), p(9, -5), Ink(54, 94, 169, alpha));
        draw->AddTriangleFilled(p(4, -4), p(4, -10), p(7, -5), Ink(248, 208, 65, alpha));
        draw->AddTriangleFilled(p(8, 1), p(15, 5), p(8, 7), Ink(233, 183, 71, alpha));
        draw->AddCircleFilled(p(14, 5), 1.5f * scale, Ink(50, 47, 43, alpha), 12);
        draw->AddCircleFilled(p(8, -0.5f), 1.7f * scale, Ink(22, 29, 47, alpha), 12);
        draw->AddCircleFilled(p(8.3f, -0.7f), 0.9f * scale, Ink(247, 112, 76, alpha), 10);
        draw->AddBezierQuadratic(p(-8, -1), p(-4, -5), p(0, -3), Ink(134, 200, 255, alpha * 0.85f), 1.7f * scale);
    } else if (kind == Kind::Bombs || kind == Kind::PowderKeg) {
        draw->AddCircleFilled(p(0, 3), 11 * scale, Ink(65, 85, 150, alpha), 24);
        draw->AddCircle(p(0, 3), 11 * scale, Ink(accent, alpha), 24, scale);
        draw->AddCircleFilled(p(-4, -1), 3 * scale, Ink(170, 197, 245, alpha * 0.7f), 16);
        draw->AddLine(p(2, -8), p(5, -14), Ink(225, 203, 149, alpha), 2 * scale);
        Spark(draw, p(6, -15), 3 * scale, Ink(255, 221, 145, alpha));
    } else {
        draw->AddEllipseFilled(p(0, 1), ImVec2(9 * scale, 12 * scale), Ink(165, 122, 59, alpha), -0.4f, 24);
        draw->AddEllipse(p(0, 1), ImVec2(9 * scale, 12 * scale), Ink(accent, alpha), -0.4f, 24, scale);
        draw->AddBezierQuadratic(p(-3, -7), p(6, 0), p(0, 9), Ink(255, 228, 163, alpha * 0.7f), scale);
    }
}

// Coordinates are logical pixels at 720p. The full production draw path is
// shared with the headless visual review; no separate mockup implementation.
inline constexpr float kRowWidth = 166;
inline constexpr float kRowHeight = 36;
inline constexpr float kRowStride = 43;

inline void DrawRow(ImDrawList* draw, const Row& row, ImVec2 pos, float scale, ImTextureID texture = {},
                    ImFont* font = nullptr) {
    if (!row.active || row.value.capacity <= 0 || row.Alpha() <= 0) {
        return;
    }
    if (!font) {
        font = ImGui::GetFont();
    }
    const float alpha = row.Alpha();
    const float pulse = std::exp(-row.pulse * 6.0f);
    const float enter = 1 - Smooth(row.age / 0.22f);
    pos.x += enter * 8 * scale;
    pos.y -= Smooth((row.quiet - (kLifetime - kFadeTime)) / kFadeTime) * 4 * scale;
    const auto p = [=](float x, float y) { return ImVec2(pos.x + x * scale, pos.y + y * scale); };
    const Palette accent = Accent(row.kind);
    DrawIcon(draw, row.kind, p(14, 17), scale * 0.86f * (1 + pulse * 0.06f), alpha, texture);

    char current[16], capacity[24], reward[24];
    std::snprintf(current, sizeof(current), "%d", static_cast<int>(std::round(row.displayed)));
    std::snprintf(capacity, sizeof(capacity), "/ %d", row.value.capacity);
    if (row.value.infinite) {
        std::snprintf(current, sizeof(current), "INF");
    }
    std::snprintf(reward, sizeof(reward), "+%d", row.gained);
    const float currentWidth = font->CalcTextSizeA(22 * scale, 9999, 0, current).x;
    const float capacityWidth = font->CalcTextSizeA(13 * scale, 9999, 0, capacity).x;
    const float gainWidth = row.gained > 0 ? font->CalcTextSizeA(11 * scale, 9999, 0, reward).x + 10 * scale : 0;
    const float textFit =
        std::min(1.0f, (kRowWidth - 33) * scale / (currentWidth + 3 * scale + capacityWidth + gainWidth));
    const float baseline = pos.y + 28 * scale;
    const ImVec2 countPos(pos.x + 33 * scale, baseline - 22 * scale * textFit);
    Text(draw, font, 22 * scale * textFit, countPos, Ink(247, 246, 229, alpha), current, alpha);
    const float capacityX = countPos.x + (currentWidth + 3 * scale) * textFit;
    Text(draw, font, 13 * scale * textFit, ImVec2(capacityX, baseline - 13 * scale * textFit),
         Ink(211, 219, 211, alpha * 0.92f), capacity, alpha);
    if (row.gained > 0) {
        const float gainX = capacityX + (capacityWidth + 10 * scale) * textFit;
        Text(draw, font, 11 * scale * textFit, ImVec2(gainX, baseline - 11 * scale * textFit), Ink(accent, alpha),
             reward, alpha);
    }
    // A brief deterministic glint: no heap allocations, emitter, or RNG changes.
    if (row.pulse < 0.65f) {
        const float t = row.pulse / 0.65f;
        Spark(draw, p(22 + t * 3, 5 - t * 4), (2.5f - t) * scale, Ink(237, 255, 218, alpha * (1 - t)));
    }
}
} // namespace PickupCounter
