#pragma once

#include <nlohmann/json.hpp>
#include "Rando/Rando.h"
#include <set>
#ifdef COMBO_BUILD
#include "Rando/MiscBehavior/MiscBehavior.h"
#endif

namespace Rando {
inline RandoItemId HintProgressiveFamily(RandoItemId item) {
    switch (item) {
        case RI_SINGLE_MAGIC:
        case RI_DOUBLE_MAGIC:
            return RI_PROGRESSIVE_MAGIC;
        case RI_SWORD_KOKIRI:
        case RI_SWORD_RAZOR:
        case RI_SWORD_GILDED:
            return RI_PROGRESSIVE_SWORD;
        case RI_BOW:
        case RI_QUIVER_40:
        case RI_QUIVER_50:
            return RI_PROGRESSIVE_BOW;
        case RI_BOMB_BAG_20:
        case RI_BOMB_BAG_30:
        case RI_BOMB_BAG_40:
            return RI_PROGRESSIVE_BOMB_BAG;
        case RI_WALLET_ADULT:
        case RI_WALLET_GIANT:
        case RI_WALLET_TYCOON:
            return RI_PROGRESSIVE_WALLET;
        case RI_SONG_LULLABY_INTRO:
        case RI_SONG_LULLABY:
            return RI_PROGRESSIVE_LULLABY;
        default:
            return item;
    }
}
inline bool IsHallmarkHintUpgrade(RandoItemId item) {
    return item == RI_DOUBLE_DEFENSE || HintProgressiveFamily(item) != item;
}
inline bool MagicHintMatches(RandoItemId wish, RandoItemId item) {
    if (wish == item)
        return true;
    // Desired Items scans placed identities; Saria also compares the converted tier.
    return wish == HintProgressiveFamily(item);
}

inline RandoItemId MagicHintIdentity(RandoItemId item, RandoCheckId check) {
#ifdef COMBO_BUILD
    if (item == RI_COMBO_FOREIGN) {
        const auto* foreign = MiscBehavior::MM_LookupForeign(check);
        if (foreign && !foreign->trap && foreign->itemGame == ComboRando::GAME_OOT &&
            foreign->itemName == "Magic Stat Upgrade")
            return RI_HINT_MAGIC_STAT_UPGRADE;
    }
#endif
    return item;
}

inline bool MagicHintAvailable(RandoItemId item, const std::set<RandoItemId>& pool) {
    if (item == RI_HINT_MAGIC_STAT_UPGRADE)
        return Combo_IsCombinedGame();
    if (HintProgressiveFamily(item) != item && pool.count(HintProgressiveFamily(item)))
        return true;
    return pool.count(item) != 0;
}
} // namespace Rando
