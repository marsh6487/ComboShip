#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include "global.h"
#include <fast/resource/type/DisplayList.h>
#ifdef PLAYER_SKIN_MM
#include "2s2h/resource/type/PlayerAnimation.h"
#else
#include "soh/resource/type/PlayerAnimation.h"
#endif

namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData>) {}
IResource::~IResource() = default;
}
namespace SOH {
int16_t* PlayerAnimation::GetPointer() { return limbRotData.data(); }
size_t PlayerAnimation::GetPointerSize() { return limbRotData.size() * sizeof(int16_t); }
}
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() { return Instructions.data(); }
size_t DisplayList::GetPointerSize() { return Instructions.size() * sizeof(Gfx); }
}

static std::map<std::string, std::shared_ptr<Ship::IResource>> resources;
static bool hostEligible = true, alt = false;
static const char* active;
static const char* routed;
static void** loadedSkeleton;
static Gfx openLeft[1], openRight[1];
static s32 loads;
PlayState* gPlayState;
static bool PlayerFlexSkin_SeasonalIdleHostEligible(const Player&) { return hostEligible; }
extern "C" const char* PakLoader_GetSeasonalSkeletonPath(void) { return active; }
extern "C" const char* PakLoader_GetSkinSkeletonPath(void** skeleton) {
    return skeleton == loadedSkeleton ? routed : nullptr;
}
extern "C" Gfx* PakLoader_GetSeasonalOpenHand(u8 left) { return left ? openLeft : openRight; }
extern "C" int ResourceMgr_OTRSigCheck(char* name) { return std::strncmp(name, "__OTR__", 7) == 0; }
static std::shared_ptr<Ship::IResource> PlayerFlexSkin_Load(const std::string& path) {
    ++loads;
    if (alt && resources.contains("alt/" + path))
        return resources.at("alt/" + path);
    return resources.contains(path) ? resources.at(path) : nullptr;
}
#include "combo/actors/SeasonalIdleBridge.inc"

int main() {
    PlayState play{};
    Player local{}, remote{};
    gPlayState = &play;
#ifdef PLAYER_SKIN_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &local.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &local.actor;
#endif
    constexpr auto adult = "__OTR__objects/seasonal_outfits/din/adult/autumn/gLinkAdultSkel";
    constexpr auto young = "__OTR__objects/seasonal_outfits/din/child/autumn/gLinkChildSkel";
    constexpr auto spring = "__OTR__objects/seasonal_outfits/din/adult/spring/gLinkAdultSkel";
    constexpr auto freeIdle = "__OTR__objects/gameplay_keep/gPlayerAnim_link_normal_wait_free";
    const std::string adultRoot = "objects/seasonal_outfits/din/adult/autumn";
    const std::string youngRoot = "objects/seasonal_outfits/din/child/autumn";
    auto payload = std::make_shared<SOH::PlayerAnimation>();
    payload->limbRotData.resize(89 * 67);
    payload->limbRotData[42] = 111;
    payload->limbRotData[44] = 222;
    payload->limbRotData[59] = -333;
    payload->limbRotData[44 * 67 + 42] = 444; // native frame 36 of 72
    payload->limbRotData[88 * 67 + 42] = 555;
    resources[adultRoot + "/Animations/FreeIdleArms"] = payload;
    resources[youngRoot + "/Animations/FreeIdleArms"] = payload;
    auto mesh = std::make_shared<Fast::DisplayList>();
    auto weighted = std::make_shared<Fast::DisplayList>();
    auto altMesh = std::make_shared<Fast::DisplayList>();
    mesh->Instructions.resize(1);
    weighted->Instructions.resize(1);
    altMesh->Instructions.resize(1);
    resources[adultRoot + "/IdleHands/Limb15"] = mesh;
    resources["alt/" + adultRoot + "/IdleHands/Limb15"] = altMesh;
    resources[adultRoot + "/gLinkAdultSkel/Skin/IdleHands/Limb15"] = weighted;
    // Missing rigid hand on the right cannot opt into weighted drawing.
    resources[adultRoot + "/gLinkAdultSkel/Skin/IdleHands/Limb18"] = weighted;

    void* skeleton[21]{}, *other[21]{};
    loadedSkeleton = skeleton;
    Vec3s joints[22]{}, pauseJoints[22]{};
    active = routed = adult;
    auto& anim = local.skelAnime;
    anim.jointTable = joints;
    anim.limbCount = PLAYER_LIMB_MAX;
    anim.animLength = 89.f;
    anim.animation = const_cast<char*>(freeIdle);
    local.leftHandType = PLAYER_MODELTYPE_LH_OPEN;
    local.rightHandType = PLAYER_MODELTYPE_RH_OPEN;
    auto resolve = [&](const void* actor = nullptr, void** rig = nullptr, Vec3s* table = nullptr) {
        SeasonalIdleDraw data{};
        PlayerFlexSkin_LoadSeasonalIdle(&play, actor ? actor : &local.actor, rig ? rig : skeleton,
                                        table ? table : joints, &data);
        return data;
    };
    auto pass = resolve();
    assert(pass.active && pass.rotations[0].x == 111 && pass.rotations[0].z == 222 && pass.rotations[5].z == -333);
    assert(pass.rigid[0] == mesh->GetPointer() && pass.weighted[0] == weighted->GetPointer() && !pass.weighted[1]);
    assert(pass.ordinary[0][2] == openLeft);
    alt = true;
    assert(resolve().rigid[0] == altMesh->GetPointer());
    alt = false;
    anim.curFrame = 88.f;
    assert(resolve().rotations[0].x == 555);
    anim.curFrame = 36.f;
    anim.animLength = 72.f;
    assert(resolve().rotations[0].x == 444);
    active = routed = young;
    assert(resolve().active && resolve().rigid[0] == nullptr);
    active = routed = adult;

    const auto prior = loads;
    assert(!resolve(&remote.actor).active);
    assert(!resolve(nullptr, other).active);
    assert(!resolve(nullptr, nullptr, pauseJoints).active);
    active = nullptr;
    assert(!resolve().active);
    active = routed = spring;
    assert(!resolve().active);
    active = routed = adult;
    local.rightHandType = PLAYER_MODELTYPE_RH_CLOSED;
    assert(!resolve().active);
    local.rightHandType = PLAYER_MODELTYPE_RH_OPEN;
    local.itemAction = -1;
    assert(!resolve().active);
    local.itemAction = PLAYER_IA_NONE;
    local.heldItemAction = -1;
    assert(!resolve().active);
    local.heldItemAction = PLAYER_IA_NONE;
    local.stateFlags1 = PLAYER_STATE1_CARRYING_ACTOR;
    assert(!resolve().active);
    local.stateFlags1 = 0;
    local.csAction = 1;
    assert(!resolve().active);
    local.csAction = PLAYER_CSACTION_NONE;
    local.getItemId = 1;
    assert(!resolve().active);
    local.getItemId = GI_NONE;
    hostEligible = false;
    assert(!resolve().active);
    hostEligible = true;
    anim.animation = const_cast<char*>("__OTR__objects/gameplay_keep/gPlayerAnim_link_normal_walk_free");
    assert(!resolve().active);
    anim.animation = const_cast<char*>(freeIdle);
    anim.morphWeight = .25f;
    assert(!resolve().active);
    anim.morphWeight = 0.f;
    anim.curFrame = std::numeric_limits<float>::quiet_NaN();
    assert(!resolve().active);
    anim.curFrame = 0.f;
    assert(loads == prior); // gates are evaluated before any optional loading
    payload->limbRotData.pop_back();
    assert(!resolve().active);
    resources[adultRoot + "/Animations/FreeIdleArms"] = mesh;
    assert(!resolve().active);
    std::puts("PASS Fall idle resolver: phase, Alt Assets, missing/invalid payload, exact rig, local/gear gates");
}
