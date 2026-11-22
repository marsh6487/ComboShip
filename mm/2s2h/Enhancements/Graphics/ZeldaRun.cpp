#include "global.h"
#include "BenPort.h"
#include "mods/items/logic/adult_link_render.h"
#include "mods/forms/custom_forms.h"
#include "2s2h/expansions/sm64/sm64_mario.h"
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" u8 WolfLinkForm_IsSelected(void);
extern "C" int ResourceMgr_OTRSigCheck(char* path);

static bool ZeldaRun_Adult() {
    return AdultLink_IsActive() != 0;
}
static bool ZeldaRun_Eligible(Player* player) {
    return player && gPlayState && player == GET_PLAYER(gPlayState) && player->transformation == PLAYER_FORM_HUMAN &&
           CustomForms_ActiveForm() == CUSTOM_FORM_NONE && !WolfLinkForm_IsSelected() && !Sm64Mario_ShouldHideLink();
}
#include "../../../../combo/actors/ZeldaRunBridge.inc"
