#include "MidnaAudioResources.h"

#include <algorithm>
#include <cmath>
#include <ship/Context.h>
#include <ship/resource/File.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#endif

namespace {
std::shared_ptr<Ship::ResourceManager> OwnManager() {
#ifdef COMBO_BUILD
    // Never fall back to OoT's active manager during a switch or teardown.
    return Ship::CrossRMRegistry::Get("mm");
#else
    const auto context = Ship::Context::GetRawInstance();
    return context ? context->GetResourceManager() : nullptr;
#endif
}
} // namespace

extern "C" bool MMMidnaResources_Exists(const char* path) {
    const auto manager = OwnManager();
    return manager && path && manager->GetArchiveManager()->HasFile(path);
}

extern "C" void* MMMidnaResources_Load(const char* path) {
    const auto manager = OwnManager();
    if (!manager || !path || !manager->GetArchiveManager()->HasFile(path)) {
        return nullptr;
    }
    // Do not retain a raw pointer across resource cache invalidation.
    const auto resource = manager->LoadResource(path);
    return resource && resource->GetPointerSize() != 0 ? resource->GetRawPointer() : nullptr;
}

namespace MMMidnaAudioResources {
bool Enabled() {
    return CVarGetInteger("gEnhancements.MidnaCompanionMM", 0) != 0;
}

bool HasModel() {
    // Archive filenames are available before BenPort's extension cache is populated.
    return MMMidnaResources_Exists("objects/midna_navi/poc1/MidnaFloatDL");
}

bool ReadClip(const char* path, std::vector<uint8_t>& bytes) {
    const auto manager = OwnManager();
    if (!manager || !path) {
        return false;
    }
    const auto archives = manager->GetArchiveManager();
    if (!archives->HasFile(path)) {
        return false;
    }
    const auto file = archives->LoadFile(path);
    if (!file || !file->Buffer || file->Buffer->empty() || file->Buffer->size() > 32000 * 10 * 2 + 65536) {
        return false;
    }
    bytes.assign(file->Buffer->begin(), file->Buffer->end());
    return true;
}

float Gain() {
    const float master = CVarGetFloat("gSettings.Audio.MasterVolume", 0.4f);
    const float sfx = CVarGetFloat("gSettings.Audio.SoundEffectsVolume", 1.0f);
    return std::isfinite(master) && std::isfinite(sfx) ? std::clamp(master, 0.0f, 1.0f) * std::clamp(sfx, 0.0f, 1.0f)
                                                       : 0.0f;
}
} // namespace MMMidnaAudioResources
