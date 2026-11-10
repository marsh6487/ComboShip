#pragma once

#include "RitoMaskSettings.h"
#include "RitoMaskSettingsJson.h"

namespace ComboRando::RitoMaskSettings {
inline void RestoreSnapshot(nlohmann::json& snapshot) {
    const int value = SnapshotValue(snapshot);
    SetShared(value);
    CVarSetInteger(GenerationCvar, Field(snapshot, GenerationCvar).value_or(value));
    snapshot.erase(OotCvar);
    snapshot.erase(MmCvar);
}
} // namespace ComboRando::RitoMaskSettings
