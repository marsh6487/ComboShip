// Included by each host after its native player eligibility checks.
#include "ZeldaRunPoc8.h"

static_assert(3 * PLAYER_LIMB_MAX + 1 == ZeldaRun::kWordsPerFrame);

extern "C" void* ZeldaRun_Select(Player* player, void* original) {
    if (!original || !CVarGetInteger(ZELDA_RUN_CVAR, 0) || !ZeldaRun_Eligible(player) ||
        player->skelAnime.limbCount != PLAYER_LIMB_MAX ||
        !ZeldaRun::AgeAllowed(CVarGetInteger(ZELDA_RUN_AGES_CVAR, ZeldaRun::Both), ZeldaRun_Adult()) ||
        !ResourceMgr_OTRSigCheck(static_cast<char*>(original)) ||
        !ZeldaRun::IsFreeRun(static_cast<const char*>(original)))
        return original;
    // Native SkelAnime can retain this header after selection.
    static LinkAnimationHeader header = { { ZeldaRun::kFrames }, { ZeldaRun::kFrameData } };
    return &header;
}
