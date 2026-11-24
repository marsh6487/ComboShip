// Shared draw resolver, included by each host after its form and camera gates.
#include "SeasonalIdle.h"
#include <bit>
#include <string_view>

namespace {
constexpr std::string_view kAdultFall = "objects/seasonal_outfits/din/adult/autumn";
constexpr std::string_view kYoungFall = "objects/seasonal_outfits/din/child/autumn";

bool SeasonalIdle_IsFinite(float value) {
    return (std::bit_cast<uint32_t>(value) & 0x7f800000u) != 0x7f800000u;
}

Gfx* SeasonalIdle_LoadDisplayList(const std::string& path) {
    const auto display = std::dynamic_pointer_cast<Fast::DisplayList>(PlayerFlexSkin_Load(path));
    return display && display->GetPointerSize() ? display->GetPointer() : nullptr;
}
} // namespace

extern "C" void PlayerFlexSkin_LoadSeasonalIdle(PlayState* play, const void* actor, void** skeleton, Vec3s* joints,
                                                SeasonalIdleDraw* idle) {
    if (!play || play != gPlayState || !actor || actor != GET_PLAYER(play) || !skeleton || !joints)
        return;

    const Player* player = GET_PLAYER(play);
    const SkelAnime& animation = player->skelAnime;
    if (joints != animation.jointTable || player->csAction != PLAYER_CSACTION_NONE ||
        play->csCtx.state != CS_STATE_IDLE || player->getItemId != GI_NONE ||
        player->heldItemAction != PLAYER_IA_NONE || player->itemAction != PLAYER_IA_NONE || player->heldActor ||
        player->leftHandType != PLAYER_MODELTYPE_LH_OPEN || player->rightHandType != PLAYER_MODELTYPE_RH_OPEN ||
        (player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) || !PlayerFlexSkin_SeasonalIdleHostEligible(*player) ||
        animation.limbCount != PLAYER_LIMB_MAX || animation.morphWeight != 0.f || !animation.animation ||
        !ResourceMgr_OTRSigCheck(static_cast<char*>(animation.animation)))
        return;

    constexpr std::string_view kFreeIdle = "objects/gameplay_keep/gPlayerAnim_link_normal_wait_free";
    std::string_view clip(static_cast<const char*>(animation.animation));
    clip.remove_prefix(7); // The native OTR signature has already been checked.
    if (clip.starts_with("alt/"))
        clip.remove_prefix(4);
    if (clip != kFreeIdle || !SeasonalIdle_IsFinite(animation.animLength) || !SeasonalIdle_IsFinite(animation.curFrame) ||
        !(animation.animLength > 0.f && animation.animLength <= 32767.f) ||
        !(animation.curFrame >= 0.f && animation.curFrame < animation.animLength))
        return;

    const char* active = PakLoader_GetSeasonalSkeletonPath();
    const char* routed = PakLoader_GetSkinSkeletonPath(skeleton);
    if (!active || !routed || std::string_view(active) != routed)
        return;
    const std::string_view route(active);
    const bool adult = route == "__OTR__objects/seasonal_outfits/din/adult/autumn/gLinkAdultSkel";
    if (!adult && route != "__OTR__objects/seasonal_outfits/din/child/autumn/gLinkChildSkel")
        return;
    const std::string root(adult ? kAdultFall : kYoungFall);

    constexpr size_t kFrames = 89;
    constexpr size_t kWordsPerFrame = 67;
    const auto delta = std::dynamic_pointer_cast<SOH::PlayerAnimation>(
        PlayerFlexSkin_Load(root + "/Animations/FreeIdleArms"));
    if (!delta || delta->GetPointerSize() != kFrames * kWordsPerFrame * sizeof(int16_t))
        return;
    const auto i = static_cast<size_t>(animation.curFrame * kFrames / animation.animLength);
    const auto* frame = delta->GetPointer() + (i < kFrames ? i : kFrames - 1) * kWordsPerFrame;
    for (int joint = 0; joint < SEASONAL_IDLE_ARM_COUNT; ++joint) {
        const auto j = 3 * (SEASONAL_IDLE_FIRST_ARM + joint);
        idle->rotations[joint] = { frame[j], frame[j + 1], frame[j + 2] };
    }
    idle->active = true;

    if (!adult)
        return;
    const char* limbs[] = { "Limb15", "Limb18" };
    const char* sides[] = { "Left", "Right" };
    for (int hand = 0; hand < 2; ++hand) {
        idle->rigid[hand] = SeasonalIdle_LoadDisplayList(root + "/IdleHands/" + limbs[hand]);
        // Require a rigid alternative before choosing its weighted equivalent:
        // matrix overflow can force the same draw back to the rigid list.
        if (!idle->rigid[hand])
            continue;
        idle->weighted[hand] = SeasonalIdle_LoadDisplayList(root + "/gLinkAdultSkel/Skin/IdleHands/" + limbs[hand]);
        const std::string normal = root + "/gLinkAdult" + sides[hand] + "Hand";
        idle->ordinary[hand][0] = SeasonalIdle_LoadDisplayList(normal + "NearDL");
        idle->ordinary[hand][1] = SeasonalIdle_LoadDisplayList(normal + "FarDL");
        idle->ordinary[hand][2] = PakLoader_GetSeasonalOpenHand(hand == 0);
    }
}
