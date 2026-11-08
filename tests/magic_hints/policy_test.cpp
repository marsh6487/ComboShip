#include "Rando/MagicHints.h"
#include <cassert>
#include <algorithm>
std::map<RandoItemId, Rando::StaticData::RandoStaticItem> Rando::StaticData::Items;
#include "candidates.inc"
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
static ComboRando::ForeignItem foreignItem;
const ComboRando::ForeignItem* Rando::MiscBehavior::MM_LookupForeign(RandoCheckId) { return &foreignItem; }
#endif
SaveContext gSaveContext;
std::map<RandoCheckId, Rando::StaticData::RandoStaticCheck> Rando::StaticData::Checks;
static RandoItemId nextTier = RI_SINGLE_MAGIC;
RandoItemId Rando::ConvertItem(RandoItemId item, RandoCheckId check) { return nextTier; }
#include "placement.inc"
static bool Select(RandoItemId wish, RandoItemId raw, RandoItemId itemId) {
    RandoCheckId randoCheckId = RC_UNKNOWN;
    RANDO_SAVE_CHECKS[randoCheckId].randoItemId = raw;
    std::vector<RandoItemId> priorityItems{wish};
    std::unordered_map<RandoItemId, RandoCheckId> priorityItemChecks;
    std::vector<RandoCheckId> otherChecks;
    do {
#include "selection.inc"
    } while (false);
    return priorityItemChecks.count(wish) || !otherChecks.empty();
}
int main() {
    using namespace Rando;
    const RandoItemId hallmark[] = { RI_PROGRESSIVE_MAGIC, RI_SINGLE_MAGIC, RI_DOUBLE_MAGIC,
        RI_HINT_MAGIC_STAT_UPGRADE, RI_PROGRESSIVE_SWORD, RI_PROGRESSIVE_BOW,
        RI_PROGRESSIVE_BOMB_BAG, RI_PROGRESSIVE_WALLET, RI_PROGRESSIVE_LULLABY, RI_DOUBLE_DEFENSE, RI_SWORD_GILDED, RI_QUIVER_50, RI_BOMB_BAG_40, RI_WALLET_TYCOON };
    for (auto id : hallmark) {
        StaticData::Items[id] = {id, "test", "", "test", IsHallmarkHintUpgrade(id) ? RITYPE_LESSER : RITYPE_MAJOR,
                                ITEM_NONE, GI_NONE, GID_NONE};
    }
    StaticData::Items[RI_JUNK].randoItemType = RITYPE_JUNK;
    StaticData::Items[RI_COMBO_FOREIGN].randoItemType = RITYPE_JUNK;
    assert(!Select(RI_PROGRESSIVE_MAGIC, RI_PROGRESSIVE_MAGIC, RI_JUNK));
    assert(Select(RI_PROGRESSIVE_MAGIC, RI_PROGRESSIVE_MAGIC, RI_SINGLE_MAGIC));
    RandoCheckId target = static_cast<RandoCheckId>(1);
    StaticData::Checks[target] = {};
    RANDO_SAVE_CHECKS[target].randoItemId = RI_PROGRESSIVE_MAGIC;
    assert(FindMultiItemPlacement(RI_PROGRESSIVE_MAGIC).size() == 1);
    assert(FindMultiItemPlacement(RI_SINGLE_MAGIC).size() == 1);
    assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC).empty());
    nextTier = RI_DOUBLE_MAGIC;
    assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC).size() == 1);
    nextTier = RI_JUNK;
    assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC).empty());
    auto candidates = GetSariaPriorityItemCandidates();
    for (auto id : hallmark) assert(std::find(candidates.begin(), candidates.end(), id) != candidates.end());
    assert(MagicHintMatches(RI_PROGRESSIVE_MAGIC, RI_SINGLE_MAGIC));
    assert(MagicHintMatches(RI_PROGRESSIVE_MAGIC, RI_DOUBLE_MAGIC));
    assert(!MagicHintMatches(RI_DOUBLE_MAGIC, RI_PROGRESSIVE_MAGIC));
    assert(!MagicHintMatches(RI_HINT_MAGIC_STAT_UPGRADE, RI_PROGRESSIVE_MAGIC));
    std::set<RandoItemId> pool{RI_PROGRESSIVE_MAGIC, RI_BOW, RI_HOOKSHOT};
    assert(MagicHintAvailable(RI_SINGLE_MAGIC, pool));
    assert(MagicHintAvailable(RI_DOUBLE_MAGIC, pool));
    assert(!MagicHintAvailable(RI_PROGRESSIVE_SWORD, pool));
#ifdef COMBO_BUILD
    assert(MagicHintAvailable(RI_HINT_MAGIC_STAT_UPGRADE, pool));
    foreignItem.itemGame = ComboRando::GAME_OOT;
    foreignItem.itemName = "Magic Stat Upgrade";
    assert(MagicHintIdentity(RI_COMBO_FOREIGN, RC_UNKNOWN) == RI_HINT_MAGIC_STAT_UPGRADE);
    assert(Select(RI_HINT_MAGIC_STAT_UPGRADE, RI_COMBO_FOREIGN, RI_COMBO_FOREIGN));
    RANDO_SAVE_CHECKS[target].randoItemId = RI_COMBO_FOREIGN;
    assert(FindMultiItemPlacement(RI_HINT_MAGIC_STAT_UPGRADE).size() == 1);
    foreignItem.trap = true;
    assert(!Select(RI_HINT_MAGIC_STAT_UPGRADE, RI_COMBO_FOREIGN, RI_COMBO_FOREIGN));
    assert(MagicHintIdentity(RI_COMBO_FOREIGN, RC_UNKNOWN) == RI_COMBO_FOREIGN);
    foreignItem.trap = false;
    foreignItem.itemName = "Progressive Magic Meter";
    assert(MagicHintIdentity(RI_COMBO_FOREIGN, RC_UNKNOWN) == RI_COMBO_FOREIGN);
    gComboGameMode = COMBO_GAME_MM;
    assert(!MagicHintAvailable(RI_HINT_MAGIC_STAT_UPGRADE, pool));
#else
    assert(!MagicHintAvailable(RI_HINT_MAGIC_STAT_UPGRADE, pool));
#endif
}
