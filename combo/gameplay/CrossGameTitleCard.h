#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef COMBO_BUILD
// Kept pending through the native title-card hook, then consumed by Player_Init.
// This is transient arrival state, never a save or entrance-shuffle flag.
int Combo_SOH_HasCrossGameTitleCard(void);
int Combo_SOH_ConsumeCrossGameTitleCard(void);
#else
static inline int Combo_SOH_HasCrossGameTitleCard(void) {
    return 0;
}
static inline int Combo_SOH_ConsumeCrossGameTitleCard(void) {
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif
