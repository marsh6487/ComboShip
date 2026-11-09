#pragma once

// The exact native OoT boss-soul flame pass, shared by added soul receipts.
// Keep the dedicated root: bottle Alt packs may hide gGiBlueFireFlameDL, while
// this root retains the native object_gi_fire textures used by the soul HD pack.
inline void ComboSoulGi_DrawOotFlame(PlayState* play, const uint8_t color[3]) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#ifdef COMBO_BUILD
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
#endif
    gSPSegment(POLY_XLU_DISP++, 8,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 16, 32, 1, play->state.frames,
                                             -(play->state.frames * 8), 16, 32, 0, 0, 1, -8));
    Matrix_Push();
    Matrix_Translate(0.0f, -70.0f, 0.0f, MTXMODE_APPLY);
    Matrix_Scale(5.0f, 5.0f, 5.0f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, color[0], color[1], color[2], 255);
    gSPGrayscale(POLY_XLU_DISP++, true);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)"__OTR__objects/object_boss_soul/gBossSoulFlameDL");
    gSPGrayscale(POLY_XLU_DISP++, false);
    Matrix_Pop();
#ifdef COMBO_BUILD
    gSPComboRMPop(POLY_XLU_DISP++);
#endif
    CLOSE_DISPS(play->state.gfxCtx);
}
