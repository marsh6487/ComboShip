// TU glue shared by native and foreign mask GIs. Include after the host engine headers.
#ifndef COMBO_MASK_SHIMMER_H
#define COMBO_MASK_SHIMMER_H

#include <stdint.h>

// 0 = ordinary mask; 1..4 = transformations; 5..8 = Odolwa, Goht, Gyorg, Twinmold.
static inline void ComboMaskShimmerColor(int profile, uint8_t color[4]) {
    static const uint8_t colors[9][4] = {
        { 220, 225, 240, 255 }, { 50, 220, 90, 255 }, { 240, 64, 64, 255 }, { 64, 144, 255, 255 }, { 0, 0, 0, 255 },
        { 145, 20, 133, 255 },  { 10, 138, 46, 255 }, { 19, 99, 165, 255 }, { 168, 180, 20, 255 },
    };
    int i;
    for (i = 0; i < 4; ++i) {
        color[i] = colors[profile >= 0 && profile < 9 ? profile : 0][i];
    }
}

// MM inventory ordering, including OoT's imported aliases. These are item slots, not GIDs.
static inline int ComboMmMaskShimmerColor(int index, uint8_t color[4]) {
    int profile = 0;
    if (index < 0 || index >= 24) {
        return 0;
    }
    switch (index) {
        case 5:
            profile = 1;
            break;
        case 11:
            profile = 2;
            break;
        case 17:
            profile = 3;
            break;
        case 23:
            profile = 4;
            break;
    }
    ComboMaskShimmerColor(profile, color);
    return 1;
}

static inline int ComboMmRemainsShimmerColor(int index, uint8_t color[4]) {
    if (index < 0 || index >= 4) {
        return 0;
    }
    ComboMaskShimmerColor(5 + index, color);
    return 1;
}

#ifdef COMBO_MASK_SHIMMER_HOST_MM
#define CMS_SETUP_XLU(ctx) Gfx_SetupDL25_Xlu(ctx)
#define CMS_LOAD_MTX(pkt, ctx) MATRIX_FINALIZE_AND_LOAD(pkt, ctx)
#else
#define CMS_SETUP_XLU(ctx) Gfx_SetupDL_25Xlu(ctx)
#define CMS_LOAD_MTX(pkt, ctx) \
    gSPMatrix(pkt, Matrix_NewMtx(ctx, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD)
#endif

static inline void ComboDrawMaskShimmer(PlayState* play, const char* sparkle, const uint8_t color[4],
                                        const char* owner) {
    int i;
    int dark = color[0] == 0 && color[1] == 0 && color[2] == 0;
    OPEN_DISPS(play->state.gfxCtx);
    CMS_SETUP_XLU(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING);
    if (owner != NULL) {
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    }
    // Local, deterministic motes: drawing extra masks cannot consume RNG or advance effects.
    for (i = 0; i < 5; ++i) {
        uint32_t phase = (play->gameplayFrames + i * 31) & 127;
        s16 angle = (s16)(play->gameplayFrames * 320 + i * 13107);
        float fade = (phase < 64 ? phase : 128 - phase) / 64.0f;
        float size = 0.025f + 0.025f * fade;
        Matrix_Push();
        Matrix_Translate(Math_SinS(angle) * 26.0f, -24.0f + phase * 0.375f, Math_CosS(angle) * 26.0f, MTXMODE_APPLY);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(size, size, size, MTXMODE_APPLY);
        // Fierce Deity keeps black shadows with dim silver highlights; pure black
        // primitive color would make the sparkle texture unreadable on dark shelves.
        gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, dark ? 80 : (color[0] + 255) / 2, dark ? 80 : (color[1] + 255) / 2,
                        dark ? 96 : (color[2] + 255) / 2, (u8)(144.0f * fade));
        gDPSetEnvColor(POLY_XLU_DISP++, color[0], color[1], color[2], 0);
        CMS_LOAD_MTX(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)sparkle);
        Matrix_Pop();
    }
    if (owner != NULL) {
        gSPComboRMPop(POLY_XLU_DISP++);
    }
    CMS_SETUP_XLU(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    CMS_LOAD_MTX(POLY_XLU_DISP++, play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

#undef CMS_SETUP_XLU
#undef CMS_LOAD_MTX
#endif
