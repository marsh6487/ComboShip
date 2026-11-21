#pragma once

#include <libultraship/libultra/gbi.h>
#include <span>
#include <vector>

namespace ComboZfgWallet {
alignas(2) inline constexpr char kClothTexture[] = "__OTR__objects/combo_zfg_wallet/classic/WalletTex";
alignas(2) inline constexpr char kCordTexture[] = "__OTR__objects/combo_zfg_wallet/classic/WalletCordTex";
alignas(2) inline constexpr char kBadgeTexture[] = "__OTR__objects/combo_zfg_wallet/classic/WalletBadgeTex";

struct ClassicLists {
    Gfx* cloth;
    Gfx* badge;
};

// The two textured stock sections are leaf lists. Redirect their texture images
// and adjust the inner gem's scale. A replacement with a different material
// layout continues to draw its original lists.
inline std::vector<Gfx> ClassicTextureList(std::span<const Gfx> source, const char* texture, bool gemCoordinates = false) {
    if (source.empty() || source.size() > 64)
        return {};
    std::vector<Gfx> result;
    result.reserve(source.size());
    bool boundTexture = false;
    bool stockTile = false;
    bool scaledGem = false;
    for (size_t i = 0; i < source.size();) {
        Gfx command = source[i++];
        switch (command.words.w0 >> 24) {
            case G_SETTIMG_OTR_HASH:
                if (boundTexture || i == source.size() ||
                    (command.words.w0 & 0xFFFFFF) != ((G_IM_FMT_I << 21) | (G_IM_SIZ_16b << 19)))
                    return {};
                command.words.w0 = (uintptr_t(G_SETTIMG_OTR_FILEPATH) << 24) | (command.words.w0 & 0xFFFFFF);
                command.words.w1 = reinterpret_cast<uintptr_t>(texture);
                boundTexture = true;
                ++i;
                break;
            case G_SETTILESIZE:
                stockTile = (command.words.w0 & 0xFFFFFF) == 0 && command.words.w1 == 0x000FC0FC;
                break;
            case G_MARKER:
            case G_VTX_OTR_HASH:
                if (i == source.size())
                    return {};
                result.push_back(command);
                command = source[i++];
                break;
            case G_RDPPIPESYNC:
            case G_RDPLOADSYNC:
            case G_SETOTHERMODE_H:
            case G_SETOTHERMODE_L:
            case G_SETCOMBINE:
            case G_SETTILE:
            case G_LOADBLOCK:
            case G_GEOMETRYMODE:
            case G_TRI1:
            case G_TRI2:
                break;
            case G_TEXTURE:
                if (gemCoordinates) {
                    if (scaledGem || command.words.w1 != 0x13881388)
                        return {};
                    // The gem's render tile halves S and doubles T. This
                    // gives both axes half scale, using more of the atlas.
                    command.words.w1 = 0xFFFF4000;
                    scaledGem = true;
                }
                break;
            case G_ENDDL:
                if (!boundTexture || !stockTile || (gemCoordinates && !scaledGem) || i != source.size())
                    return {};
                result.push_back(command);
                return result;
            default:
                return {};
        }
        result.push_back(command);
    }
    return {};
}
} // namespace ComboZfgWallet
