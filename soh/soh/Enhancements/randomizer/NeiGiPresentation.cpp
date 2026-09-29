#include "NeiGiPresentation.h"
#include "NeiGiEffectPolicy.h"
#include "NeiGiEnergyTexture.h"
#include "NeiGiRender.h"
#include "NeiGiShopFit.h"
#include <algorithm>
#include <cstring>
#include "draw.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/frame_interpolation.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
}

namespace {
using NeiGi::Kind;
struct Presentation {
    CustomDrawFunc draw;
    const char* opaque;
    const char* translucent;
    float scale;
    Kind effect;
    Vec3f effectCenter;
    NeiGi::ShopFit shop;
};

#define GI_PATH(slug) "__OTR__objects/nei_gi_redesign/" slug "/gi_dl"
#define GI_XLU(slug) "__OTR__objects/nei_gi_redesign/" slug "/gi_xlu_dl"
// These bindings are for presentation only. Actor and held-item resources retain their own paths.
const Presentation kPresentations[] = {
    { Randomizer_DrawRocsFeatherSkijer,
      GI_PATH("rocs_feather"),
      nullptr,
      .5f,
      Kind::Neutral,
      {},
      NeiGi::kFeatherShopFit },
    { Randomizer_DrawRocsFeather, GI_PATH("rocs_feather"), nullptr, .5f, Kind::Neutral, {}, NeiGi::kFeatherShopFit },
    { Randomizer_DrawWhip, GI_PATH("whip"), nullptr, .5f, Kind::Neutral, {} },
    { Randomizer_DrawFireRod,
      GI_PATH("fire_rod"),
      nullptr,
      .2f,
      Kind::Fire,
      { 9.883f, 30.415f, 0 },
      NeiGi::kRodShopFit },
    { Randomizer_DrawIceRod, GI_PATH("ice_rod"), nullptr, .2f, Kind::Ice, { 10.365f, 31.899f, 0 }, NeiGi::kRodShopFit },
    { Randomizer_DrawLightRod,
      GI_PATH("light_rod"),
      nullptr,
      .2f,
      Kind::Light,
      { 9.883f, 30.415f, 0 },
      NeiGi::kRodShopFit },
    { Randomizer_DrawDekuLeaf, GI_PATH("deku_leaf"), nullptr, .5f, Kind::Leaf, {} },
    { Randomizer_DrawSwitchHook, GI_PATH("switch_hook"), nullptr, .01f, Kind::Neutral, {}, NeiGi::kSwitchHookShopFit },
    { Randomizer_DrawMogmaMitts, GI_PATH("mogma_mitts"), nullptr, .5f, Kind::Neutral, {} },
    { Randomizer_DrawGustJar, GI_PATH("gust_jar"), nullptr, 5.f, Kind::Neutral, {} },
    { Randomizer_DrawBallAndChain,
      GI_PATH("ball_and_chain"),
      nullptr,
      .25f,
      Kind::Neutral,
      {},
      NeiGi::kBallAndChainShopFit },
    { Randomizer_DrawTimeGate, GI_PATH("time_gate"), nullptr, .5f, Kind::Neutral, {}, NeiGi::kTimeGateShopFit },
    { Randomizer_DrawBeetle, GI_PATH("beetle"), nullptr, .3f, Kind::Neutral, {} },
    { Randomizer_DrawShovel, GI_PATH("shovel"), nullptr, .2f, Kind::Neutral, {}, NeiGi::kShovelShopFit },
    { Randomizer_DrawHyliaGrace,
      GI_PATH("hylia_grace"),
      GI_XLU("hylia_grace"),
      1.f,
      Kind::Hylia,
      {},
      NeiGi::kSpellShopFit },
    { Randomizer_DrawZonaiPermafrost,
      GI_PATH("zonai_permafrost"),
      GI_XLU("zonai_permafrost"),
      1.f,
      Kind::Zonai,
      {},
      NeiGi::kSpellShopFit },
    { Randomizer_DrawDemiseDestruction,
      GI_PATH("demise_destruction"),
      GI_XLU("demise_destruction"),
      1.f,
      Kind::Demise,
      {},
      NeiGi::kSpellShopFit },
    { Randomizer_DrawRocsCape, GI_PATH("rocs_cape"), nullptr, .6f, Kind::Neutral, {}, NeiGi::kRocsCapeShopFit },
    { Randomizer_DrawSpinner, GI_PATH("spinner"), nullptr, .3f, Kind::Neutral, {} },
    { Randomizer_DrawBombArrows, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawCaneOfSomaria,
      GI_PATH("cane_of_somaria"),
      nullptr,
      .25f,
      Kind::Neutral,
      {},
      NeiGi::kSomariaShopFit },
    { Randomizer_DrawCaneSomariaUpgrade,
      GI_PATH("cane_of_somaria"),
      nullptr,
      .25f,
      Kind::Neutral,
      {},
      NeiGi::kSomariaShopFit },
    { Randomizer_DrawMinishCap, GI_PATH("minish_cap"), nullptr, .5f, Kind::Neutral, {} },
    { Randomizer_DrawDominionRod, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawMagnesis, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawStasis, nullptr, nullptr, 1.f, Kind::Neutral, {} },
    { Randomizer_DrawLantern, GI_PATH("lantern"), GI_XLU("lantern"), .025f, Kind::Neutral, {} },
    { Randomizer_DrawCryonis, nullptr, nullptr, 1.f, Kind::Neutral, {} },
};
#undef GI_PATH
#undef GI_XLU

float Spin(PlayState* play) {
    // Match DrawCustomItemDiamond's signed 16-bit rotation, including wrap.
    const uint32_t bits = (static_cast<uint32_t>(play->gameplayFrames) * 2u) & 0xFFFFu;
    const int32_t signedBits = bits >= 0x8000u ? static_cast<int32_t>(bits) - 0x10000 : bits;
    return signedBits * .01f;
}

bool HasResource(const char* path) {
    return path != nullptr &&
           (ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path)));
}
} // namespace

// OPEN_DISPS declares interpolation callbacks with the enclosing C linkage.
extern "C" {
#include "NeiGiMeshRenderer.inc"

static void NeiGi_DrawEffects(PlayState* play, const Presentation& item, bool upgraded) {
    const bool shimmer = CVarGetInteger(CVAR_NEI_GI_EFFECTS, 0) != 0;
    const bool energy = upgraded && (NeiGi::IsRod(item.effect) || NeiGi::IsSpell(item.effect));
    if (!shimmer && !energy)
        return;
    Matrix_Push();
    Matrix_RotateY(Spin(play), MTXMODE_APPLY);
    const auto camera = NeiGi_CameraBasis(play);
    if (energy) {
        Matrix_Push();
        Matrix_Translate(item.effectCenter.x, item.effectCenter.y, item.effectCenter.z, MTXMODE_APPLY);
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(item.effect, camera), item.effect);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(item.effect, play->gameplayFrames, camera));
        Matrix_Pop();
    }
    if (shimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, camera, item.effect));
    Matrix_Pop();
}
}

static bool NeiGi_DrawImpl(PlayState* play, GetItemEntry* entry, bool shop) {
    if (play == nullptr || entry == nullptr || entry->drawFunc == nullptr)
        return false;
    const Presentation* item = nullptr;
    for (const auto& candidate : kPresentations) {
        if (candidate.draw == entry->drawFunc) {
            item = &candidate;
            break;
        }
    }
    if (item == nullptr)
        return false;
    // Queue stable paths for the interpreter. Loading through the legacy GBI wrapper
    // here would evict/reload base resources on each draw when Alt Assets is enabled.
    // Archive presence checks preserve the original model if a required pass is absent.
    const bool upgraded = HasResource(item->opaque) && (!item->translucent || HasResource(item->translucent));
    Matrix_Push();
    if (shop && upgraded) {
        // The same shelf pose encloses the mesh, energy and crystal skin.
        // World/overhead sizes and incomplete-resource fallbacks stay intact.
        Matrix_Translate(0, item->shop.lift, 0, MTXMODE_APPLY);
        Matrix_Scale(item->shop.scale, item->shop.scale, item->shop.scale, MTXMODE_APPLY);
    }
    if (upgraded && item->draw == Randomizer_DrawCaneSomariaUpgrade) {
        // Retain the original red skill-upgrade flame with the authored cane.
        Randomizer_DrawCaneSomariaUpgradeFlame(play);
    }
    Matrix_Push();
    if (upgraded) {
        OPEN_DISPS(play->state.gfxCtx);
        Matrix_Scale(item->scale, item->scale, item->scale, MTXMODE_APPLY);
        Matrix_RotateY(Spin(play), MTXMODE_APPLY);
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, item->opaque, 0, G_DL_PUSH);
        CLOSE_DISPS(play->state.gfxCtx);
    } else {
        entry->drawFunc(play, entry);
    }
    Matrix_Pop();
    NeiGi_DrawEffects(play, *item, upgraded);
    if (upgraded && item->translucent != nullptr) {
        // Composite the crystal skin over its contained energy, using the same pose.
        OPEN_DISPS(play->state.gfxCtx);
        Matrix_Push();
        Matrix_Scale(item->scale, item->scale, item->scale, MTXMODE_APPLY);
        Matrix_RotateY(Spin(play), MTXMODE_APPLY);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, item->translucent, 0, G_DL_PUSH);
        Matrix_Pop();
        CLOSE_DISPS(play->state.gfxCtx);
    }
    Matrix_Pop();
    return true;
}

extern "C" bool NeiGi_Draw(PlayState* play, GetItemEntry* entry) {
    return NeiGi_DrawImpl(play, entry, false);
}

extern "C" bool NeiGi_DrawShop(PlayState* play, GetItemEntry* entry) {
    return NeiGi_DrawImpl(play, entry, true);
}

#ifdef COMBO_BUILD
#include "ComboItemDrawABI.h"
#include "ComboExport.h"

static bool NeiGi_FillCrossGameInfo(const Presentation& item, CwItemDrawInfo* out) {
    if (!out || !HasResource(item.opaque) || (item.translucent && !HasResource(item.translucent))) {
        return false;
    }
    out->dlists[0] = item.opaque;
    out->dlistCount = 1;
    out->xluStartIndex = -1;
    if (item.translucent) {
        out->dlists[1] = item.translucent;
        out->dlistCount = 2;
        out->xluStartIndex = 1;
    }
    out->scale = item.scale;
    out->drawKind = CW_DRAW_KIND_NEI_GI;
    out->neiEffect = static_cast<int32_t>(item.effect);
    out->neiEffectCenter[0] = item.effectCenter.x;
    out->neiEffectCenter[1] = item.effectCenter.y;
    out->neiEffectCenter[2] = item.effectCenter.z;
    out->neiSomariaUpgrade = item.draw == Randomizer_DrawCaneSomariaUpgrade;
    return true;
}

extern "C" int32_t NeiGi_DescribeEntry(const GetItemEntry* entry, CwItemDrawInfo* out) {
    if (!entry || !entry->drawFunc)
        return 0;
    for (const auto& item : kPresentations) {
        if (item.draw == entry->drawFunc)
            return NeiGi_FillCrossGameInfo(item, out);
    }
    return 0;
}

// Concrete presentation lookup for MM's native NEI items: never resolve through OoT's save.
extern "C" COMBO_EXPORT int32_t OOT_GetNeiGiDrawInfo(const char* slug, CwItemDrawInfo* out) {
    if (!slug || !out)
        return 0;
    constexpr char prefix[] = "__OTR__objects/nei_gi_redesign/";
    for (const auto& item : kPresentations) {
        if (!item.opaque)
            continue;
        const char* name = item.opaque + sizeof(prefix) - 1;
        const size_t length = std::strlen(slug);
        if (std::strncmp(name, slug, length) == 0 && std::strcmp(name + length, "/gi_dl") == 0) {
            *out = CwItemDrawInfo{};
            return NeiGi_FillCrossGameInfo(item, out);
        }
    }
    return 0;
}
#endif
