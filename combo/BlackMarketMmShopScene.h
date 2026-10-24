#pragma once
#include <cstdint>

namespace BlackMarketMmShopScene {
inline constexpr int16_t kKeeperActor = 42;
inline constexpr int16_t kKeeperParams = 1;
inline constexpr int16_t kItemParamBase = 28672;
inline constexpr int16_t kCameraSetting = 31;
inline constexpr int16_t kCutsceneCount = 4;
inline constexpr int16_t kFirstCutscene = 0;
inline constexpr int16_t kFirstPriority = 10;
inline constexpr int16_t kHudVisibility = 4;
inline constexpr int16_t kLetterboxSize = 32;

// Room adaptation of the native Trading Post Fixed1 camera records. The room,
// angled display banks and stock anchors stay unchanged. These coordinates are
// authored for this room; they are not a rigid transform of the donor room.
// Each row is position XYZ, rotation XYZ, then FOV/rate/unused. FOV -1 selects
// native Fixed1's default 60; rate -1 selects its default 1.0, and 75 is 0.75.
// The rounded bank eyes follow the authored 30-degree bank normals. Their
// distance fits the full native 32-pixel-per-edge letterbox, selected-item/seal
// envelopes and 24-pixel cursor without changing any room or stock coordinates.
inline constexpr int16_t kCameraRecords[4][9] = {
    { 0, 53, 310, 0, -32768, 0, -1, -1, -1 },
    { 0, 53, 310, 0, -32768, 0, -1, 75, -1 },
    { 57, 53, 310, 0, -27307, 0, -1, 75, -1 },
    { -57, 53, 310, 0, 27307, 0, -1, 75, -1 },
};

// Native chain order: keeper -> left shelf -> right shelf -> return to keeper.
inline constexpr int16_t kCutsceneCameras[4] = { 0, 2, 3, 1 };

// Native EnOssan's shelf-to-selection tween keeps its original timing. Adapt
// only its two endpoints to this room: right bank first, then left bank.
inline constexpr float kSelectedTargets[2][3] = {
    { 27.5f, 84.0f, 163.79166f },
    { -27.5f, 84.0f, 163.79166f },
};
} // namespace BlackMarketMmShopScene
