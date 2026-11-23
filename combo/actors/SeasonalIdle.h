#pragma once

// Native types come from the host's global.h. This is a draw-time payload,
// with no changes to the player's animation, action, face or joint buffers.
enum {
    SEASONAL_IDLE_FIRST_ARM = 14,
    SEASONAL_IDLE_ARM_COUNT = 6,
};

typedef struct SeasonalIdleDraw {
    s32 active;
    Vec3s rotations[SEASONAL_IDLE_ARM_COUNT];
    Gfx* rigid[2];
    Gfx* weighted[2];
    Gfx* ordinary[2][3];
} SeasonalIdleDraw;

#ifdef __cplusplus
extern "C" {
#endif
void PlayerFlexSkin_LoadSeasonalIdle(PlayState* play, const void* actor, void** skeleton, Vec3s* joints,
                                     SeasonalIdleDraw* idle);
#ifdef __cplusplus
}
#endif
