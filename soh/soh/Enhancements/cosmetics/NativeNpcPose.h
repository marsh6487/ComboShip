#pragma once
#include "z64animation.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Returns the original animation when Alt is off, the rig is unrelated or a
 * private fitted clip is absent/invalid. The actor retains its native identity. */
AnimationHeader* NativeNpcPose_ResolveAnimation(const SkelAnime* skelAnime, AnimationHeader* animation);
/* Optional pose-specific meshes use the same full native-animation key. */
Gfx* NativeNpcPose_SelectLimb(const SkelAnime* skelAnime, s32 limbIndex, Gfx* original);
Gfx* NativeNpcPose_SelectProp(const SkelAnime* skelAnime, const char* prop, Gfx* original);
#ifdef __cplusplus
}
#endif
