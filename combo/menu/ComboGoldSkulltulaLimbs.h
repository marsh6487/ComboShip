#pragma once

// Native EnSw gold-variant replacements, shared by OoT and MM. Keep each mesh
// attached to the original actor's limb instead of submitting the whole set at
// the GI root. This profile does not instantiate or mutate an actor.
namespace ComboGoldSkulltula {
struct Limb {
    int index;
    const char* path;
};
inline constexpr Limb Limbs[] = {
    { 23, "__OTR__objects/object_st/object_st_DL_004788" }, { 8, "__OTR__objects/object_st/object_st_DL_0046F0" },
    { 14, "__OTR__objects/object_st/object_st_DL_004658" }, { 11, "__OTR__objects/object_st/object_st_DL_0045C0" },
    { 26, "__OTR__objects/object_st/object_st_DL_004820" }, { 20, "__OTR__objects/object_st/object_st_DL_0048B8" },
    { 17, "__OTR__objects/object_st/object_st_DL_004950" }, { 29, "__OTR__objects/object_st/object_st_DL_0049E8" },
    { 5, "__OTR__objects/object_st/object_st_DL_003FB0" },  { 4, "__OTR__objects/object_st/object_st_DL_0043D8" },
};
inline const char* LimbPath(int index) {
    for (const auto& limb : Limbs)
        if (limb.index == index)
            return limb.path;
    return nullptr;
}
} // namespace ComboGoldSkulltula
