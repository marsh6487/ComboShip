#ifndef COMBO_FAIRY_BOTTLE_H
#define COMBO_FAIRY_BOTTLE_H

#include <math.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    const char* opaque;
    const char* glass;
} ComboFairyBottleShell;

// A fairy-specific mod owns its whole shell, including partial OPA/XLU packs.
// Otherwise prefer the user's selected Blue Fire bottle model, then a selected
// generic bottle. An unmodified Blue Fire chamberstick is never a bottle fallback.
static inline ComboFairyBottleShell ComboFairyBottle_SelectShell(const char* opaque, const char* glass,
                                                                 const char* genericOpaque, const char* genericGlass,
                                                                 int (*isSelectedMod)(const char*),
                                                                 const char* blueFire, int blueFireSelected) {
    ComboFairyBottleShell shell = { opaque, glass };
    if (isSelectedMod != 0 && !isSelectedMod(opaque) && !isSelectedMod(glass)) {
        if (blueFire != 0 && blueFireSelected) {
            // Repeated shell paths encode a single-piece shell across the ABI.
            // Consumers submit it once; the Blue Fire flame is never selected.
            shell.opaque = shell.glass = blueFire;
        } else if (isSelectedMod(genericOpaque) || isSelectedMod(genericGlass)) {
            shell.opaque = genericOpaque;
            shell.glass = genericGlass;
        }
    }
    return shell;
}

static inline int ComboFairyBottle_IsBlueFireShell(const char* path) {
    return path != 0 && strstr(path, "objects/object_gi_fire/gGiBlueFireChamberstickDL") != 0;
}

typedef struct {
    float x, y, z;
    float scaleX, scaleY, scaleZ;
} ComboFairyBottleMotion;

// Keep the native contents and their placement matrix. Only the contents move:
// <= .65 X, 1.15 Y, .35 Z GI units, with a smaller .74..82 fluttering silhouette.
// This guarantees the transform envelope, not containment in arbitrary mod art.
// A shared active-host clock avoids frozen motion when the asset owner is dormant.
static inline ComboFairyBottleMotion ComboFairyBottle_Sample(uint32_t frame) {
    const float phase = (float)(frame % 360u) * (6.2831853071795864769f / 180.0f);
    ComboFairyBottleMotion motion = { .65f * sinf(phase),
                                      1.15f * sinf(phase * 1.5f + .4f),
                                      .35f * cosf(phase),
                                      .78f + .04f * sinf(phase * 30.0f),
                                      .78f,
                                      .78f };
    return motion;
}

#endif
