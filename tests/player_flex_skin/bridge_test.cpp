#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include "global.h"
#include <fast/lus_gbi.h>
#ifdef PLAYER_SKIN_MM
#include "2s2h/resource/type/Array.h"
#include "2s2h/resource/type/Skeleton.h"
#else
#include "soh/resource/type/Array.h"
#include "soh/resource/type/Skeleton.h"
#endif
#include <fast/resource/type/DisplayList.h>

namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData>) {}
IResource::~IResource() = default;
}
namespace SOH {
Array::Array() : Resource(nullptr) {}
void* Array::GetPointer() { return nullptr; }
size_t Array::GetPointerSize() { return 0; }
SkeletonData* Skeleton::GetPointer() { return &skeletonData; }
size_t Skeleton::GetPointerSize() { return sizeof(skeletonData); }
std::vector<SkeletonPatchInfo> SkeletonPatcher::skeletons;
}
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() { return Instructions.data(); }
size_t DisplayList::GetPointerSize() { return Instructions.size() * sizeof(Gfx); }
}
static bool alt;
static std::map<std::string, std::shared_ptr<Ship::IResource>> resources;
static const char* privatePath;
static void** privateSkeleton;
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return alt; }
extern "C" const char* PakLoader_GetSkinSkeletonPath(void** skeleton) {
    return skeleton == privateSkeleton ? privatePath : nullptr;
}
static std::shared_ptr<Ship::IResource> PlayerFlexSkin_Load(const std::string& path) {
    if (alt && !path.starts_with("alt/") && resources.contains("alt/" + path)) return resources.at("alt/" + path);
    return resources.contains(path) ? resources.at(path) : nullptr;
}
#include "combo/actors/PlayerFlexSkinBridge.inc"

static std::vector<uint16_t> Words() {
    std::vector<uint16_t> words{0x4e53, 1, 2, 1};
    for (int bone = 0; bone < 2; ++bone)
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col) {
                float value = row == col ? 1.f : (bone == 1 && row == 0 && col == 3 ? -1.f : 0.f);
                uint32_t bits; std::memcpy(&bits, &value, sizeof(bits));
                words.push_back(bits); words.push_back(bits >> 16);
            }
    words.insert(words.end(), {0, 1, 8});
    return words;
}
static std::shared_ptr<SOH::Array> Palette(std::vector<uint16_t> words) {
    auto array = std::make_shared<SOH::Array>();
    array->ArrayType = SOH::ArrayResourceType::Scalar;
    array->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
    array->ArrayCount = words.size();
    for (auto word : words) { SOH::ScalarData scalar{}; scalar.u16 = word; array->Scalars.push_back(scalar); }
    return array;
}
static std::shared_ptr<SOH::Skeleton> Rig(void** limbs) {
    auto rig = std::make_shared<SOH::Skeleton>();
    rig->type = SOH::SkeletonType::Flex;
    rig->limbType = SOH::LimbType::LOD;
    rig->limbCount = 2;
    rig->skeletonData.flexSkeletonHeader.sh.segment = limbs;
    rig->skeletonData.flexSkeletonHeader.dListCount = 2;
    return rig;
}
int main() {
    const std::string path = "objects/object_link_boy/gLinkAdultSkel";
    const std::string base = path + "/Skin/";
    void* limbs[2]{}; void* other[2]{};
    SkelAnime skel{}; skel.skeleton = limbs;
    SOH::SkeletonPatcher::skeletons.push_back({&skel, path});
    auto rig = Rig(limbs); resources[path] = rig;
    Gfx rigid[1]{}, equipment[1]{};
    auto mesh = std::make_shared<Fast::DisplayList>(); mesh->Instructions.resize(1);
    assert(!PlayerFlexSkin_GetPlan(limbs, 2));
    resources[base + "Palette"] = Palette(Words());
    assert(!PlayerFlexSkin_GetPlan(limbs, 2)); // no weighted companion
    resources[base + "Limbs/Limb00"] = mesh;
    auto plan = PlayerFlexSkin_GetPlan(limbs, 2);
    assert(plan && PlayerFlexSkin_MatrixCount(plan) == 1);
    assert(PlayerFlexSkin_SelectLimb(plan, 1, rigid, rigid) == mesh->GetPointer());
    assert(PlayerFlexSkin_SelectLimb(plan, 2, rigid, rigid) == rigid); // partial body opt-in
    assert(PlayerFlexSkin_SelectLimb(plan, 1, equipment, rigid) == equipment);
    assert(!PlayerFlexSkin_SelectLimb(plan, 1, nullptr, rigid));
    assert(PlayerFlexSkin_SelectLimb(nullptr, 1, rigid, rigid) == rigid);
    MtxF native[2]{}; for (auto& matrix : native) matrix.xx = matrix.yy = matrix.zz = matrix.ww = 1;
    native[1].xw = 1;
    MtxF result{};
    PlayerFlexSkin_BlendMatrix(plan, 0, native, &result);
    assert(result.xx == 1 && result.yy == 1 && result.zz == 1 && result.xw == 0);
    native[1].xx = native[1].yy = 0; native[1].yx = 1; native[1].xy = -1;
    PlayerFlexSkin_BlendMatrix(plan, 0, native, &result);
    assert(std::abs(result.xx * 1.f + result.xy * .2f + result.xw - .9f) < 1e-6);
    assert(std::abs(result.yx * 1.f + result.yy * .2f + result.yw - .1f) < 1e-6);
    assert(!PlayerFlexSkin_GetPlan(limbs, 3));
    assert(!PlayerFlexSkin_GetPlan(other, 2));
    // Invalid sidecars never activate weighted companions.
    auto good = Words();
    for (int type = 0; type < 10; ++type) {
        auto words = good;
        if (type == 0) words[0] = 0;
        if (type == 1) words[2] = 3;
        if (type == 2) words[3] = 255;
        if (type == 3) words.back() = 16;
        if (type == 4) words[words.size() - 2] = 2;
        if (type == 5) words.pop_back();
        if (type == 6) { words[4] = 0; words[5] = 0x7fc0; } // NaN, even with fast-math
        if (type == 7) { words[4] = 0; words[5] = 0; } // singular inverse
        if (type == 8) { words[4] = 0; words[5] = 0x7f80; } // infinity
        if (type == 9) { words[4] = 0xffff; words[5] = 0x7f7f; } // oversized finite
        resources[base + "Palette"] = Palette(words);
        assert(!PlayerFlexSkin_GetPlan(limbs, 2));
    }
    resources[base + "Palette"] = Palette(good);
    // Alt lookup must agree with the actual skeleton pointer, not only its alias.
    auto altRig = Rig(other);
    resources["alt/" + path] = altRig;
    resources["alt/" + base + "Palette"] = Palette(good);
    auto altMesh = std::make_shared<Fast::DisplayList>(); altMesh->Instructions.resize(1);
    resources["alt/" + base + "Limbs/Limb00"] = altMesh;
    alt = true;
    assert(!PlayerFlexSkin_GetPlan(limbs, 2));
    skel.skeleton = other;
    auto alternate = PlayerFlexSkin_GetPlan(other, 2);
    assert(alternate && PlayerFlexSkin_SelectLimb(alternate, 1, rigid, rigid) == altMesh->GetPointer());
    alt = false;
    assert(!PlayerFlexSkin_GetPlan(other, 2));
    skel.skeleton = limbs;
    assert(PlayerFlexSkin_GetPlan(limbs, 2));
    // A private o2r body resolves its own palette; raw PAKs have no such API path.
    privatePath = "__OTR__objects/din/winter/Skel"; privateSkeleton = other;
    resources["objects/din/winter/Skel"] = altRig;
    resources["objects/din/winter/Skel/Skin/Palette"] = Palette(good);
    resources["objects/din/winter/Skel/Skin/Limbs/Limb00"] = mesh;
    assert(PlayerFlexSkin_GetPlan(other, 2));
    privatePath = nullptr;
    assert(!PlayerFlexSkin_GetPlan(other, 2));
    std::cout << "PASS production player palette resolver/blend: bind/rest/bend, missing/invalid, Alt, private o2r, rigid/equipment/hidden fallbacks\n";
}
