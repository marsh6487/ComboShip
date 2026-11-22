#pragma once

#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

#if defined(PICKUP_COUNTER_WALLET_MM) && defined(COMBO_BUILD)
#include "../entrances/BlackMarketEntrance.h"
#endif

static inline bool PickupCounter_IsShopping(const PlayState* play) {
    if (play == NULL || !play->state.running || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE) {
        return false;
    }

#ifdef PICKUP_COUNTER_WALLET_MM
    switch (play->sceneId) {
        case SCENE_WITCH_SHOP:
        case SCENE_AYASHIISHOP:
        case SCENE_MILK_BAR:
        case SCENE_8ITEMSHOP:
        case SCENE_GORONSHOP:
        case SCENE_BOMYA:
#ifdef COMBO_BUILD
        case COMBO_BLACK_MARKET_SCENE:
#endif
            return true;
    }
#else
    switch (play->sceneNum) {
        case SCENE_BAZAAR:
        case SCENE_KOKIRI_SHOP:
        case SCENE_GORON_SHOP:
        case SCENE_ZORA_SHOP:
        case SCENE_POTION_SHOP_KAKARIKO:
        case SCENE_POTION_SHOP_MARKET:
        case SCENE_POTION_SHOP_GRANNY:
        case SCENE_BOMBCHU_SHOP:
        case SCENE_HAPPY_MASK_SHOP:
        case SCENE_TREASURE_BOX_SHOP:
            return true;
    }
#endif

    if (play->msgCtx.msgMode == MSGMODE_NONE || play->msgCtx.talkActor == NULL) {
        return false;
    }
    switch (play->msgCtx.talkActor->id) {
        case ACTOR_EN_OSSAN:
#ifdef PICKUP_COUNTER_WALLET_MM
        case ACTOR_EN_FSN:
        case ACTOR_EN_TRT:
        case ACTOR_EN_SELLNUTS:
        case ACTOR_EN_SCOPENUTS:
        case ACTOR_EN_AKINDONUTS:
        case ACTOR_EN_BAL:
        case ACTOR_EN_GINKO_MAN:
#else
        case ACTOR_EN_MS:
        case ACTOR_EN_DNS:
        case ACTOR_EN_DS:
        case ACTOR_EN_JS:
#endif
            return true;
        default:
            return false;
    }
}

static inline bool PickupCounter_ShouldDrawWallet(const PlayState* play) {
#ifdef PICKUP_COUNTER_WALLET_MM
    const bool hide = CVarGetInteger("gEnhancements.PickupCounter", 0) &&
                      CVarGetInteger("gEnhancements.PickupCounterHideHudRupees", 0) == 1;
#else
    const bool hide = CVarGetInteger(CVAR_ENHANCEMENT("PickupCounter"), 0) &&
                      CVarGetInteger(CVAR_ENHANCEMENT("PickupCounterHideHudRupees"), 0) == 1;
#endif
    return !hide || PickupCounter_IsShopping(play);
}
