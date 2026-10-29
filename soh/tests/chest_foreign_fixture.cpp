#include <string>
#include <unordered_map>
#include <libultraship/libultra.h>
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/Enhancements/randomizer/item_category_adj.h"
#include "tests/test_require.h"

#ifdef COMBO_BUILD
// Exact ForeignItem and GameId declarations are extracted from CrossForeign.h.
#include "chest_foreign_types_production.inc"
static std::unordered_map<RandomizerCheck, ComboRando::ForeignItem> foreignItems;

const ComboRando::ForeignItem* OOT_LookupForeignByCheck(RandomizerCheck check) {
    REQUIRE(check > RC_UNKNOWN_CHECK && check < RC_MAX);
    const auto found = foreignItems.find(check);
    return found == foreignItems.end() ? nullptr : &found->second;
}

extern "C" void ChestTest_SetForeignCategory(int32_t check, const char* category, bool trap) {
    ComboRando::ForeignItem item = {};
    item.itemGame = ComboRando::GAME_MM;
    item.itemName = "fixture reward";
    item.category = category;
    item.trap = trap;
    foreignItems[(RandomizerCheck)check] = item;
}
#endif

// Execute the real C-linkage classifier against native entries and the lookup fixture.
#include "chest_fairy_production.inc"
