#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include "soh/Enhancements/cosmetics/NativeNpcPose.h"
#include "soh/Enhancements/cosmetics/NativeNpcPoseResources.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Animation.h"
#include "soh/resource/type/Skeleton.h"
#include <fast/resource/type/DisplayList.h>

// In-memory resource-manager fixture; the production resolver and real SOH
// animation resources below are compiled and linked without the game runtime.
namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> init) {
}
IResource::~IResource() = default;
} // namespace Ship
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {
}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() {
    return Instructions.data();
}
size_t DisplayList::GetPointerSize() {
    return Instructions.size() * sizeof(Gfx);
}
} // namespace Fast
std::vector<SOH::SkeletonPatchInfo> SOH::SkeletonPatcher::skeletons;
static std::unordered_map<std::string, std::shared_ptr<Ship::IResource>> resources;
static bool alt = true;
bool ResourceMgr_IsAltAssetsEnabled() {
    return alt;
}
int ResourceMgr_OTRSigCheck(char* data) {
    return data && std::memcmp(data, "__OTR__", 7) == 0;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    return resources.contains("alt/" + std::string(SOH::NativeNpcPose::Canonical(path)));
}
std::shared_ptr<Ship::IResource> ResourceMgr_GetResourceByNameHandlingMQ(const char* path) {
    auto key = std::string(SOH::NativeNpcPose::Canonical(path));
    if (alt && resources.contains("alt/" + key))
        return resources["alt/" + key];
    return resources.contains(key) ? resources[key] : nullptr;
}
static std::shared_ptr<SOH::Animation> Clip() {
    auto clip = std::make_shared<SOH::Animation>();
    clip->type = SOH::AnimationType::Normal;
    clip->animationData.animationHeader.common.frameCount = 2;
    clip->animationData.animationHeader.staticIndexMax = 3;
    clip->rotationValues = { 10, 20, 30, 100, 200 };
    clip->rotationIndices = { { 0, 1, 2 }, { 3, 1, 2 } };
    return clip;
}
int main() {
    SkelAnime skel = {};
    skel.limbCount = 2;
    const char source[] = "__OTR__objects/object_os_anime/gKokiriStandingArmsBehindBackAnim";
    auto original = reinterpret_cast<AnimationHeader*>(const_cast<char*>(source));
    skel.animation = original;
    auto native = Clip(), fit = Clip();
    fit->rotationValues[3] = 1000;
    const std::string skeleton = "objects/object_kw1/NativeNpcV1/Skel";
    const auto key = "alt/" + SOH::NativeNpcPose::AnimationPath(skeleton, source);
    resources["objects/object_os_anime/gKokiriStandingArmsBehindBackAnim"] = native;
    resources["alt/" + skeleton] = Clip(); // Presence witness only.
    resources[key] = fit;
    SOH::SkeletonPatcher::skeletons.push_back({ &skel, skeleton });
    SOH::SkeletonPatcher::skeletons.push_back({ &skel, "objects/object_fa/gFaSkel" });
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    SOH::SkeletonPatcher::skeletons.pop_back();
    auto result = NativeNpcPose_ResolveAnimation(&skel, original);
    assert(result == reinterpret_cast<AnimationHeader*>(&fit->animationData.animationHeader));
    assert(skel.animation == original);
    alt = false;
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    alt = true;
    fit->rotationValues[0]++;
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    fit->rotationValues[0]--;
    fit->type = SOH::AnimationType::Link;
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    fit->type = SOH::AnimationType::Normal;
    resources.erase(key);
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    Gfx fallback[1] = {};
    const auto limbKey = key + "/Limbs/Limb00";
    auto display = std::make_shared<Fast::DisplayList>();
    display->Instructions.resize(1);
    resources[limbKey] = display;
    assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
    assert(NativeNpcPose_SelectLimb(&skel, 0, fallback) == fallback);
    resources[key + "/Props/Basket"] = display;
    assert(NativeNpcPose_SelectProp(&skel, "Basket", fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
    assert(NativeNpcPose_SelectProp(&skel, "MissingProp", fallback) == fallback);
    resources[limbKey] = native;
    assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == fallback);
    SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = "objects/object_fa/gFaSkel";
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    puts("PASS production pose resource bridge: exact private clip/mesh/prop, live Alt, absent/invalid resources, "
         "original "
         "identity and unrelated actor fallback.");
}
