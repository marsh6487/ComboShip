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
    { RI_OOT_NEI_ELEMENTAL_WAND, "elemental_wand" },
    { RI_OOT_NEI_WAND_SAND_ROD, "sand_rod" },
    { RI_OOT_NEI_WAND_TORNADO_ROD, "tornado_rod" },
    { RI_OOT_NEI_WAND_WATER_ROD, "water_rod" },
    { RI_OOT_NEI_WAND_METEOR_ROD, "meteor_rod" },
    { RI_OOT_NEI_WAND_STORM_ROD, "storm_rod" },
    { RI_OOT_NEI_WAND_SHADOW_SCEPTER, "shadow_scepter" },
    { RI_OOT_EXT_DIVINE_SHIELD, "divine_shield" },
    { RI_OOT_EXT_SHEIKAH_SHIELD, "sheikah_shield" },
    { RI_SHIELD_MIRROR, "shield_of_ikana" },
    { RI_OOT_EXT_MAGIC_CAPE, "magic_cape" },
    { RI_OOT_EXT_SPIRIT_BREASTPLATE, "spirit_breastplate" },
    { RI_OOT_EXT_WATER_DRAGON_SCALE, "sages_tunic" },
    { RI_OOT_EXT_CHAMPIONS_TUNIC, "champions_tunic" },
    { RI_OOT_EXT_PEGASUS_ANKLET, "pegasus_anklet" },
    { RI_OOT_EXT_TRIDENT, "trident" },
    { RI_OOT_EXT_CLIMB_BOOTS, "climb_boots" },
    { RI_OOT_EXT_ROC_BOOTS, "roc_boots" },
    { RI_OOT_EXT_CANE_OF_BYRNA, "cane_of_byrna" },
    { RI_OOT_EXT_FOUR_SWORD, "four_sword" },
    { RI_PENDANT_OF_MEMORIES, "pendant_of_memories" },
    { RI_OOT_NEI_SHEIKAH_SLATE, "sheikah_slate" },
    { RI_OOT_NEI_SLATE_RUNE_BOMB, "slate_bomb" },
    { RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE, "slate_master_cycle" },
    { RI_OOT_NEI_SLATE_RUNE_STASIS, "slate_stasis" },
    { RI_OOT_NEI_SLATE_RUNE_CRYONIS, "slate_cryonis" },
    { RI_OOT_NEI_DESIRE_SENSOR, "slate_sensor" },
    { RI_OOT_NEI_PHANTOM_HOURGLASS, "phantom_hourglass" },
    { RI_OOT_NEI_SHADOW_CRYSTAL, "shadow_crystal" },
    { RI_OOT_NEI_ROD_OF_SEASONS, "rod_of_seasons" },
    { RI_SWORD_KOKIRI, "kokiri_sword" },
    { RI_SWORD_RAZOR, "razor_sword" },
    { RI_SWORD_GILDED, "gilded_sword" },
    { RI_OOT_MASTER_SWORD, "master_sword" },
    { RI_OOT_TRUE_MASTER_SWORD, "true_master_sword" },
    { RI_OOT_BIGGORON_SWORD, "biggoron_sword" },
    { RI_GREAT_FAIRY_SWORD, "great_fairy_sword" },
    { RI_OOT_IRON_KNUCKLE_AXE, "iron_knuckle_axe" },
};
} // namespace

extern "C" {
#define Graph_Alloc(gfx, bytes) GRAPH_ALLOC(gfx, bytes)
#define NeiGi_DrawTexturedMesh NeiGi_DrawTexturedMeshNative
#define NEI_GI_ROTATE_Y Matrix_RotateYF
#include "../../../soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc"
#undef NeiGi_DrawTexturedMesh
#undef NEI_GI_ROTATE_Y
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
    if (play && info.drawKind == CW_DRAW_KIND_SEASON_GI) {
        if (info.neiEffect >= 1 && info.neiEffect <= 4 && info.dlistCount == 0 && info.opCount == 0)
            NeiGi_DrawSeasonOverlay(play, info.neiEffect, nullptr);
        return;
    }
    if (!play || info.dlistCount < 1 || !info.dlists[0])
        return;
    Matrix_Push();
    if (info.neiSomariaUpgrade)
        DrawOotSlateRuneFlame(255, 60, 60);
    NeiGi_DrawPresentation(play, info.dlists[0],
                           info.xluStartIndex == 1 && info.dlistCount > 1 ? info.dlists[1] : nullptr, info.scale,
                           info.neiEffect, info.neiEffectCenter,
                           info.itemShimmer || CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0), "oot");
    Matrix_Pop();
}

bool MM_DescribeNeiGi(RandoItemId item, CwItemDrawInfo* out) {
    if (!out)
        return false;
    if (item >= RI_OOT_NEI_SEASON_SPRING && item <= RI_OOT_NEI_SEASON_WINTER) {
        *out = CwItemDrawInfo{};
        out->drawKind = CW_DRAW_KIND_SEASON_GI;
        out->neiEffect = 1 + item - RI_OOT_NEI_SEASON_SPRING;
        out->xluStartIndex = -1;
        return true;
    }
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
    *out = info;
    return true;
}

bool MM_TryDrawNeiGi(RandoItemId item) {
    if (!gPlayState)
        return false;
    CwItemDrawInfo info{};
    if (!MM_DescribeNeiGi(item, &info))
        return false;
    MM_DrawNeiGi(info);
    return true;
}

MM_NeiGiFallbackShimmer::MM_NeiGiFallbackShimmer(RandoItemId item) : mItem(item), mEnabled(false) {
    if (!gPlayState)
        return;
    const bool sword = item == RI_SWORD_KOKIRI || item == RI_SWORD_RAZOR || item == RI_SWORD_GILDED ||
                       item == RI_GREAT_FAIRY_SWORD || item == RI_OOT_MASTER_SWORD ||
                       item == RI_OOT_TRUE_MASTER_SWORD || item == RI_OOT_BIGGORON_SWORD ||
                       item == RI_OOT_IRON_KNUCKLE_AXE;
    if (!sword && !CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0))
        return;
    // The four seasons are deliberately absent: their weather is the GI.
    mEnabled = std::any_of(std::begin(kBindings), std::end(kBindings),
                           [item](const Binding& binding) { return binding.id == item; });
    if (mEnabled)
        Matrix_Push();
}

MM_NeiGiFallbackShimmer::~MM_NeiGiFallbackShimmer() {
    if (!mEnabled)
        return;
    Matrix_Pop();
    const Kind kind = mItem == RI_OOT_NEI_DEKU_LEAF ? Kind::Leaf : Kind::Neutral;
    NeiGi_DrawMesh(gPlayState, NeiGi::SampleShimmer(gPlayState->gameplayFrames, true,
                                                  NeiGi_CameraBasis(gPlayState), kind));
}
