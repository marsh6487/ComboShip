#pragma once

#include "OptionalSfxSample.h"

#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/File.h>
#include <ship/resource/ResourceManager.h>

namespace ComboOptionalSfx {

inline std::vector<int16_t> ReadClip(const std::shared_ptr<Ship::File>& file) {
    return file && file->Buffer ? Decode(*file->Buffer) : std::vector<int16_t>{};
}

inline std::shared_ptr<Ship::ArchiveManager> OwnArchives([[maybe_unused]] const char* game) {
#ifdef COMBO_BUILD
    // The active manager can change during cross-game transitions.
    const auto manager = Ship::CrossRMRegistry::Get(game);
#else
    const auto context = Ship::Context::GetRawInstance();
    const auto manager = context ? context->GetResourceManager() : nullptr;
#endif
    return manager ? manager->GetArchiveManager() : nullptr;
}

inline std::vector<int16_t> ReadCameraCue(const std::shared_ptr<Ship::ArchiveManager>& archives,
                                          const char* samplePath) {
    constexpr const char* cue = "audio/sfx/TwilightPrincessTargetCamera.cue";
    if (archives && archives->HasFile(cue)) {
        // The marker opts in to the empty-target cue. Use its archive's sample
        // so a normal lock-on sound or another pack cannot stand in for it.
        const auto owner = archives->GetArchiveFromFile(cue);
        if (owner && owner->HasFile(samplePath)) {
            return ReadClip(owner->LoadFile(samplePath));
        }
    }
    return {};
}

} // namespace ComboOptionalSfx
