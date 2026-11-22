#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace ComboRando {

struct BossWarpRewardSave {
    explicit BossWarpRewardSave(int sourceGame) : required(1u << sourceGame) {
    }

    std::array<nlohmann::json, 2> native;
    uint8_t required;
    bool complete = false;
    std::string displayName;

    bool NeedsSnapshot(int game) const {
        return (required & (1u << game)) && !native[game].is_object();
    }
};

} // namespace ComboRando
