// Shared host-local resolver. The including TU supplies its own resource types,
// registered skeletons, native MtxF ABI and PlayerFlexSkin_Load function.
#include "PlayerFlexSkin.h"
#include "PlayerFlexSkinPolicy.h"
#include <cstdio>
#include <memory>
#include <map>
#include <tuple>

namespace {
struct PlayerSkinEntry {
    std::shared_ptr<SOH::Array> resource;
    std::shared_ptr<SOH::Skeleton> skeleton;
    PlayerFlexSkin::Plan plan;
    std::array<std::shared_ptr<Fast::DisplayList>, 32> limbs;
};
std::map<std::tuple<std::string, uintptr_t, uintptr_t, bool>, PlayerSkinEntry> playerSkinCache;
const void* PlayerSkinResolve(void** skeleton, int matrixCount, const std::string& path) {
    auto rig = std::dynamic_pointer_cast<SOH::Skeleton>(PlayerFlexSkin_Load(path));
    if (!rig || rig->type != SOH::SkeletonType::Flex || rig->limbType != SOH::LimbType::LOD ||
        rig->skeletonData.flexSkeletonHeader.sh.segment != skeleton ||
        rig->skeletonData.flexSkeletonHeader.dListCount != matrixCount || rig->limbCount < 1 || rig->limbCount > 32)
        return nullptr;
    const std::string base = path + "/Skin/";
    auto resource = std::dynamic_pointer_cast<SOH::Array>(PlayerFlexSkin_Load(base + "Palette"));
    if (!resource || resource->ArrayType != SOH::ArrayResourceType::Scalar ||
        resource->ArrayScalarType != SOH::ScalarType::ZSCALAR_U16 || resource->ArrayCount != resource->Scalars.size() ||
        resource->Scalars.size() < 4 ||
        resource->Scalars.size() > 4 + size_t(matrixCount) * 24 + size_t(256 - matrixCount) * 3)
        return nullptr;
    const auto cacheKey = std::make_tuple(path, reinterpret_cast<uintptr_t>(resource.get()),
                                        reinterpret_cast<uintptr_t>(rig.get()), ResourceMgr_IsAltAssetsEnabled());
    const auto cached = playerSkinCache.find(cacheKey);
    if (cached != playerSkinCache.end())
        return cached->second.plan.inverse.size() == size_t(matrixCount) ? &cached->second : nullptr;
    std::vector<uint16_t> words;
    words.reserve(resource->Scalars.size());
    for (const auto& scalar : resource->Scalars) words.push_back(scalar.u16);
    auto plan = PlayerFlexSkin::Decode(words, matrixCount);
    if (!plan) return nullptr;
    PlayerSkinEntry entry{resource, rig, std::move(*plan), {}};
    bool any = false;
    for (int i = 0; i < rig->limbCount; ++i) {
        char suffix[32];
        std::snprintf(suffix, sizeof(suffix), "Limbs/Limb%02d", i);
        auto limb = std::dynamic_pointer_cast<Fast::DisplayList>(PlayerFlexSkin_Load(base + suffix));
        if (limb && limb->GetPointerSize()) {
            entry.limbs[i] = std::move(limb);
            any = true;
        }
    }
    if (!any) return nullptr;
    return &playerSkinCache.emplace(cacheKey, std::move(entry)).first->second;
}
} // namespace

extern "C" const void* PlayerFlexSkin_GetPlan(void** skeleton, s32 matrixCount) {
    if (!skeleton || matrixCount < 1 || matrixCount > 32) return nullptr;
    // A private PAK/o2r skeleton must use its own source, never a vanilla alias
    // left in SkeletonPatcher after the draw-time skeleton swap.
    if (const char* path = PakLoader_GetSkinSkeletonPath(skeleton)) {
        std::string_view source(path);
        if (source.starts_with("__OTR__")) source.remove_prefix(7);
        return PlayerSkinResolve(skeleton, matrixCount, std::string(source));
    }
    for (auto it = SOH::SkeletonPatcher::skeletons.rbegin(); it != SOH::SkeletonPatcher::skeletons.rend(); ++it) {
        if (it->skelAnime && it->skelAnime->skeleton == skeleton &&
            PlayerFlexSkin::PlayerSkeleton(it->vanillaSkeletonPath))
            return PlayerSkinResolve(skeleton, matrixCount,
                                     std::string(PlayerFlexSkin::Canonical(it->vanillaSkeletonPath)));
    }
    return nullptr;
}
extern "C" s32 PlayerFlexSkin_MatrixCount(const void* opaque) {
    return opaque ? static_cast<const PlayerSkinEntry*>(opaque)->plan.blends.size() : 0;
}
extern "C" void PlayerFlexSkin_BlendMatrix(const void* opaque, s32 index, const MtxF* native, MtxF* result) {
    static_assert(sizeof(MtxF) == 16 * sizeof(float));
    *result = {};
    result->xx = result->yy = result->zz = result->ww = 1.f;
    if (!opaque || !native || index < 0 || index >= PlayerFlexSkin_MatrixCount(opaque)) return;
    const auto& plan = static_cast<const PlayerSkinEntry*>(opaque)->plan;
    std::array<PlayerFlexSkin::Matrix, 32> matrices{};
    for (size_t i = 0; i < plan.inverse.size(); ++i)
        std::memcpy(matrices[i].data(), &native[i].xx, sizeof(MtxF));
    const auto blended = PlayerFlexSkin::Blend(plan, index, matrices.data());
    std::memcpy(&result->xx, blended.data(), sizeof(MtxF));
}
extern "C" Gfx* PlayerFlexSkin_SelectLimb(const void* opaque, s32 index, Gfx* selected, Gfx* original) {
    if (!opaque || !selected || selected != original || index < 1 || index > 32) return selected;
    const auto& limb = static_cast<const PlayerSkinEntry*>(opaque)->limbs[index - 1];
    return limb ? limb->GetPointer() : selected;
}
