#pragma once
#include "z64animation.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Opt-in player-only sidecar; ordinary skeletons keep their rigid Flex path. */
const void* PlayerFlexSkin_GetPlan(void** skeleton, s32 matrixCount);
s32 PlayerFlexSkin_MatrixCount(const void* plan);
void PlayerFlexSkin_BlendMatrix(const void* plan, s32 index, const MtxF* nativeMatrices, MtxF* result);
/* Weighted companions never replace equipment/hidden/first-person overrides. */
Gfx* PlayerFlexSkin_SelectLimb(const void* plan, s32 index, Gfx* selected, Gfx* original);
#ifdef __cplusplus
}
#endif
