#pragma once
#include "../../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"

// Enlarge custom sword pickups around the existing frame's center.
// Apply once in caller/GI units, outside the model's binary coordinate fit,
// so the blade, material layers and award-space effects grow together.
static inline void ComboSwordGi_ApplyPresentationSize(bool shop = false, int mmPickup = 0) {
    // Preserve shelves. Goron's close Item0 camera is already tight around
    // Kokiri's shimmer, so preserve that complete model/effect fit too.
    if (shop || mmPickup == 2)
        return;
    constexpr float size = 1.15f;
    const float center = mmPickup ? -14.f : -2.f;
    Matrix_Translate(0.f, (1.f - size) * center, 0.f, MTXMODE_APPLY);
    Matrix_Scale(size, size, size, MTXMODE_APPLY);
}

// The pre-polish selected sword shelf used the 1.8-radian native pose.
// Descriptors carry straight-up pickup poses; retain that shelf recipe only.
static inline float ComboSwordGi_SelectedTilt(float tilt, bool shop = false) {
    return shop && tilt > 1.5707f && tilt < 1.5709f ? 1.8f : tilt;
}

// Procedural effects use authored GI units. A binary weapon's coordinate
// conversion belongs only to its geometry and matching Din material layers.
static inline void ComboSwordGi_ApplyEffectFit(NeiGi::Kind kind, bool shop = false, int mmPickup = 0) {
    if (const auto* bounds = NeiGi::FindSwordFrameBounds(kind)) {
        const auto fit = NeiGi::FrameFit(*bounds, 1.f, shop, mmPickup);
        Matrix_Translate(0.f, fit.lift, 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit.scale, fit.scale, fit.scale, MTXMODE_APPLY);
    }
}
