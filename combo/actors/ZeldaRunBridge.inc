// Included by each owning host after its native resource/eligibility boundary.
#include "ZeldaRun.h"
#include "ZeldaRunPolicy.h"
#include <map>
#include <memory>

extern "C" void* ZeldaRun_Select(Player* player, void* original) {
    if (!original || !CVarGetInteger(ZELDA_RUN_CVAR, 0) || !ZeldaRun_Eligible(player) ||
        !ZeldaRun::AgeAllowed(CVarGetInteger(ZELDA_RUN_AGES_CVAR, ZeldaRun::Both), ZeldaRun_Adult()) ||
        !ResourceMgr_OTRSigCheck(static_cast<char*>(original)) ||
        !ZeldaRun::IsFreeRun(static_cast<const char*>(original)))
        return original;
    auto header = std::dynamic_pointer_cast<SOH::Animation>(ZeldaRun_Load(ZeldaRun::kHeader));
    auto data = std::dynamic_pointer_cast<SOH::PlayerAnimation>(ZeldaRun_Load(ZeldaRun::kData));
    if (!header || !data || header->type != SOH::AnimationType::Link ||
        header->animationData.linkAnimationHeader.common.frameCount != ZeldaRun::kFrames ||
        data->limbRotData.size() != ZeldaRun::kFrames * ZeldaRun::kWordsPerFrame ||
        header->animationData.linkAnimationHeader.segment != data->GetPointer())
        return original;
    // Native SkelAnime retains this header beyond the selection callback; retain
    // both resources even across asset-cache invalidation or an Alt toggle.
    using Resources = std::pair<std::shared_ptr<SOH::Animation>, std::shared_ptr<SOH::PlayerAnimation>>;
    static std::map<const void*, Resources> retained;
    retained.try_emplace(header.get(), header, data);
    static_assert(sizeof(LinkAnimationHeader) == sizeof(SOH::LinkAnimationHeader));
    return &header->animationData.linkAnimationHeader;
}
