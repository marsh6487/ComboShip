#ifdef COMBO_BUILD
#include "PlacedSariaResources.h"
#include "ComboExport.h"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>

extern "C" COMBO_EXPORT int OOT_PlacedSariaAltEnabled(void) {
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    return owner && owner->IsAltAssetsEnabled();
}

extern "C" COMBO_EXPORT int OOT_PlacedSariaLoad(unsigned pose, ComboPlacedSariaResources* output) {
    if (!output)
        return 0;
    *output = {};
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!owner)
        return 0;
    try {
        Ship::OwnRMScope scope("oot");
        auto snapshot = PlacedSaria::Build([&](const std::string& path) { return owner->LoadResource(path, true); },
                                           owner->IsAltAssetsEnabled(), pose);
        if (!snapshot)
            return 0;
        *output = snapshot->result;
        output->owner = snapshot.release();
        return 1;
    } catch (const std::exception&) { return 0; }
}

extern "C" COMBO_EXPORT void OOT_PlacedSariaRelease(void* owner) {
    delete static_cast<PlacedSaria::Snapshot*>(owner);
}
#endif
