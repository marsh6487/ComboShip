#ifdef COMBO_BUILD
#include "MmPlacedSariaResources.h"
#include <string>
#include <cstddef>

static_assert(sizeof(SOH::FlexSkeletonHeader) == sizeof(::FlexSkeletonHeader));
static_assert(offsetof(SOH::FlexSkeletonHeader, dListCount) == offsetof(::FlexSkeletonHeader, dListCount));
static_assert(sizeof(SOH::AnimationHeader) == sizeof(::AnimationHeader));
static_assert(offsetof(SOH::AnimationHeader, frameData) == offsetof(::AnimationHeader, frameData));

// Native MM helpers, bound to the initialized host resource manager.
extern std::shared_ptr<Ship::IResource> GetResourceByName(const char* path, bool loadExact);
extern "C" bool ResourceMgr_IsAltAssetsEnabled();

int MM_PlacedSariaAlt() {
    try {
        return ResourceMgr_IsAltAssetsEnabled();
    } catch (...) { return 0; }
}

int MM_PlacedSariaLoad(unsigned pose, ComboPlacedSariaResources* output) {
    if (!output)
        return 0;
    *output = {};
    try {
        // Use MM's own mounted companion/mod archives and its current Alt policy.
        auto snapshot = MmPlacedSaria::Build(
            [](const std::string& path) { return GetResourceByName(path.c_str(), true); }, MM_PlacedSariaAlt(), pose);
        if (!snapshot)
            return 0;
        *output = snapshot->result;
        output->owner = snapshot.release();
        return 1;
    } catch (...) {
        *output = {};
        return 0;
    }
}

void MM_PlacedSariaRelease(void* owner) {
    delete static_cast<MmPlacedSaria::Snapshot*>(owner);
}

#endif // COMBO_BUILD
