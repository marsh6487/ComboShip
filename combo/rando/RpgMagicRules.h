#pragma once

#include "NativeRandoDump.h"
#include "RpgStats.h"
#include <algorithm>
#include <string>
#include <vector>

namespace ComboRando {
// Frozen OoT generation rules, shared by the combined fill and every replay.
// An older/solo dump without this metadata retains ordinary native magic.
struct RpgMagicRules {
    bool enabled = false;
    uint8_t required = 8;
    uint8_t total = 96;
};

inline RpgMagicRules ParseRpgMagicRules(const std::string& ootDumpJson) {
    const auto dump = ParseOptionalStaticDataDump(ootDumpJson, "OOT RPG magic");
    if (!dump.contains("rpgMagic"))
        return {};
    const auto& magic = dump.at("rpgMagic");
    if (!magic.at("enabled").get<bool>())
        return {};
    const int required = magic.at("required").get<int>();
    const int total = magic.at("total").get<int>();
    if (required < 1 || required > 100 || (total != 96 && total != 100))
        throw std::runtime_error("invalid frozen RPG magic rules");
    return { true, static_cast<uint8_t>(required), static_cast<uint8_t>(total) };
}

inline constexpr const char* kRpgMagicItem = "Magic Stat Upgrade";
inline constexpr const char* kRpgMagicOraclePrefix = "__combo_rpg_magic:";

inline bool IsNativeMmMagicItem(const std::string& name) {
    return name == "Progressive Magic" || name == "Power of Magic" || name == "Magic Upgrade";
}

// This private oracle input is never a pool item, placement or pickup. Keeping
// it out of the actual-owned vectors prevents repeated spheres granting tiers.
inline void AddRpgMagicOracleView(const RpgMagicRules& rules, const std::vector<std::string>& ootOwned,
                                  std::vector<std::string>& mmSeen) {
    if (!rules.enabled)
        return;
    const int level = std::min<int>(std::count(ootOwned.begin(), ootOwned.end(), kRpgMagicItem), rules.required);
    mmSeen.push_back(
        kRpgMagicOraclePrefix +
        nlohmann::json({ { "required", rules.required }, { "total", rules.total }, { "level", level } }).dump());
}

inline bool ParseRpgMagicOracleItem(const std::string& item, ComboRpgState& state) {
    const std::string prefix = kRpgMagicOraclePrefix;
    if (item.compare(0, prefix.size(), prefix) != 0)
        return false;
    const auto magic = nlohmann::json::parse(item.substr(prefix.size()));
    const int required = magic.at("required").get<int>();
    const int total = magic.at("total").get<int>();
    const int level = magic.at("level").get<int>();
    if (required < 1 || required > 100 || (total != 96 && total != 100) || level < 0 || level > required)
        throw std::runtime_error("invalid RPG magic oracle input");
    state.knownMask |= 1 << COMBO_RPG_MAGIC;
    state.enabledMask |= 1 << COMBO_RPG_MAGIC;
    state.required[COMBO_RPG_MAGIC] = required;
    state.level[COMBO_RPG_MAGIC] = level;
    state.magicTotal = total;
    return true;
}
} // namespace ComboRando
