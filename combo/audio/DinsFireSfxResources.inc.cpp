// Included once by each native host adapter. Optional files are read/decoded on
// the game thread before Mixer::Begin; this code never runs in the audio callback.
#include "DinsFireSfxSamples.h"
#include "DinsFireSfxWav.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#endif
#include <cstdio>
#include <exception>
#include <span>

namespace {
struct DinsOverrideCache {
    // A weak identity avoids retaining another module's manager/thread pool.
    // Its foreign control block is dropped before registry teardown as well.
    std::weak_ptr<Ship::ResourceManager> owner;
    std::shared_ptr<const CwDinsAudio::SampleBank> bank;
};

DinsOverrideCache& DinsGetOverrideCache() {
    static DinsOverrideCache cache;
    return cache;
}

void DinsClearOverrideCache() {
    // CrossRMRegistry invokes teardown listeners on the deinit/game thread,
    // before releasing the manager or unloading the owning module.
    auto& cache = DinsGetOverrideCache();
    cache.bank.reset();
    cache.owner.reset();
}

std::shared_ptr<Ship::ResourceManager> DinsOwnResourceManager() {
#ifdef COMBO_BUILD
#ifdef CW_DINS_AUDIO_MM
    return Ship::CrossRMRegistry::Get("mm");
#else
    return Ship::CrossRMRegistry::Get("oot");
#endif
#else
    const auto context = Ship::Context::GetRawInstance();
    return context ? context->GetResourceManager() : nullptr;
#endif
}

std::shared_ptr<const CwDinsAudio::SampleBank> DinsLoadOverrideBank() {
#ifdef COMBO_BUILD
    static const bool teardownRegistered = [] {
        Ship::CrossRMRegistry::RegisterTeardownListener(DinsClearOverrideCache);
        return true;
    }();
    (void)teardownRegistered;
#endif
    const auto manager = DinsOwnResourceManager();
    auto& cache = DinsGetOverrideCache();
    if (!manager) {
        DinsClearOverrideCache();
        return {}; // Startup/teardown is retryable; never cache a missing owner.
    }
    if (cache.bank && cache.owner.lock() == manager) {
        return cache.bank;
    }

    const auto archives = manager->GetArchiveManager();
    if (!archives) {
        DinsClearOverrideCache();
        return {};
    }
    auto loaded = std::make_shared<CwDinsAudio::SampleBank>();
    for (size_t cue = 0; cue < CwDinsAudio::kCueCount; ++cue) {
        const char* path = CwDinsAudio::kOverridePaths[cue];
        try {
            // These literal raw archive filenames retain their .wav extension.
            // The typed ResourceMgr extension-stripped cache cannot answer this.
            if (!archives->HasFile(path)) {
                continue;
            }
            const auto file = archives->LoadFile(path);
            if (file && file->Buffer && !file->Buffer->empty() &&
                file->Buffer->size() <= CwDinsAudio::kMaxOverrideFileBytes) {
                loaded->clips[cue] = CwDinsAudio::DecodeOverrideWav(
                    std::span(reinterpret_cast<const uint8_t*>(file->Buffer->data()), file->Buffer->size()));
            }
            if (loaded->clips[cue].empty()) {
                std::fprintf(
                    stderr,
                    "[Din audio] invalid override %s; using compiled cue (nonempty mono PCM16 32000Hz, <=10s)\n", path);
            }
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[Din audio] unable to load %s: %s; using compiled cue\n", path, error.what());
        }
    }
    // Publish a complete owned bank. Mod edits require a restart/new manager;
    // active casts retain immutable snapshots independently of archive lifetime.
    cache.owner = manager;
    cache.bank = std::move(loaded);
    return cache.bank;
}
} // namespace
