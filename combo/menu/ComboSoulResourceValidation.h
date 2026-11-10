/* Owner-local native resource inspection. Include only from an owning producer TU,
 * after that owner's SOH::Skeleton / Animation / PlayerAnimation definitions. */
#ifndef COMBO_SOUL_RESOURCE_VALIDATION_H
#define COMBO_SOUL_RESOURCE_VALIDATION_H
#include "ComboItemDrawABI.h"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include <ship/resource/ResourceManager.h>
#include <memory>
#include <cstring>

static inline int32_t ComboSoulValidateOwnedResource(const char* game, const char* path, int32_t kind,
                                                     int32_t expectedJoints, CwSoulResourceInfo* out) {
    if (!out || !path || std::strncmp(path, "__OTR__", 7) || expectedJoints < 1 || expectedJoints > 256)
        return 0;
    std::memset(out, 0, sizeof(*out));
    auto rm = Ship::CrossRMRegistry::Get(game);
    if (!rm)
        return CW_DRAW_NOT_READY;
    Ship::ResourceManagerScope scope(rm);
    auto resource = rm->LoadResource(path);
    if (!resource || !resource->GetInitData() || !resource->GetRawPointer())
        return 0;
    const auto type = resource->GetInitData()->Type;
    if (kind == CW_SOUL_RESOURCE_SKELETON) {
        if (type != 0x4F534B4C)
            return 0;
        auto skeleton = std::dynamic_pointer_cast<SOH::Skeleton>(resource);
        if (!skeleton || resource->GetPointerSize() < sizeof(SOH::SkeletonHeader))
            return 0;
        const auto& header = skeleton->skeletonData.skeletonHeader;
        const size_t count = skeleton->skeletonHeaderSegments.size();
        if (!count || count > 255 || header.segment != skeleton->skeletonHeaderSegments.data() ||
            (header.skeletonType != 0 && header.skeletonType != 1))
            return 0;
        if (header.limbCount != count) {
            // EnFsn_Init documents this original MM header bug. Never mutate the
            // resource shared with live actors; the receiver builds a local copy.
            if (std::strcmp(game, "mm") || std::strcmp(path, "__OTR__objects/object_fsn/gFsnSkel") ||
                header.limbCount != 18 || count != 17 || expectedJoints != 18)
                return 0;
        }
        if ((size_t)expectedJoints != count + 1)
            return 0;
        if ((skeleton->limbType != SOH::LimbType::Standard && skeleton->limbType != SOH::LimbType::LOD) ||
            skeleton->limbTable.size() != count)
            return 0;
        for (size_t i = 0; i < count; ++i) {
            const auto limbResource = rm->LoadResource(skeleton->limbTable[i]);
            if (!limbResource || !limbResource->GetInitData() || limbResource->GetInitData()->Type != 0x4F534C42 ||
                limbResource->GetRawPointer() != skeleton->skeletonHeaderSegments[i])
                return 0;
            const auto limb = std::dynamic_pointer_cast<SOH::SkeletonLimb>(limbResource);
            if (!limb || (limb->limbType != SOH::LimbType::Standard && limb->limbType != SOH::LimbType::LOD) ||
                limbResource->GetPointerSize() < sizeof(SOH::StandardLimb))
                return 0;
        }
        out->elementCount = (int32_t)count;
        out->skeletonType = header.skeletonType;
        if (header.skeletonType == 1) {
            if (resource->GetPointerSize() < sizeof(SOH::FlexSkeletonHeader))
                return 0;
            out->dListCount = skeleton->skeletonData.flexSkeletonHeader.dListCount;
            if (out->dListCount < 0 || (size_t)out->dListCount > count)
                return 0;
        }
    } else if (kind == CW_SOUL_RESOURCE_ANIMATION) {
        if (type != 0x4F414E4D || resource->GetPointerSize() < sizeof(SOH::AnimationHeader))
            return 0;
        auto animation = std::dynamic_pointer_cast<SOH::Animation>(resource);
        if (!animation || animation->type != SOH::AnimationType::Normal ||
            animation->rotationIndices.size() < (size_t)expectedJoints || animation->rotationValues.empty())
            return 0;
        const auto& header = animation->animationData.animationHeader;
        if (header.common.frameCount <= 0 || header.frameData != (const int16_t*)animation->rotationValues.data() ||
            (const void*)header.jointIndices != (const void*)animation->rotationIndices.data())
            return 0;
        for (int i = 0; i < expectedJoints; ++i) {
            const auto& index = animation->rotationIndices[i];
            for (uint16_t offset : { index.x, index.y, index.z }) {
                size_t last = offset + (offset >= header.staticIndexMax ? header.common.frameCount - 1 : 0);
                if (last >= animation->rotationValues.size())
                    return 0;
            }
        }
        out->elementCount = (int32_t)animation->rotationIndices.size();
    } else if (kind == CW_SOUL_RESOURCE_PLAYER_POSE) {
        if (type != 0x4F50414D || resource->GetPointerSize() < (size_t)expectedJoints * 3 * sizeof(int16_t))
            return 0;
        auto pose = std::dynamic_pointer_cast<SOH::PlayerAnimation>(resource);
        if (!pose || pose->limbRotData.size() < (size_t)expectedJoints * 3 ||
            pose->limbRotData.data() != resource->GetRawPointer())
            return 0;
        out->elementCount = (int32_t)(pose->limbRotData.size() / 3);
    } else
        return 0;
    out->rawPointer = resource->GetRawPointer();
    return 1;
}
#endif
