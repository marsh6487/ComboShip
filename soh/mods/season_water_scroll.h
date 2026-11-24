#pragma once

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 SeasonScene_IsWaterScrollFrozen(const PlayState* play);

#ifdef __cplusplus
}
#endif

static inline Gfx* Gfx_TwoTexWaterScroll(PlayState* play, s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2,
                                         u32 x2, u32 y2, s32 width2, s32 height2, s32 xStep1, s32 yStep1, s32 xStep2,
                                         s32 yStep2) {
    if (SeasonScene_IsWaterScrollFrozen(play)) {
        // Native, non-interpolated bounds keep the ice still even between game ticks.
        return Gfx_TwoTexScroll(play->state.gfxCtx, tile1, 0, 0, width1, height1, tile2, 0, 0, width2, height2);
    }
    return Gfx_TwoTexScrollEx(play->state.gfxCtx, tile1, x1, y1, width1, height1, tile2, x2, y2, width2, height2,
                              xStep1, yStep1, xStep2, yStep2);
}
