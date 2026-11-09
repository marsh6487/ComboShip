#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>

namespace ComboRando {
// Separate category bits: these are not scalar tier families.
inline constexpr uint32_t SS_ENEMY = 1u << 17;
inline constexpr uint32_t SS_NPC = 1u << 18;
inline constexpr uint32_t SS_ANIMAL = 1u << 19;
inline constexpr uint32_t SS_MISC = 1u << 20;
inline constexpr uint32_t SS_ALL = SS_ENEMY | SS_NPC | SS_ANIMAL | SS_MISC;
struct SharedSoulCategory {
    uint32_t bit;
    const char* key;
    const char* cvar;
    const char* option;
};
inline constexpr SharedSoulCategory SharedSoulCategories[] = {
    { SS_ENEMY, "sharedSoulsEnemy", "gCombo.Rando.Shared.SoulsEnemy", "enemy" },
    { SS_NPC, "sharedSoulsNpc", "gCombo.Rando.Shared.SoulsNpc", "npc" },
    { SS_ANIMAL, "sharedSoulsAnimal", "gCombo.Rando.Shared.SoulsAnimal", "animal" },
    { SS_MISC, "sharedSoulsMisc", "gCombo.Rando.Shared.SoulsMisc", "misc" },
};
struct SharedSoulPair {
    uint32_t bit;
    const char* ootName;
    const char* mmName;
};
inline constexpr SharedSoulPair SharedSoulPairs[] = {
#define SHARED_SOUL(category, oot, mm, ootName, mmName) { SS_##category, ootName, mmName },
#include "SharedSoulCatalog.inc"
#undef SHARED_SOUL
};
inline uint32_t EffectiveSharedSoulMask(uint32_t requested, const nlohmann::json& oot, const nlohmann::json& mm) {
    uint32_t mask = 0;
    auto enabled = [](const nlohmann::json& dump, const char* key) {
        if (!dump.is_object() || !dump.contains("soulOptions") || !dump["soulOptions"].is_object())
            return false;
        const auto& options = dump["soulOptions"];
        return options.contains(key) && options[key].is_boolean() && options[key].get<bool>();
    };
    for (const auto& category : SharedSoulCategories)
        if ((requested & category.bit) && enabled(oot, category.option) && enabled(mm, category.option))
            mask |= category.bit;
    return mask;
}
} // namespace ComboRando
