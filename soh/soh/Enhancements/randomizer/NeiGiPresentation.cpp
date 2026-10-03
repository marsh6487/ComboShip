#include "NeiGiPresentation.h"
#include "NeiGiEffectPolicy.h"
#include "NeiGiEnergyTexture.h"
#include "NeiGiRender.h"
#include "NeiGiShopFit.h"
#include <algorithm>
#include <cstring>
#include <iterator>
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
    int32_t identity = 0; // distinct awards sharing one legacy callback
    bool alwaysShimmer = false;
    int32_t nativeGid = -1;
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
    { Randomizer_DrawElementalWand,
      GI_PATH("elemental_wand"),
      nullptr,
      1.f,
      Kind::Light,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("sand_rod"),
      nullptr,
      1.f,
      Kind::Sand,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_SAND_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("tornado_rod"),
      nullptr,
      1.f,
      Kind::Tornado,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_TORNADO_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("water_rod"),
      nullptr,
      1.f,
      Kind::Water,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_WATER_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("meteor_rod"),
      nullptr,
      1.f,
      Kind::Meteor,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_METEOR_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("storm_rod"),
      nullptr,
      1.f,
      Kind::Storm,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_STORM_ROD,
      false,
      -1 },
    { Randomizer_DrawElementalWand,
      GI_PATH("shadow_scepter"),
      nullptr,
      1.f,
      Kind::Shadow,
      { 6.1327f, 24.5970f, 0.0000f },
      { .85f, 14.f },
      RG_WAND_SHADOW_SCEPTER,
      false,
      -1 },
    { Randomizer_DrawExtDivineShield,
      GI_PATH("divine_shield"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtSheikahShield,
      GI_PATH("sheikah_shield"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtShieldOfIkana,
      GI_PATH("shield_of_ikana"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtMagicCape,
      GI_PATH("magic_cape"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtSpiritBreastplate,
      GI_PATH("spirit_breastplate"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtSagesTunic,
      GI_PATH("sages_tunic"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtChampionsTunic,
      GI_PATH("champions_tunic"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtPegasusAnklet,
      GI_PATH("pegasus_anklet"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtTrident,
      GI_PATH("trident"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtClimbBoots,
      GI_PATH("climb_boots"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtRocBoots,
      GI_PATH("roc_boots"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtCaneOfByrna,
      GI_PATH("cane_of_byrna"),
      nullptr,
      1.f,
      Kind::CaneBlue,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtFourSword,
      GI_PATH("four_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawExtPendantOfMemories,
      GI_PATH("pendant_of_memories"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiSheikahSlate,
      GI_PATH("sheikah_slate"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneBomb,
      GI_PATH("slate_bomb"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneMasterCycle,
      GI_PATH("slate_master_cycle"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneStasis,
      GI_PATH("slate_stasis"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneCryonis,
      GI_PATH("slate_cryonis"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawSlateRuneSensor,
      GI_PATH("slate_sensor"),
      nullptr,
      1.f,
      Kind::Slate,
      { 0.0000f, 0.0000f, 4.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiPhantomHourglass,
      GI_PATH("phantom_hourglass"),
      GI_XLU("phantom_hourglass"),
      1.f,
      Kind::Hourglass,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiShadowCrystal,
      GI_PATH("shadow_crystal"),
      GI_XLU("shadow_crystal"),
      1.f,
      Kind::DarkCrystal,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawNeiRodOfSeasons,
      GI_PATH("rod_of_seasons"),
      nullptr,
      1.f,
      Kind::SeasonCycle,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      false,
      -1 },
    { Randomizer_DrawProgressiveKokiriSword,
      GI_PATH("kokiri_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawProgressiveMasterSword,
      GI_PATH("master_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawMasterSword,
      GI_PATH("master_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawProgressiveBGS,
      GI_PATH("biggoron_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawRazorSword,
      GI_PATH("razor_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawGildedSword,
      GI_PATH("gilded_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawTrueMasterSword,
      GI_PATH("true_master_sword"),
      nullptr,
      1.f,
      Kind::SwordAura,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawGreatFairySword,
      GI_PATH("great_fairy_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { Randomizer_DrawIronKnuckleAxe,
      GI_PATH("iron_knuckle_axe"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      -1 },
    { nullptr,
      GI_PATH("kokiri_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      GID_SWORD_KOKIRI },
    { nullptr,
      GI_PATH("biggoron_sword"),
      nullptr,
      1.f,
      Kind::Neutral,
      { 0.0000f, 0.0000f, 0.0000f },
      { .85f, 14.f },
      0,
      true,
      GID_SWORD_BGS },
};
#undef GI_PATH
#undef GI_XLU

const Presentation* FindPresentation(const GetItemEntry* entry) {
    if (!entry)
        return nullptr;
    const Presentation* fallback = nullptr;
    for (const auto& candidate : kPresentations) {
        if (!entry->drawFunc && !candidate.draw && candidate.nativeGid == entry->gid)
            return &candidate;
        if (!entry->drawFunc || candidate.draw != entry->drawFunc)
            continue;
        if (candidate.identity && candidate.identity == entry->drawItemId)
            return &candidate;
        if (!candidate.identity)
            fallback = &candidate;
    }
    return fallback;
}

struct SeasonPresentation {
    CustomDrawFunc draw;
    const char* slug;
};
constexpr SeasonPresentation kSeasons[] = {
    { Randomizer_DrawSeasonSpring, "season_spring" },
    { Randomizer_DrawSeasonSummer, "season_summer" },
    { Randomizer_DrawSeasonAutumn, "season_autumn" },
    { Randomizer_DrawSeasonWinter, "season_winter" },
};

int SeasonForDraw(CustomDrawFunc draw) {
    for (size_t i = 0; i < std::size(kSeasons); ++i)
        if (kSeasons[i].draw == draw)
            return static_cast<int>(i) + 1;
    return 0;
}

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

bool HasSelectedSword(const Presentation& item, bool altAssets, bool (*available)(const char*)) {
    if (!item.alwaysShimmer || !altAssets)
        return false;
    const char* selected = nullptr;
    const char* fire = nullptr;
    if (std::strstr(item.opaque, "/kokiri_sword/") || std::strstr(item.opaque, "/razor_sword/") ||
        std::strstr(item.opaque, "/gilded_sword/")) {
        selected = "__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL";
        fire = "__OTR__objects/din_fire_sword/progressive/child/SwordDL";
    } else if (std::strstr(item.opaque, "/master_sword/") || std::strstr(item.opaque, "/true_master_sword/")) {
        selected = "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL";
        fire = "__OTR__objects/din_fire_sword/progressive/adult/SwordDL";
    } else if (std::strstr(item.opaque, "/biggoron_sword/") || std::strstr(item.opaque, "/great_fairy_sword/")) {
        selected = "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL";
        fire = "__OTR__objects/din_fire_sword/progressive/bgs/SwordDL";
    }
    // Preserve the selected standalone weapon pack and the protected Din GI.
    return selected &&
           (available(selected) || (CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0) && available(fire)));
}
} // namespace

// OPEN_DISPS declares interpolation callbacks with the enclosing C linkage.
extern "C" {
#include "NeiGiMeshRenderer.inc"

static void NeiGi_DrawEffects(PlayState* play, const Presentation& item, bool upgraded) {
    if (upgraded && item.effect == Kind::SeasonCycle)
        NeiGi_DrawSeasonOverlay(play, 5, nullptr);
    const bool shimmer = CVarGetInteger(CVAR_NEI_GI_EFFECTS, 0) != 0;
    const bool energy =
        upgraded && (NeiGi::IsRod(item.effect) || NeiGi::IsSpell(item.effect) || NeiGi::IsSpecial(item.effect));
    if (!shimmer && !energy && !item.alwaysShimmer)
        return;
    Matrix_Push();
    Matrix_RotateY(Spin(play), MTXMODE_APPLY);
    const auto camera = NeiGi_CameraBasis(play);
    if (energy) {
        Matrix_Push();
        Matrix_Translate(item.effectCenter.x, item.effectCenter.y, item.effectCenter.z, MTXMODE_APPLY);
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(item.effect, camera), item.effect);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(item.effect, play->gameplayFrames, camera));
        if (NeiGi::IsSpecial(item.effect))
            NeiGi_DrawMesh(play, NeiGi::SampleSpecial(item.effect, play->gameplayFrames, camera));
        Matrix_Pop();
    }
    if (shimmer || item.alwaysShimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, camera, item.effect));
    Matrix_Pop();
}
}

static bool NeiGi_DrawImpl(PlayState* play, GetItemEntry* entry, bool shop) {
    if (play == nullptr || entry == nullptr)
        return false;
    // Intrinsic season weather precedes archive/model selection and optional effects.
    if (const int season = SeasonForDraw(entry->drawFunc)) {
        NeiGi_DrawSeasonOverlay(play, season, nullptr);
        return true;
    }
    const Presentation* item = FindPresentation(entry);
    if (item == nullptr)
        return false;
    const bool selectedSword = HasSelectedSword(*item, ResourceMgr_IsAltAssetsEnabled(), HasResource);
    // Queue stable paths for the interpreter. Loading through the legacy GBI wrapper
    // here would evict/reload base resources on each draw when Alt Assets is enabled.
    // Archive presence checks preserve the original model if a required pass is absent.
    const bool upgraded = !selectedSword && HasResource(item->opaque) &&
                          (!item->translucent || HasResource(item->translucent));
    if (!upgraded && !entry->drawFunc && !item->alwaysShimmer)
        return false;
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
    } else if (entry->drawFunc) {
        entry->drawFunc(play, entry);
    } else {
        GetItem_Draw(play, entry->gid);
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

static bool NeiGi_FillSeasonInfo(int season, CwItemDrawInfo* out) {
    if (!out)
        return false;
    out->drawKind = CW_DRAW_KIND_SEASON_GI;
    out->neiEffect = season;
    out->dlistCount = 0;
    out->xluStartIndex = -1;
    return true;
}

static bool NeiGi_FillCrossGameInfo(const Presentation& item, CwItemDrawInfo* out) {
    if (!out ||
        HasSelectedSword(item, OOT_NeiAltAssetsEnabled(),
                         [](const char* path) { return OOT_NeiResourceExists(path) != 0; }) ||
        !OOT_NeiResourceExists(item.opaque) || (item.translucent && !OOT_NeiResourceExists(item.translucent))) {
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
    if (item.alwaysShimmer)
        out->itemShimmer = 1;
    if (item.alwaysShimmer) {
        const uint8_t ordinary[4] = { 220, 225, 240, 255 };
        std::memcpy(out->itemShimmerColor, ordinary, 4);
    }
    return true;
}

extern "C" int32_t NeiGi_DescribeEntry(const GetItemEntry* entry, CwItemDrawInfo* out) {
    if (entry)
        if (const int season = SeasonForDraw(entry->drawFunc))
            return NeiGi_FillSeasonInfo(season, out);
    const auto* item = FindPresentation(entry);
    return item ? NeiGi_FillCrossGameInfo(*item, out) : 0;
}

// Concrete presentation lookup for MM's native NEI items: never resolve through OoT's save.
extern "C" COMBO_EXPORT int32_t OOT_GetNeiGiDrawInfo(const char* slug, CwItemDrawInfo* out) {
    if (!slug || !out)
        return 0;
    for (size_t i = 0; i < std::size(kSeasons); ++i) {
        if (std::strcmp(slug, kSeasons[i].slug) == 0) {
            *out = CwItemDrawInfo{};
            return NeiGi_FillSeasonInfo(static_cast<int>(i) + 1, out);
        }
    }
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
