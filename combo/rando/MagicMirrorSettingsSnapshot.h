#pragma once

#include "MagicMirrorSettings.h"
#include "MagicMirrorSettingsJson.h"

namespace ComboRando::MagicMirrorSettings {
inline void RestoreSnapshot(nlohmann::json& snapshot) {
    const int value = SnapshotValue(snapshot);
    SetShared(value);
    CVarSetInteger(GenerationCvar, Field(snapshot, GenerationCvar).value_or(value));
    snapshot.erase(OotCvar);
    snapshot.erase(MmCvar);
}
} // namespace ComboRando::MagicMirrorSettings
