// Real native/seed selectors and receipt blitter; game services are boundaries.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
using u8 = uint8_t;
using s16 = int16_t;
#include "mm/include/z64item.h"
#include "mm/2s2h/Rando/Types.h"
#include "mm/assets/2s2h_assets.h"
#include "mm/assets/interface/parameter_static/parameter_static.h"
#include "mm/assets/interface/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "mm/assets/interface/icon_item_field_static/icon_item_field_static.h"
#include "mm/assets/archives/icon_item_static/icon_item_static_yar.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
#include "combo/menu/ComboSongDrawMM.h"
#include "tests/item_receipts/boss_soul_fixtures.h"
/* SOUL_RECEIPT_HELPER */
static const void* gItemIcons[256]{};
static s16 D_801CFF94[250]{};
static std::string stagedPath;
static const void* stagedTexture;
static int stagedWidth;
void Message_StageCustomItemIcon(void* texture, s16 width) {
    stagedPath = static_cast<const char*>(texture);
    stagedTexture = texture;
    stagedWidth = width;
}
void Message_StageCustomItemIconTint(void*, s16, s16, u8, u8, u8, u8) { assert(false); }
namespace ComboRando {
static std::string StripGameSuffix(const char* name) {
    std::string key(name);
    for (const char* suffix : {" (MM)", " (OOT)"})
        if (key.ends_with(suffix)) key.resize(key.size() - std::strlen(suffix));
    return key;
}
}
namespace Rando::StaticData {
struct Item { const char* name; int itemId = ITEM_NONE, getItemId = GI_NONE; };
static std::map<RandoItemId, Item> Items = {
#define SOUL_ROW(mm, oot, name, color) {mm, {name}},
    BOSS_SOUL_RECEIPT_FIXTURES(SOUL_ROW)
#undef SOUL_ROW
};
const char* GetIconTexturePath(RandoItemId);
/* NATIVE_ICON_SELECTORS */
}
#define COMBO_EXPORT
/* SEED_ICON_SELECTOR */

struct Gfx {};
static std::string loadedPath;
static int loadedFormat, loadedBits, loadedWidth, loadedHeight;
static void RecordTexture(const char* path, int format, int bits, int width, int height) {
    loadedPath = path;
    loadedFormat = format;
    loadedBits = bits;
    loadedWidth = width;
    loadedHeight = height;
}
#define gDPPipeSync(pkt) ((void)(pkt))
#define gDPSetCombineMode(pkt, ...) ((void)(pkt))
#define gDPSetPrimColor(pkt, min, lod, r, g, b, alpha) ((void)(pkt), (void)(alpha))
#define gDPSetCombineLERP(pkt, ...) ((void)(pkt))
#define gSPTextureRectangle(pkt, x, y, ...) ((void)(pkt), (void)(x), (void)(y))
#define gDPLoadTextureBlock(pkt, path, fmt, bits, w, h, ...) \
    ((void)(pkt), RecordTexture(path, fmt, bits, w, h))
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_IA 3
#define G_IM_SIZ_32b 32
#define G_IM_SIZ_8b 8
#include "combo/menu/ComboItemReceiptRender.h"
/* ASSET_METADATA */

static void NativeRenderAndCheck() {
    struct Message { int unk12014 = 32, unk12016 = 32, textColorAlpha = 255; const char* textboxSegment[1]; } context;
    auto* msgCtx = &context;
    context.textboxSegment[0] = stagedPath.c_str();
    const int sMsgCustomIconWidth = stagedWidth, sMsgCustomIconHeight = stagedWidth;
    const bool sMsgCustomIconIA8 = false;
    Gfx commands[16];
    auto* gfx = commands;
    int textureStep;
#define TEXTBOX_SEG_ICON 0
    /* NATIVE_MESSAGE_RENDERER */
#undef TEXTBOX_SEG_ICON
    assert(textureStep > 0 && gfx > commands);
    const auto metadata = assetMetadata.at(loadedPath);
    assert(loadedFormat == 0 && loadedBits == metadata.bits &&
           loadedWidth == metadata.width && loadedHeight == metadata.height);
}

static void RenderAndCheck(CwItemIconInfo icon, const char* owner) {
    CwItemReceiptPresentation receipt{};
    receipt.singleBox = 1;
    assert(ComboReceipt_CopyIcon(&receipt, &icon, owner));
    CwItemReceiptLayout layout{};
    layout.iconWidth = layout.iconHeight = 24;
    Gfx commands[16];
    assert(ComboReceipt_DrawIcon(commands, &receipt, &layout, 60, 60, 255) > commands);
    assert(loadedPath.starts_with(std::string("__OTR__@") + owner + ":"));
    std::string local = "__OTR__" + loadedPath.substr(loadedPath.find(':') + 1);
    const auto metadata = assetMetadata.at(local);
    assert(loadedFormat == 0 && loadedBits == metadata.bits &&
           loadedWidth == metadata.width && loadedHeight == metadata.height &&
           "receipt command format/dimensions differ from the real texture asset");
}

int main() {
    gItemIcons[ITEM_MASK_CAPTAIN] = gItemIconCaptainsHatTex;
    gItemIcons[ITEM_REMAINS_ODOLWA] = gItemIconOdolwasRemainsTex;
    gItemIcons[ITEM_REMAINS_GOHT] = gItemIconGohtsRemainsTex;
    gItemIcons[ITEM_REMAINS_GYORG] = gItemIconGyorgsRemainsTex;
    gItemIcons[ITEM_REMAINS_TWINMOLD] = gItemIconTwinmoldsRemainsTex;
    const auto mapBefore = std::string(gDungeonMapSkullTex);
    for (const auto& [id, item] : Rando::StaticData::Items) {
        CwItemIconInfo icon{};
        assert(MM_GetSeedItemIconInfo(item.name, &icon) == 1);
        const bool generic = id == RI_SOUL_BOSS_MAJORA ||
            (id >= RI_SOUL_OOT_BOSS_BARINADE && id <= RI_SOUL_OOT_BOSS_VOLVAGIA);
        if (generic) {
            assert(icon.path == gItemIcons[ITEM_MASK_CAPTAIN] &&
                   "export must retain the aligned native resource pointer");
            assert((reinterpret_cast<uintptr_t>(icon.path) & 1) == 0);
        }
        // Exercise the actual receipt command against the XML asset facts first:
        // the broken selector advertises a 16x16 RGBA16 skull as 32x32 RGBA32.
        RenderAndCheck(icon, "mm");
        assert(icon.width == 32 && icon.height == 32 && !icon.isIA8 && !icon.hasColor);
        stagedPath.clear();
        assert(Rando::StaticData::GetIconForZMessage(id) == 0xF5);
        assert(stagedPath == icon.path && stagedWidth == icon.width &&
               "native and exported boss soul receipts disagree on their icon");
        NativeRenderAndCheck();
        CwItemIconInfo native{stagedPath.c_str(), stagedWidth, stagedWidth, 0, 0, {}};
        RenderAndCheck(native, "mm");
        if (generic) {
            assert(stagedTexture == gItemIcons[ITEM_MASK_CAPTAIN] &&
                   "direct receipt must retain the aligned native resource pointer");
            assert(!std::strcmp(icon.path, gItemIconCaptainsHatTex));
            assert(mapBefore == Rando::StaticData::GetIconTexturePath(id) &&
                   "receipt correction changed notification/map skull artwork");
        }
        icon = {};
        assert(MM_GetSeedItemIconInfo((std::string(item.name) + " (MM)").c_str(), &icon));
        RenderAndCheck(icon, "mm");
    }
    // The same generic skull is also the established enemy-soul receipt family.
    for (int id = RI_SOUL_ENEMY_ALIEN; id <= RI_SOUL_ENEMY_WOLFOS; ++id) {
        stagedPath.clear();
        assert(Rando::StaticData::GetIconForZMessage(static_cast<RandoItemId>(id)) == 0xF5);
        assert(stagedPath == gItemIconCaptainsHatTex && stagedWidth == 32);
    }
    assert(mapBefore == gDungeonMapSkullTex);
    std::cout << "PASS 14 MM-native/imported boss soul direct/exported receipt icons: actual asset format, dimensions, routed owner and renderer; native skull toast/map art unchanged\n";
}
