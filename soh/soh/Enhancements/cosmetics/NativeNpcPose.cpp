#include "NativeNpcPose.h"
#include "NativeNpcPoseResources.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Animation.h"
#include "soh/resource/type/Skeleton.h"
#include <cstdio>
#include <fast/resource/type/DisplayList.h>

static std::string NativeNpcPose_Path(const SkelAnime* skelAnime, AnimationHeader* animation) {
    if (skelAnime == nullptr || animation == nullptr || !ResourceMgr_IsAltAssetsEnabled() ||
        !ResourceMgr_OTRSigCheck(reinterpret_cast<char*>(animation))) {
        return {};
    }
    for (auto entry = SOH::SkeletonPatcher::skeletons.rbegin(); entry != SOH::SkeletonPatcher::skeletons.rend();
         ++entry) {
        if (entry->skelAnime == skelAnime) {
            return ResourceMgr_FileAltExists(entry->vanillaSkeletonPath.c_str())
                       ? SOH::NativeNpcPose::AnimationPath(entry->vanillaSkeletonPath,
                                                           reinterpret_cast<const char*>(animation))
                       : std::string{};
        }
    }
    return {};
}

extern "C" AnimationHeader* NativeNpcPose_ResolveAnimation(const SkelAnime* skelAnime, AnimationHeader* animation) {
    const auto path = NativeNpcPose_Path(skelAnime, animation);
    if (path.empty() || !ResourceMgr_FileAltExists(path.c_str())) {
        return animation;
    }
    const auto fitted =
        std::dynamic_pointer_cast<SOH::Animation>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
    const auto native = std::dynamic_pointer_cast<SOH::Animation>(
        ResourceMgr_GetResourceByNameHandlingMQ(reinterpret_cast<const char*>(animation)));
    if (fitted == nullptr || native == nullptr || fitted->type != SOH::AnimationType::Normal ||
        native->type != SOH::AnimationType::Normal ||
        !SOH::NativeNpcPose::Validate(*fitted, *native, skelAnime->limbCount)) {
        return animation;
    }
    return reinterpret_cast<AnimationHeader*>(&fitted->animationData.animationHeader);
}

extern "C" Gfx* NativeNpcPose_SelectLimb(const SkelAnime* skelAnime, s32 limbIndex, Gfx* original) {
    if (original == nullptr || skelAnime == nullptr || limbIndex < 1 || limbIndex >= skelAnime->limbCount) {
        return original;
    }
    auto path = NativeNpcPose_Path(skelAnime, static_cast<AnimationHeader*>(skelAnime->animation));
    if (path.empty()) {
        return original;
    }
    char suffix[32];
    snprintf(suffix, sizeof(suffix), "/Limbs/Limb%02d", limbIndex - 1);
    path += suffix;
    if (!ResourceMgr_FileAltExists(path.c_str())) {
        return original;
    }
    // Return the resource's display-list data; never a temporary path string.
    auto resource =
        std::dynamic_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
    return resource != nullptr && resource->GetPointerSize() > 0 ? reinterpret_cast<Gfx*>(resource->GetRawPointer())
                                                                 : original;
}

extern "C" Gfx* NativeNpcPose_SelectProp(const SkelAnime* skelAnime, const char* prop, Gfx* original) {
    if (skelAnime == nullptr || prop == nullptr || *prop == '\0' ||
        std::string_view(prop).find('/') != std::string_view::npos) {
        return original;
    }
    auto path = NativeNpcPose_Path(skelAnime, static_cast<AnimationHeader*>(skelAnime->animation));
    if (path.empty()) {
        return original;
    }
    path += "/Props/";
    path += prop;
    if (!ResourceMgr_FileAltExists(path.c_str())) {
        return original;
    }
    auto resource =
        std::dynamic_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
    return resource != nullptr && resource->GetPointerSize() > 0 ? reinterpret_cast<Gfx*>(resource->GetRawPointer())
                                                                 : original;
}
