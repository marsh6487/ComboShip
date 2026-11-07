#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

// Remnant from OoT
void Regs_InitData(PlayState* play) {
    R_MINIMAP_DISABLED = CVarGetInteger("gEnhancements.MinimalUI", 0);
}
