#pragma once

#include "RitoMaskSettingsKeys.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <algorithm>

namespace ComboRando::RitoMaskSettings {
inline std::optional<int> Field(const nlohmann::json& value, const char* name) {
    auto it = value.find(name);
    if (it == value.end())
        return std::nullopt;
    if (it->is_boolean())
        return it->get<bool>();
    if (!it->is_number_integer())
        throw std::runtime_error("Invalid Rito Mask setting");
    return it->get<int>() != 0;
}

inline std::optional<int> AgreedField(const nlohmann::json& oot, const nlohmann::json& mm, const char* name) {
    auto a = Field(oot, name), b = Field(mm, name);
    if (a && b && *a != *b)
        throw std::runtime_error("Conflicting combined Rito Mask settings");
    return a ? a : b;
}

inline int SnapshotValue(const nlohmann::json& value) {
    if (auto field = Field(value, SharedCvar))
        return *field;
    if (auto field = Field(value, GenerationCvar))
        return *field;
    return Field(value, OotCvar).value_or(0) || Field(value, MmCvar).value_or(0);
}

// Normalize before either native restore/prep. This also handles old spoilers
// whose only request was MM's, without letting stale solo values affect replay.
inline void NormalizeSnapshots(nlohmann::json& oot, nlohmann::json& mm) {
    int value;
    if (auto shared = AgreedField(oot, mm, SharedCvar))
        value = *shared;
    else if (auto generation = AgreedField(oot, mm, GenerationCvar))
        value = *generation;
    else
        value = Field(oot, OotCvar).value_or(0) || Field(mm, MmCvar).value_or(0);
    for (auto* snapshot : { &oot, &mm }) {
        (*snapshot)[SharedCvar] = value;
        (*snapshot)[GenerationCvar] = value;
    }
    oot[OotCvar] = value;
    mm[MmCvar] = value;
}

inline void NormalizeConfig(nlohmann::json& config) {
    using Pointer = nlohmann::json::json_pointer;
    const Pointer shared("/CVars/gCombo/Rando/ShuffleRitoMask");
    const Pointer oot("/CVars/gRandoSettings/ShuffleRitoMask");
    const Pointer mm("/CVars/gRando/Options/RO_SHUFFLE_RITO_MASK");
    if (!config.contains(shared) && !config.contains(oot) && !config.contains(mm))
        return;
    if (config.contains(shared))
        config[shared] = config.at(shared).get<int>() != 0 ? 1 : 0;
    else
        config[shared] = (config.contains(oot) && config.at(oot).get<int>() != 0) ||
                                 (config.contains(mm) && config.at(mm).get<int>() != 0)
                             ? 1
                             : 0;
}

inline int OotPresetValue(const nlohmann::json& section, bool merge, int before) {
    auto block = section.find("gRandoSettings");
    if (block == section.end())
        return before;
    if (block->is_null())
        return 0;
    return Field(*block, "ShuffleRitoMask").value_or(merge ? before : 0);
}

inline int MmPresetValue(const nlohmann::json& preset, int before) {
    int value = before;
    if (preset.contains("ClearCVars")) {
        for (auto path : preset.at("ClearCVars").get<std::vector<std::string>>()) {
            std::replace(path.begin(), path.end(), '/', '.');
            if (!path.empty() && path.front() == '.')
                path.erase(0, 1);
            if (!path.empty() && (path == MmCvar || std::string(MmCvar).starts_with(path + ".")))
                value = 0;
        }
    }
    const nlohmann::json::json_pointer mirror("/CVars/gRando/Options/RO_SHUFFLE_RITO_MASK");
    if (preset.contains(mirror))
        value = preset.at(mirror).get<int>() != 0;
    return value;
}

inline void StampNativePreset(nlohmann::json& cvars, bool oot, int value) {
    const nlohmann::json::json_pointer path(oot ? "/gRandoSettings/ShuffleRitoMask"
                                                : "/gRando/Options/RO_SHUFFLE_RITO_MASK");
    cvars[path] = value != 0 ? 1 : 0;
}

// Native dumps describe the frozen seed. Temporary save/replay work also needs
// to restore a newer UI choice, which may intentionally differ from that seed.
inline std::string WithSharedPreference(const std::string& snapshot, int value) {
    if (snapshot.empty())
        return snapshot;
    auto json = nlohmann::json::parse(snapshot);
    json[SharedCvar] = value != 0 ? 1 : 0;
    return json.dump();
}
} // namespace ComboRando::RitoMaskSettings
