#pragma once
// Only the native names needed by the two OoT-derived companion actors.
#include "z64.h"
#include "z64animation.h"
#define SkelAnime_DrawSkeletonOpa(play, skel, override, post, self) \
    SkelAnime_DrawFlexOpa(play, (skel)->skeleton, (skel)->jointTable, (skel)->dListCount, override, post, self)
