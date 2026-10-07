#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
// Stable receipt profiles: 1 Fire, 2 Ice, 3 Light. No actor IDs cross the DLL ABI.
static inline int NeiArrowGi_ProfileForDrawId(int id, int fire, int ice, int light) {
    return id == fire ? 1 : id == ice ? 2 : id == light ? 3 : 0;
}
void NeiGi_DrawElementalArrow(struct PlayState* play, int profile);
#ifdef __cplusplus
}
#endif
