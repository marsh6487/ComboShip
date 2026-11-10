// Compiled into two independent DLLs. The foreign caller sees only the POD ABI.
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ComboItemDrawABI.h"
namespace Ship {
struct ResourceInitData { uint32_t Type; };
struct IResource {
    std::shared_ptr<ResourceInitData> metadata = std::make_shared<ResourceInitData>();
    virtual ~IResource() = default;
    virtual void* GetRawPointer() = 0;
    virtual size_t GetPointerSize() = 0;
    std::shared_ptr<ResourceInitData> GetInitData() { return metadata; }
};
struct ResourceManager {
    std::unordered_map<std::string, std::shared_ptr<IResource>> resources;
    std::shared_ptr<IResource> LoadResource(const std::string& path) { return resources[path]; }
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const char*) { static auto rm = std::make_shared<ResourceManager>(); return rm; }
};
struct ResourceManagerScope { explicit ResourceManagerScope(std::shared_ptr<ResourceManager>) {} };
}
namespace SOH {
struct SkeletonHeader { void** segment; uint8_t limbCount, skeletonType; };
struct FlexSkeletonHeader { SkeletonHeader sh; uint8_t dListCount; };
union SkeletonData { SkeletonHeader skeletonHeader; FlexSkeletonHeader flexSkeletonHeader; };
enum class LimbType { Invalid, Standard, LOD, Skin, Curve };
struct StandardLimb { int16_t jointPos[3]; uint8_t child, sibling; void* dl; };
struct SkeletonLimb : Ship::IResource {
    LimbType limbType = LimbType::Standard;
    StandardLimb limb{};
    void* GetRawPointer() override { return &limb; }
    size_t GetPointerSize() override { return sizeof(limb); }
};
struct Skeleton : Ship::IResource {
    SkeletonData skeletonData{};
#ifdef OWNER_OOT
    // The real OoT Skeleton has this additional member before its pointer table.
    std::vector<std::shared_ptr<Ship::IResource>> limbResources;
#endif
    std::vector<void*> skeletonHeaderSegments;
    LimbType limbType = LimbType::Standard;
    std::vector<std::string> limbTable;
    void* GetRawPointer() override { return &skeletonData; }
    size_t GetPointerSize() override { return sizeof(SkeletonData); }
};
enum class AnimationType { Normal, Link };
struct JointIndex { uint16_t x, y, z; };
struct AnimationHeader {
    struct { int16_t frameCount; } common;
    int16_t* frameData;
    JointIndex* jointIndices;
    uint16_t staticIndexMax;
};
struct Animation : Ship::IResource {
    AnimationType type = AnimationType::Normal;
    struct { AnimationHeader animationHeader; } animationData;
    std::vector<uint16_t> rotationValues;
    std::vector<JointIndex> rotationIndices;
    void* GetRawPointer() override { return &animationData; }
    size_t GetPointerSize() override { return sizeof(AnimationHeader); }
};
struct PlayerAnimation : Ship::IResource {
    std::vector<int16_t> limbRotData;
    void* GetRawPointer() override { return limbRotData.data(); }
    size_t GetPointerSize() override { return limbRotData.size() * sizeof(int16_t); }
};
}
#include "ComboSoulResourceValidation.h"
#ifdef OWNER_OOT
static constexpr const char* game = "oot";
#else
static constexpr const char* game = "mm";
#endif
#define EXPORTED extern "C" __attribute__((visibility("default")))
EXPORTED size_t FixtureSkeletonClassSize() { return sizeof(SOH::Skeleton); }
EXPORTED void FixtureConfigure(int scenario) {
    auto rm = Ship::CrossRMRegistry::Get(game);
    rm->resources.clear();
    auto skel = std::make_shared<SOH::Skeleton>();
    skel->metadata->Type = 0x4F534B4C;
    const int boneCount = scenario == 5 ? 17 : 2;
    for (int i = 0; i < boneCount; ++i) {
        auto limb = std::make_shared<SOH::SkeletonLimb>();
        limb->metadata->Type = 0x4F534C42;
        if (scenario == 7) limb->limbType = SOH::LimbType::Skin;
        std::string path = "__OTR__limb" + std::to_string(i);
        rm->resources[path] = limb;
        skel->limbTable.push_back(path);
        skel->skeletonHeaderSegments.push_back(limb->GetRawPointer());
    }
    skel->skeletonData.flexSkeletonHeader = {{skel->skeletonHeaderSegments.data(), (uint8_t)boneCount, 1}, (uint8_t)boneCount};
#ifdef OWNER_OOT
    // Deliberately poison the extra vector's shape: a host-layout cast of this
    // as MM's skeleton reads this empty vector instead of the two valid bones.
    skel->limbResources.clear();
#endif
    if (scenario == 1) skel->skeletonData.skeletonHeader.limbCount = 3;
    if (scenario == 2) skel->skeletonHeaderSegments[1] = nullptr;
    if (scenario == 3) skel->skeletonData.skeletonHeader.skeletonType = 2;
    if (scenario == 5) skel->skeletonData.skeletonHeader.limbCount = 18;
    rm->resources[scenario == 5 ? "__OTR__objects/object_fsn/gFsnSkel" : "__OTR__rig"] = skel;
    auto anim = std::make_shared<SOH::Animation>();
    anim->metadata->Type = 0x4F414E4D;
    anim->rotationValues.resize(9);
    anim->rotationIndices.resize(3);
    anim->animationData.animationHeader = {{3}, (int16_t*)anim->rotationValues.data(), anim->rotationIndices.data(), 1};
    if (scenario == 4) anim->rotationIndices[2].z = 9;
    rm->resources["__OTR__anim"] = anim;
    auto pose = std::make_shared<SOH::PlayerAnimation>();
    pose->metadata->Type = 0x4F50414D;
    pose->limbRotData.resize(scenario == 6 ? 8 : 9);
    rm->resources["__OTR__pose"] = pose;
}
EXPORTED int32_t FixtureQuery(const char* path, int32_t kind, int32_t expectedJoints, CwSoulResourceInfo* out) {
    return ComboSoulValidateOwnedResource(game, path, kind, expectedJoints, out);
}
