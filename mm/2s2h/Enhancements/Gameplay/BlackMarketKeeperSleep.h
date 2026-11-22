#pragma once

#include <array>
#include <limits>
#include <utility>
#include <vector>

namespace BlackMarketKeeperSleep {

inline Vec3s SampleJoint(const AnimationHeader& animation, size_t joint, size_t frame) {
    const auto& index = animation.jointIndices[joint];
    const auto channel = [&](u16 offset) {
        return animation.frameData[offset + (offset < animation.staticIndexMax ? 0 : frame)];
    };
    return { channel(index.x), channel(index.y), channel(index.z) };
}

// The retained sleep/get-up clips use OoT's ten-limb upper-body rig. MM adds
// a root, two arm pivots, and legs. Keep the native stance and compensate
// for its waist orientation in the torso track before copying shared joints.
struct Clip {
    static constexpr size_t JointCount = TREASURE_CHEST_SHOP_GAL_LIMB_MAX;
    AnimationHeader animation{};
    std::array<JointIndex, JointCount> indices{};
    std::vector<s16> values;

    bool Build(const AnimationHeader& donor, const AnimationHeader& stance) {
        if (donor.common.frameCount <= 0)
            return false;
        const size_t frames = donor.common.frameCount;
        if (frames * JointCount * 3 > std::numeric_limits<u16>::max())
            return false;

        std::array<Vec3s, JointCount> pose;
        for (size_t joint = 0; joint < pose.size(); ++joint)
            pose[joint] = SampleJoint(stance, joint, 0);
        pose[TREASURE_CHEST_SHOP_GAL_LIMB_LEFT_ARM_ROOT] = {};
        pose[TREASURE_CHEST_SHOP_GAL_LIMB_RIGHT_ARM_ROOT] = {};

        auto rotation = [](Vec3s angles, MtxF& matrix) {
            SkinMatrix_SetRotateRPY(&matrix, angles.x, angles.y, angles.z);
        };
        MtxF root, hips, waist, inverseWaist;
        rotation(pose[TREASURE_CHEST_SHOP_GAL_LIMB_ROOT], root);
        rotation(pose[TREASURE_CHEST_SHOP_GAL_LIMB_HIPS], hips);
        SkinMatrix_MtxFMtxFMult(&root, &hips, &waist);
        if (SkinMatrix_Invert(&waist, &inverseWaist) != 0)
            return false;

        constexpr std::pair<size_t, size_t> sharedJoints[] = {
            { TREASURE_CHEST_SHOP_GAL_LIMB_NECK, 3 },           { TREASURE_CHEST_SHOP_GAL_LIMB_HEAD, 4 },
            { TREASURE_CHEST_SHOP_GAL_LIMB_LEFT_UPPER_ARM, 5 }, { TREASURE_CHEST_SHOP_GAL_LIMB_LEFT_FOREARM, 6 },
            { TREASURE_CHEST_SHOP_GAL_LIMB_LEFT_HAND, 7 },      { TREASURE_CHEST_SHOP_GAL_LIMB_RIGHT_UPPER_ARM, 8 },
            { TREASURE_CHEST_SHOP_GAL_LIMB_RIGHT_FOREARM, 9 },  { TREASURE_CHEST_SHOP_GAL_LIMB_RIGHT_HAND, 10 },
        };
        values.resize(JointCount * 3 * frames);
        for (size_t joint = 0; joint < pose.size(); ++joint) {
            const size_t base = joint * 3 * frames;
            indices[joint] = { static_cast<u16>(base), static_cast<u16>(base + frames),
                               static_cast<u16>(base + 2 * frames) };
        }
        for (size_t frame = 0; frame < frames; ++frame) {
            MtxF donorWaist, donorTorso, relativeWaist, torso;
            rotation(SampleJoint(donor, 1, frame), donorWaist);
            rotation(SampleJoint(donor, 2, frame), donorTorso);
            SkinMatrix_MtxFMtxFMult(&inverseWaist, &donorWaist, &relativeWaist);
            SkinMatrix_MtxFMtxFMult(&relativeWaist, &donorTorso, &torso);
            Matrix_MtxFToZYXRot(&torso, &pose[TREASURE_CHEST_SHOP_GAL_LIMB_TORSO], false);
            for (const auto& [target, source] : sharedJoints)
                pose[target] = SampleJoint(donor, source, frame);
            for (size_t joint = 0; joint < pose.size(); ++joint) {
                const auto& index = indices[joint];
                values[index.x + frame] = pose[joint].x;
                values[index.y + frame] = pose[joint].y;
                values[index.z + frame] = pose[joint].z;
            }
        }
        animation = { donor.common, values.data(), indices.data(), 0 };
        return true;
    }
};

} // namespace BlackMarketKeeperSleep
