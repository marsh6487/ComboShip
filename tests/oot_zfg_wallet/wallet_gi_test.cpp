#include "ComboItemDrawABI.h"
#include "global.h"
#include "soh/Enhancements/randomizer/WalletGi.h"
#include <cassert>
#include <cstring>

static PlayState play;
static GraphicsContext graphics;
static bool effects;
static int models;
static int shimmers;
static int overflowDraws;
static int seenModel;
static uint8_t seenTint[4];

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return std::strcmp(key, CVAR_NEI_GI_EFFECTS) == 0 ? effects : fallback;
}

namespace ComboZfgWallet {
static void Draw(PlayState* current, const char* owner) {
    assert(current == &play && current->state.gfxCtx == &graphics && std::strcmp(owner, "oot") == 0);
    ++models;
    seenModel = GID_WALLET_GIANT;
    ++overflowDraws;
}
} // namespace ComboZfgWallet

extern "C" void GetItem_Draw(PlayState* current, s16 drawId) {
    assert(current == &play);
    ++models;
    seenModel = drawId;
}

extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState* current, const uint8_t color[4], const char*) {
    assert(current == &play && models == 1);
    ++shimmers;
    std::memcpy(seenTint, color, 4);
}

static s32 GetItem_GetDrawTableEntry(s32 id, void** lists, s32 max, s32* xluStart, f32* scale, s32* kind, uint8_t*) {
    assert(id == GID_WALLET_ADULT || id == GID_WALLET_GIANT);
    assert(max == 8);
    for (int i = 0; i < max; ++i)
        lists[i] = const_cast<char*>("__OTR__wallet_dlist");
    *xluStart = -1;
    *scale = 0;
    *kind = CW_DRAW_KIND_SIMPLE;
    return 8;
}

#include "wallet_gi_binding.inc"

enum class ComboForeignResolve { Unknown, Ok };
static ComboForeignResolve Validate(const CwItemDrawInfo& raw) {
    const int rcStatic = 1;
#include "wallet_gi_receive.inc"
    return ComboForeignResolve::Ok;
}

static void DrawForeign(PlayState* play, int kind) {
    switch (kind) {
#include "wallet_gi_receive_draw.inc"
        default:
            assert(false);
    }
}

int main() {
    play.state.gfxCtx = &graphics;
    const int identities[CW_WALLET_COUNT] = {
        RG_CHILD_WALLET, ITEM_WALLET_ADULT, ITEM_WALLET_GIANT, RG_TYCOON_WALLET, RG_ZFG_WALLET, RG_WALLET_INF,
    };
    for (int tier = 0; tier < CW_WALLET_COUNT; ++tier) {
        GetItemEntry entry{};
        entry.drawItemId = identities[tier];
        entry.drawModIndex = tier == CW_WALLET_ADULT || tier == CW_WALLET_GIANT ? MOD_NONE : MOD_RANDOMIZER;
        entry.gid = tier <= CW_WALLET_ADULT ? GID_WALLET_ADULT : GID_WALLET_GIANT;
        entry.drawFunc = Randomizer_DrawWallet;
        entry.getItemId = RG_ICE_TRAP; // Cover an award whose actual item differs
                                       // from its drawn wallet tier.

        for (bool enabled : { false, true }) {
            effects = enabled;
            models = shimmers = overflowDraws = 0;
            uint8_t expected[4];
            assert(ComboWalletGi_ShimmerColor(tier, expected));
            Randomizer_DrawWallet(&play, &entry);
            assert(models == 1 && shimmers == enabled);
            assert(seenModel == entry.gid && overflowDraws == (tier == CW_WALLET_ZFG));
            if (enabled)
                assert(std::memcmp(expected, seenTint, 4) == 0);

            CwItemDrawInfo recipe{};
            assert(NeiGi_DescribeWallet(&entry, &recipe));
            assert(recipe.stateDependent == 2);
            assert(recipe.itemShimmer == (enabled ? CW_ITEM_SHIMMER_TINTED : CW_ITEM_SHIMMER_NONE));
            if (enabled)
                assert(std::memcmp(expected, recipe.itemShimmerColor, 4) == 0);
            if (tier != CW_WALLET_ZFG) {
                assert(recipe.drawKind == CW_DRAW_KIND_SIMPLE && recipe.dlistCount == 8);
                continue;
            }

            assert(recipe.drawKind == CW_DRAW_KIND_ZFG_WALLET && recipe.dlistCount == 0 && recipe.opCount == 0);
            assert(std::strcmp(recipe.resolvedName, "ZFG's Wallet") == 0);
            assert(Validate(recipe) == ComboForeignResolve::Ok);
            models = overflowDraws = 0;
            DrawForeign(&play, recipe.drawKind);
            assert(models == 1 && seenModel == GID_WALLET_GIANT && overflowDraws == 1);
            recipe.dlistCount = 1;
            assert(Validate(recipe) == ComboForeignResolve::Unknown);
            recipe.dlistCount = 0;
            recipe.opCount = 1;
            assert(Validate(recipe) == ComboForeignResolve::Unknown);
        }
    }
}
