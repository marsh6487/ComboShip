#pragma once
#include "../../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"

// Procedural effects use authored GI units. A binary weapon's coordinate
// conversion belongs only to its geometry and matching Din material layers.
static inline void ComboSwordGi_ApplyEffectFit(NeiGi::Kind kind, bool shop = false, int mmPickup = 0) {
    if (const auto* bounds = NeiGi::FindSwordFrameBounds(kind)) {
        const auto fit = NeiGi::FrameFit(*bounds, 1.f, shop, mmPickup);
        Matrix_Translate(0.f, fit.lift, 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit.scale, fit.scale, fit.scale, MTXMODE_APPLY);
    }
}
