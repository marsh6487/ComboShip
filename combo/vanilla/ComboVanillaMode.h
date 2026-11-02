#ifndef COMBO_VANILLA_MODE_H
#define COMBO_VANILLA_MODE_H

#include "../ComboGameMode.h"

#ifdef COMBO_BUILD
#ifdef __cplusplus
extern "C" {
#endif
/* Hidden per game DLL, set from the selected file rather than menu CVars. */
extern int gComboVanillaMode;
extern int gComboVanillaCreationFailed;
int Combo_PeekVanillaMode(int slot);
int Combo_QueryVanillaMode(int slot);
extern int (*gComboVanillaCreateCallback)(int slot, int phase);
#ifdef __cplusplus
}
#endif
static inline int Combo_IsVanilla(void) {
    return Combo_IsCombinedGame() && gComboVanillaMode == 1;
}
#else
static inline int Combo_IsVanilla(void) {
    return 0;
}
#endif

#endif
