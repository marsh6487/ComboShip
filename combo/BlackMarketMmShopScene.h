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
inline constexpr const char* kPresentationResources[] = {
    "custom/skiejer_market/mm_shop_presentation_v1/room",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_15",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_15",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_17",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_17",
};

// Room adaptation of the native Trading Post Fixed1 camera records. The room,
// angled display banks and stock anchors stay unchanged. These coordinates are
// authored for this room; they are not a rigid transform of the donor room.
// Each row is position XYZ, rotation XYZ, then FOV/rate/unused. FOV -1 selects
// native Fixed1's default 60; rate -1 selects its default 1.0, and 75 is 0.75.
// The rounded bank eyes follow the authored 30-degree bank normals. Their
// distance fits the full native 32-pixel-per-edge letterbox, selected-item/seal
// envelopes and 24-pixel cursor without changing any room or stock coordinates.
// The bank shots advance along those normals; the unchanged keeper shots
// retain their original scene position.
inline constexpr int16_t kCameraRecords[4][9] = {
    { 0, 53, 310, 0, -32768, 0, -1, -1, -1 },
    { 0, 53, 310, 0, -32768, 0, -1, 75, -1 },
    { 50, 58, 298, 0, -27307, 0, -1, 75, -1 },
    { -50, 58, 298, 0, 27307, 0, -1, 75, -1 },
};

// Native chain order: keeper -> left shelf -> right shelf -> return to keeper.
inline constexpr int16_t kCutsceneCameras[4] = { 0, 2, 3, 1 };

// Native EnOssan's shelf-to-selection tween keeps its original timing. Adapt
// only its two endpoints to this room: right bank first, then left bank.
// Move the offer into the clear customer aisle for a readable confirmation
// view. The former target kept it near the shelves, small in the wide shot.
inline constexpr float kSelectedTargets[2][3] = {
    { 8.0f, 67.0f, 198.0f },
    { -8.0f, 67.0f, 198.0f },
};
} // namespace BlackMarketMmShopScene
