#pragma once

#include "ZfgWalletRupees.h"
#include "ZfgWalletClassicLists.h"
#include "objects/object_gi_purse/object_gi_purse.h"
#include "objects/object_gi_rupy/object_gi_rupy.h"
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"

extern "C" uint8_t ResourceMgr_FileExists(const char* path);
extern "C" Gfx* ResourceMgr_LoadGfxByName(const char* path);

#ifdef COMBO_ZFG_WALLET_HOST_MM
#define ZFG_GI_OPA Gfx_SetupDL25_Opa
#define ZFG_GI_XLU Gfx_SetupDL25_Xlu
#define ZFG_GI_MATRIX(gfx) Matrix_Finalize(gfx)
#define ZFG_GI_ROTATE_Y Matrix_RotateYF
#define ZFG_GI_ROTATE_Z Matrix_RotateZF
#else
#define ZFG_GI_OPA Gfx_SetupDL_25Opa
#define ZFG_GI_XLU Gfx_SetupDL_25Xlu
#define ZFG_GI_MATRIX(gfx) MATRIX_NEWMTX(gfx)
#define ZFG_GI_ROTATE_Y Matrix_RotateY
#define ZFG_GI_ROTATE_Z Matrix_RotateZ
#endif

namespace ComboZfgWallet {
// OPEN_DISPS declares these in its enclosing namespace in debug builds.
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild();
static ClassicLists LoadClassicLists(PlayState* play, const char* owner);

struct PouchSection {
    const char* material;
    const char* body;
    uint8_t prim[3];
    uint8_t env[3];
};

static Gfx sBindCord[] = {
    gsDPLoadTextureBlock(kCordTexture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_WRAP, G_TX_WRAP, 6, 6, G_TX_NOLOD,
                         G_TX_NOLOD),
    gsSPEndDisplayList(),
};
static Gfx sBindBadge[] = {
    gsDPLoadTextureBlock(kBadgeTexture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_WRAP, G_TX_WRAP, 6, 6, G_TX_NOLOD,
                         G_TX_NOLOD),
    gsSPEndDisplayList(),
};

static inline void DrawPouch(PlayState* play, const char* owner, bool twilight) {
    GraphicsContext* gfx = play->state.gfxCtx;
    const auto classic = twilight ? ClassicLists{} : LoadClassicLists(play, owner);
    NeiGi_ReportBlackMarketModel(owner, twilight ? kTwilightModel : gGiWalletDL);
    OPEN_DISPS(gfx);
    ZFG_GI_OPA(gfx);
    gSPMatrix(POLY_OPA_DISP++, ZFG_GI_MATRIX(gfx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    if (twilight) {
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kTwilightModel);
    } else {
        static constexpr PouchSection sections[] = {
            { gGiGiantsWalletColorDL, gGiWalletDL, { 156, 42, 61 }, { 50, 9, 22 } },
            { gGiGiantsWalletRupeeOuterColorDL, gGiWalletRupeeOuterDL, { 206, 151, 64 }, { 67, 29, 10 } },
            { gGiGiantsWalletStringColorDL, gGiWalletStringDL, { 249, 200, 110 }, { 86, 46, 13 } },
            { gGiGiantsWalletRupeeInnerColorDL, gGiWalletRupeeInnerDL, { 255, 241, 165 }, { 100, 56, 16 } },
        };
        Gfx* const privateLists[] = { classic.cloth, nullptr, nullptr, classic.badge };
        Gfx* const bindings[] = { nullptr, sBindBadge, sBindCord, nullptr };
        for (size_t i = 0; i < std::size(sections); ++i) {
            const auto& part = sections[i];
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)part.material);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, part.prim[0], part.prim[1], part.prim[2], 255);
            gDPSetEnvColor(POLY_OPA_DISP++, part.env[0], part.env[1], part.env[2], 255);
            if (classic.cloth && bindings[i])
                gSPDisplayList(POLY_OPA_DISP++, bindings[i]);
            gSPDisplayList(POLY_OPA_DISP++, privateLists[i] ? privateLists[i] : (Gfx*)part.body);
        }
    }
    ZFG_GI_OPA(gfx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    CLOSE_DISPS(gfx);
}

static inline void DrawOverflow(PlayState* play, Point mouth) {
    auto rupees = SampleRupees(play->gameplayFrames, mouth);
    const auto forward = NeiGi_CameraBasis(play).forward;
    const auto depth = [&](const Rupee& r) {
        return r.center.x * forward.x + r.center.y * forward.y + r.center.z * forward.z;
    };
    std::sort(rupees.begin(), rupees.end(), [&](const Rupee& a, const Rupee& b) { return depth(a) < depth(b); });

    static constexpr const char* innerColors[] = {
        gGiGreenRupeeInnerColorDL,
        gGiBlueRupeeInnerColorDL,
        gGiGoldRupeeInnerColorDL,
    };
    static constexpr const char* outerColors[] = {
        gGiGreenRupeeOuterColorDL,
        gGiBlueRupeeOuterColorDL,
        gGiGoldRupeeOuterColorDL,
    };
    GraphicsContext* gfx = play->state.gfxCtx;
    OPEN_DISPS(gfx);
    ZFG_GI_XLU(gfx);
    for (const Rupee& rupee : rupees) {
        if (rupee.alpha == 0)
            continue;
        Matrix_Push();
        Matrix_Translate(rupee.center.x, rupee.center.y, rupee.center.z, MTXMODE_APPLY);
        ZFG_GI_ROTATE_Y(rupee.yaw, MTXMODE_APPLY);
        ZFG_GI_ROTATE_Z(rupee.roll, MTXMODE_APPLY);
        Matrix_Scale(rupee.scale, rupee.scale, rupee.scale, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, ZFG_GI_MATRIX(gfx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPModelOpacityPush(POLY_XLU_DISP++, rupee.alpha);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)innerColors[rupee.color]);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gGiRupeeInnerDL);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)outerColors[rupee.color]);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gGiRupeeOuterDL);
        gSPModelOpacityPop(POLY_XLU_DISP++);
        Matrix_Pop();
    }
    // The next XLU draw inherits the incoming GI pose, not the last rupee transform.
    gSPMatrix(POLY_XLU_DISP++, ZFG_GI_MATRIX(gfx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    ZFG_GI_XLU(gfx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    CLOSE_DISPS(gfx);
}

static inline void Draw(PlayState* play, const char* owner) {
    if (!NeiGi_CanDrawLayers(play, 1, 32, 2))
        return;
    const bool twilight = ResourceMgr_FileExists(kTwilightModel) && ResourceMgr_LoadGfxByName(kTwilightModel);
    DrawPouch(play, owner, twilight);
    if (NeiGi_CanDrawLayers(play, 4, 2, 40))
        DrawOverflow(play, twilight ? kTwilightMouth : kClassicMouth);
}
} // namespace ComboZfgWallet

#undef ZFG_GI_OPA
#undef ZFG_GI_XLU
#undef ZFG_GI_MATRIX
#undef ZFG_GI_ROTATE_Y
#undef ZFG_GI_ROTATE_Z
