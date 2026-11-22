#pragma once

#include "ZfgWalletClassicLists.h"
#include "objects/object_gi_purse/object_gi_purse.h"
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include <fast/resource/type/DisplayList.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <algorithm>

extern "C" uint8_t ResourceMgr_FileExists(const char* path);

namespace ComboZfgWallet {
static ClassicLists LoadClassicLists(PlayState* play, const char* owner) {
    if (!ResourceMgr_FileExists(kClothTexture) || !ResourceMgr_FileExists(kCordTexture) ||
        !ResourceMgr_FileExists(kBadgeTexture))
        return {};
    const auto manager = Ship::CrossRMRegistry::GetOrActive(owner);
    if (!manager)
        return {};
    const auto load = [&](const char* path, const char* texture, bool gemCoordinates) {
        const auto list = std::dynamic_pointer_cast<Fast::DisplayList>(manager->LoadResource(path));
        if (!list || list->UCode != ucode_f3dex2)
            return std::vector<Gfx>{};
        return ClassicTextureList(list->Instructions, texture, gemCoordinates);
    };
    const auto cloth = load(gGiWalletDL, kClothTexture, false);
    const auto badge = load(gGiWalletRupeeInnerDL, kBadgeTexture, true);
    if (cloth.empty() || badge.empty() || !NeiGi_CanDrawLayers(play, 1, 36 + cloth.size() + badge.size(), 2))
        return {};
    // Own the copied commands in the same arena as the matrix. This also keeps
    // Alt Assets toggles and archive reloads from changing a submitted draw.
    GraphicsContext* gfx = play->state.gfxCtx;
    const size_t size = (cloth.size() + badge.size()) * sizeof(Gfx);
#ifdef COMBO_ZFG_WALLET_HOST_MM
    auto* memory = static_cast<Gfx*>(GRAPH_ALLOC(gfx, size));
#else
    auto* memory = static_cast<Gfx*>(Graph_Alloc(gfx, size));
#endif
    if (!memory)
        return {};
    std::copy(cloth.begin(), cloth.end(), memory);
    Gfx* next = memory + cloth.size();
    std::copy(badge.begin(), badge.end(), next);
    return { memory, next };
}
} // namespace ComboZfgWallet
