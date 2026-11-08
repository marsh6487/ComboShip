#pragma once

#include <nlohmann/json.hpp>
#include "Rando/Rando.h"
#include <set>
#include <string_view>
#include <libultraship/bridge/consolevariablebridge.h>
#ifdef COMBO_BUILD
#include "Rando/MiscBehavior/MiscBehavior.h"
#endif

namespace Rando {
struct RpgHintDefinition {
    RandoItemId item;
    std::string_view name;
    const char* setting;
    ComboRpgStat stat;
};

inline constexpr RpgHintDefinition RpgHintDefinitions[] = {
    { RI_HINT_MAGIC_STAT_UPGRADE, "Magic Stat Upgrade", "gRandoSettings.MagicStatUpgrade", COMBO_RPG_MAGIC },
    { RI_HINT_DEFENSE_UPGRADE, "Defense Upgrade", "gRandoSettings.DefenseUpgrade", COMBO_RPG_DEFENSE },
    { RI_HINT_SPEED_UPGRADE, "Speed Upgrade", "gRandoSettings.SpeedUpgrade", COMBO_RPG_SPEED },
    { RI_HINT_POWER_UPGRADE, "Power Upgrade", "gRandoSettings.PowerUpgrade", COMBO_RPG_POWER },
    { RI_HINT_CRAWL_SPEED_UPGRADE, "Crawl Speed Upgrade", "gRandoSettings.CrawlSpeedUpgrade", COMBO_RPG_CRAWL },
    { RI_HINT_CLIMB_SPEED_UPGRADE, "Climb Speed Upgrade", "gRandoSettings.ClimbSpeedUpgrade", COMBO_RPG_CLIMB },
    { RI_HINT_PUSH_SPEED_UPGRADE, "Push Speed Upgrade", "gRandoSettings.PushSpeedUpgrade", COMBO_RPG_PUSH },
};

inline const RpgHintDefinition* FindRpgHintDefinition(RandoItemId item) {
    for (const auto& definition : RpgHintDefinitions) {
        if (definition.item == item)
            return &definition;
    }
    return nullptr;
}

inline RandoItemId RpgHintIdentityFromName(std::string_view name) {
    for (const auto& definition : RpgHintDefinitions) {
        if (name == definition.name)
            return definition.item;
    }
    return RI_UNKNOWN;
}

inline bool IsRpgHintUpgrade(RandoItemId item) {
    return FindRpgHintDefinition(item) != nullptr;
}

// Stable selector identity is independent of whether a stat still benefits
// Link. Saria must not spend a consumable hint on a disabled or capped stat.
// Desired Items intentionally retains OoT's explicit raw-copy wish behavior.
inline bool RpgHintUseful(RandoItemId item) {
    const auto* definition = FindRpgHintDefinition(item);
    if (!definition)
        return true;
    if (!Combo_IsCombinedGame())
        return false;
    const auto& state = gSaveContext.save.shipSaveInfo.nei.comboRpg;
    const int stat = definition->stat;
    // Legacy paired seeds without mirrored rules retain their existing hints;
    // unknown metadata is not evidence that an upgrade is disabled or capped.
    if (!(state.knownMask & (1u << stat)))
        return true;
    return ComboRpgState_Enabled(&state, stat) && state.level[stat] < ComboRpgState_Required(&state, stat);
}

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
        if (foreign && !foreign->trap && foreign->itemGame == ComboRando::GAME_OOT) {
            auto upgrade = RpgHintIdentityFromName(foreign->itemName);
            if (upgrade != RI_UNKNOWN)
                return upgrade;
        }
    }
#endif
    return item;
}

inline bool MagicHintAvailable(RandoItemId item, const std::set<RandoItemId>& pool) {
    if (const auto* definition = FindRpgHintDefinition(item)) {
        // This is the seed-configuration UI, which also runs at the title
        // screen. Read configured OoT pool options rather than an unrelated
        // currently loaded save's mirrored RPG rules.
        return Combo_IsCombinedGame() && CVarGetInteger(definition->setting, 0) != 0;
    }
    // A generic progressive wish also represents pools containing concrete
    // tiers, without offering a specific missing tier merely because another
    // concrete tier is present.
    if (HintProgressiveFamily(item) == item) {
        for (auto pooled : pool) {
            if (HintProgressiveFamily(pooled) == item)
                return true;
        }
    }
    if (HintProgressiveFamily(item) != item && pool.count(HintProgressiveFamily(item)))
        return true;
    return pool.count(item) != 0;
}
} // namespace Rando
