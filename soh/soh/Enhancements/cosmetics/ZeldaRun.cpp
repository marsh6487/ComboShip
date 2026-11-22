#include "global.h"
#include "soh/ResourceManagerHelpers.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "expansions/sm64/sm64_mario.h"
#include <libultraship/bridge/consolevariablebridge.h>

static bool ZeldaRun_Adult() {
    return LINK_AGE_IN_YEARS == YEARS_ADULT;
}
static bool ZeldaRun_Eligible(Player* player) {
    // Wolf shares the transformation state machine. Its renderer can stay selected after the form ends.
    return player && gPlayState && player == GET_PLAYER(gPlayState) && !TransformMasks_IsTransformedAny() &&
           !Sm64Mario_ShouldHideLink();
}
#include "../../../../combo/actors/ZeldaRunBridge.inc"
