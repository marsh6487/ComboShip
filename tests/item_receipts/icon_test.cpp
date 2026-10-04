// Execute the production icon selector at its item/catalog boundary. Numeric
// identities reproduce the real RG_DOUBLE_DEFENSE / ITEM_FISH collision.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboSongDrawOOT.h"
constexpr int ITEM_FISH = 0x19, ITEM_HEART_CONTAINER = 0x72;
constexpr int ITEM_MEDALLION_FOREST = 0x66, ITEM_HEART_PIECE_2 = 0x7A;
constexpr int ITEM_ROCS_FEATHER_SKIJER = 0xA0, ICON_SIZE_24 = 0;
struct GetItemEntry { int itemId; };
void* gItemIcons[256]{};
namespace Rando::StaticData {
struct Item {
    RandomizerGet rg;
    std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet*) const {
        return std::make_shared<GetItemEntry>(GetItemEntry{rg == RG_DOUBLE_DEFENSE ? (int)RG_DOUBLE_DEFENSE : 3});
    }
    bool HasCustomIcon() const { return false; }
    const char* GetCustomIcon() const { return nullptr; }
    int GetCustomIconSize() const { return 0; }
};
Item RetrieveItem(RandomizerGet rg) { return {rg}; }
}
/* ICON_SELECTOR */
int main() {
    static_assert(RG_DOUBLE_DEFENSE == ITEM_FISH);
    gItemIcons[ITEM_FISH] = (void*)"__OTR__textures/icon_item_static/gItemIconBottleFishTex";
    gItemIcons[ITEM_HEART_CONTAINER] = (void*)"__OTR__textures/icon_item_24_static/gQuestIconHeartContainerTex";
    gItemIcons[3] = (void*)"__OTR__textures/icon_item_static/gItemIconBowTex";
    CwItemIconInfo icon{};
    assert(OOT_FillItemIconInfo(RG_DOUBLE_DEFENSE, &icon) == 1);
    assert(std::strstr(icon.path, "HeartContainer") && !std::strstr(icon.path, "Fish"));
    assert(icon.width == 24 && icon.height == 24 && !icon.isIA8);
    icon = {};
    assert(OOT_FillItemIconInfo(RG_FAIRY_BOW, &icon) == 1);
    assert(std::strstr(icon.path, "Bow") && icon.width == 32);
    std::cout << "Double Defense icon uses the heart, not the colliding RG/item ID\n";
}
