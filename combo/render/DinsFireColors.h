/* Existing native Din cosmetic entries, shared by rendering and actor lights. */
#ifndef COMBO_DINS_FIRE_COLORS_H
#define COMBO_DINS_FIRE_COLORS_H

#include "DinsFireRemake.h"
#include <libultraship/bridge/consolevariablebridge.h>
#ifdef CW_DINS_HOST_MM
#include "2s2h/BenGui/CosmeticEditor.h"
#endif

static inline int CwDinsFire_CustomPalette(uint32_t primary, uint32_t secondary) {
    return primary != 0xFFC800 || secondary != 0xFF0000;
}

static inline void CwDinsFire_GetColors(uint32_t* primary, uint32_t* secondary) {
    *primary = 0xFFC800;
    *secondary = 0xFF0000;
#ifdef CW_DINS_HOST_MM
    Color_RGBA8 p = CosmeticEditor_GetChangedColor(255, 200, 0, 255, COSMETIC_ID("Magic.DinsPrimary"));
    Color_RGBA8 e = CosmeticEditor_GetChangedColor(255, 0, 0, 255, COSMETIC_ID("Magic.DinsSecondary"));
    *primary = (uint32_t)p.r << 16 | (uint32_t)p.g << 8 | p.b;
    *secondary = (uint32_t)e.r << 16 | (uint32_t)e.g << 8 | e.b;
#else
    if (CVarGetInteger(CVAR_COSMETIC("Magic.DinsPrimary.Changed"), 0)) {
        Color_RGB8 p = CVarGetColor24(CVAR_COSMETIC("Magic.DinsPrimary.Value"), (Color_RGB8){ 255, 200, 0 });
        *primary = (uint32_t)p.r << 16 | (uint32_t)p.g << 8 | p.b;
    }
    if (CVarGetInteger(CVAR_COSMETIC("Magic.DinsSecondary.Changed"), 0)) {
        Color_RGB8 e = CVarGetColor24(CVAR_COSMETIC("Magic.DinsSecondary.Value"), (Color_RGB8){ 255, 0, 0 });
        *secondary = (uint32_t)e.r << 16 | (uint32_t)e.g << 8 | e.b;
    }
#endif
}

static inline uint32_t CwDinsFire_LocalLightColor(uint32_t primary, uint32_t secondary) {
    return CwDins_NativeColor(0xFF7D1F, primary, secondary);
}

#endif
