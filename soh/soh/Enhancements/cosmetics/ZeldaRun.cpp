#include "global.h"
#include "soh/ResourceManagerHelpers.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/transformation_masks/wolf_link_form.h"
#include "expansions/sm64/sm64_mario.h"
#include <libultraship/bridge/consolevariablebridge.h>

static bool ZeldaRun_Adult() {
    return LINK_AGE_IN_YEARS == YEARS_ADULT;
}
static bool ZeldaRun_Eligible(Player* player) {
    return player && gPlayState && player == GET_PLAYER(gPlayState) && !TransformMasks_IsTransformedAny() &&
           !WolfLinkForm_IsSelected() && !Sm64Mario_ShouldHideLink();
}
#include "../../../../combo/actors/ZeldaRunBridge.inc"
