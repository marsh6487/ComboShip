#ifndef COMBO_VANILLA_POLICY_H
#define COMBO_VANILLA_POLICY_H

/* Creation choices never determine the identity of an existing slot. */
static inline int ComboVanilla_CanStart(int combined, int vanillaChoice, int seedReady) {
    return (combined && vanillaChoice) || seedReady;
}

static inline int ComboVanilla_MMFirstCycle(int vanillaMode, unsigned int threeDayResetCount) {
    return vanillaMode && threeDayResetCount == 0;
}

#endif
