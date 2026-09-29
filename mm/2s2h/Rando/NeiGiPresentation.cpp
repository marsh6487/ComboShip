#include "NeiGiPresentation.h"
#include "NeiResourceRouting.h"
#include "variables.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "ComboResolve.h"
#include <algorithm>
#include <cstring>
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" {
#include "functions.h"
#include "macros.h"
#include "mods/nei_oot_compat.h"
}
#include "../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiGiEnergyTexture.h"

using NeiGi::Kind;
namespace {
bool HasResource(const char* path) {
    return NeiResource_Available(path);
}
float Spin(PlayState* play) {
    const uint32_t bits = (static_cast<uint32_t>(play->gameplayFrames) * 2u) & 0xFFFFu;
    return (bits >= 0x8000u ? static_cast<int32_t>(bits) - 0x10000 : static_cast<int32_t>(bits)) * .01f;
}
struct Binding {
    RandoItemId id;
    const char* slug;
};
const Binding kBindings[] = {
    { RI_OOT_ROCS_FEATHER, "rocs_feather" },
    { RI_OOT_NEI_ROCS_FEATHER, "rocs_feather" },
    { RI_OOT_NEI_WHIP, "whip" },
    { RI_OOT_NEI_FIRE_ROD, "fire_rod" },
    { RI_OOT_NEI_ICE_ROD, "ice_rod" },
    { RI_OOT_NEI_LIGHT_ROD, "light_rod" },
    { RI_OOT_NEI_DEKU_LEAF, "deku_leaf" },
    { RI_OOT_NEI_SWITCH_HOOK, "switch_hook" },
    { RI_OOT_NEI_MOGMA_MITTS, "mogma_mitts" },
    { RI_OOT_NEI_GUST_JAR, "gust_jar" },
    { RI_OOT_NEI_BALL_AND_CHAIN, "ball_and_chain" },
    { RI_OOT_NEI_TIME_GATE, "time_gate" },
    { RI_OOT_NEI_BEETLE, "beetle" },
    { RI_OOT_NEI_SHOVEL, "shovel" },
    { RI_OOT_NEI_HYLIAS_GRACE, "hylia_grace" },
    { RI_OOT_NEI_ZONAI_PERMAFROST, "zonai_permafrost" },
    { RI_OOT_NEI_DEMISE_DESTRUCTION, "demise_destruction" },
    { RI_OOT_NEI_ROCS_CAPE, "rocs_cape" },
    { RI_OOT_NEI_SPINNER, "spinner" },
    { RI_OOT_NEI_CANE_OF_SOMARIA, "cane_of_somaria" },
    { RI_OOT_NEI_CANE_SOMARIA_BLOCK, "cane_of_somaria" },
    { RI_OOT_NEI_CANE_SOMARIA_PLATFORM, "cane_of_somaria" },
    { RI_OOT_NEI_MINISH_CAP, "minish_cap" },
    { RI_OOT_NEI_LANTERN, "lantern" },
};
} // namespace

extern "C" {
#define Graph_Alloc(gfx, bytes) GRAPH_ALLOC(gfx, bytes)
#define NeiGi_DrawTexturedMesh NeiGi_DrawTexturedMeshNative
#include "../../../soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc"
#undef NeiGi_DrawTexturedMesh
#undef Graph_Alloc
}

// Texture commands resolve through the interpreter's active resource manager,
// unlike G_DL_OTR_FILEPATH which parses @oot:. Keep the raw texture path and
// bracket exactly the native mesh submission in its asset owner's namespace.
extern "C" bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh,
                                       const NeiGi::TextureMaterial& material) {
    if (!play || !mesh.count || !HasResource(material.path))
        return false;
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material);
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    return true;
}

void DrawOotSlateRuneFlame(u8 r, u8 g, u8 b);

void MM_DrawNeiGi(const CwItemDrawInfo& info) {
    PlayState* play = gPlayState;
    if (!play || info.dlistCount < 1 || !info.dlists[0])
        return;
    Matrix_Push();
    if (info.neiSomariaUpgrade)
        DrawOotSlateRuneFlame(255, 60, 60);
    const Kind effect = static_cast<Kind>(info.neiEffect);
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Scale(info.scale, info.scale, info.scale, MTXMODE_APPLY);
    Matrix_RotateYF(Spin(play), MTXMODE_APPLY);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, info.dlists[0], 0, G_DL_PUSH);
    Matrix_Pop();
    Matrix_Push();
    Matrix_RotateYF(Spin(play), MTXMODE_APPLY);
    const auto camera = NeiGi_CameraBasis(play);
    if (NeiGi::IsRod(effect) || NeiGi::IsSpell(effect)) {
        Matrix_Push();
        Matrix_Translate(info.neiEffectCenter[0], info.neiEffectCenter[1], info.neiEffectCenter[2], MTXMODE_APPLY);
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(effect, camera), effect);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(effect, play->gameplayFrames, camera));
        Matrix_Pop();
    }
    if (CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0)) {
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, camera, effect));
    }
    Matrix_Pop();
    if (info.xluStartIndex == 1 && info.dlistCount > 1 && info.dlists[1]) {
        Matrix_Push();
        Matrix_Scale(info.scale, info.scale, info.scale, MTXMODE_APPLY);
        Matrix_RotateYF(Spin(play), MTXMODE_APPLY);
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, info.dlists[1], 0, G_DL_PUSH);
        Matrix_Pop();
    }
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}

bool MM_TryDrawNeiGi(RandoItemId item) {
    const Binding* binding = nullptr;
    for (const auto& candidate : kBindings)
        if (candidate.id == item) {
            binding = &candidate;
            break;
        }
    if (!binding)
        return false;
    static Fn_GetNeiGiDrawInfo describe = nullptr;
    if (!describe)
        describe = (Fn_GetNeiGiDrawInfo)Combo_ResolveSym("soh", "OOT_GetNeiGiDrawInfo");
    if (!describe)
        return false;
    CwItemDrawInfo info{};
    if (describe(binding->slug, &info) != 1)
        return false;
    for (int i = 0; i < info.dlistCount; ++i) {
        info.dlists[i] = NeiResource_Route(info.dlists[i]);
        if (!info.dlists[i])
            return false;
    }
    info.neiSomariaUpgrade = item == RI_OOT_NEI_CANE_SOMARIA_BLOCK || item == RI_OOT_NEI_CANE_SOMARIA_PLATFORM;
    MM_DrawNeiGi(info);
    return true;
}
