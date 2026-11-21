#include "combo/hud/PickupCounterWallet.h"
#include <assert.h>
#include <string.h>

SaveContext gSaveContext;
static PlayState play;
static Actor merchant;
static int pickupEnabled = 1;
static int hideWallet = 1;

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (strcmp(name, "gEnhancements.PickupCounter") == 0) {
        return pickupEnabled;
    }
    if (strcmp(name, "gEnhancements.PickupCounterHideHudRupees") == 0) {
        return hideWallet;
    }
    return fallback;
}

int main(void) {
    play.state.running = true;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
#ifdef PICKUP_COUNTER_WALLET_MM
    play.sceneId = SCENE_WITCH_SHOP;
#else
    play.sceneNum = SCENE_POTION_SHOP_GRANNY;
#endif
    assert(PickupCounter_ShouldDrawWallet(&play));
    play.transitionTrigger = TRANS_TRIGGER_START;
    assert(!PickupCounter_ShouldDrawWallet(&play));
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    play.state.running = false;
    assert(!PickupCounter_ShouldDrawWallet(&play));
    play.state.running = true;
#ifdef PICKUP_COUNTER_WALLET_MM
#ifdef COMBO_BUILD
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    assert(PickupCounter_ShouldDrawWallet(&play));
#endif
    play.sceneId = SCENE_20SICHITAI2;
    merchant.id = ACTOR_EN_AKINDONUTS;
#else
    play.sceneNum = SCENE_HYRULE_FIELD;
    merchant.id = ACTOR_EN_DNS;
#endif
    assert(!PickupCounter_ShouldDrawWallet(&play));
    play.msgCtx.talkActor = &merchant;
    assert(!PickupCounter_ShouldDrawWallet(&play));
    play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
    assert(PickupCounter_ShouldDrawWallet(&play));
    merchant.id = ACTOR_EN_BOX;
    assert(!PickupCounter_ShouldDrawWallet(&play));
    hideWallet = 0;
    assert(PickupCounter_ShouldDrawWallet(&play));
    hideWallet = 1;
    pickupEnabled = 0;
    assert(PickupCounter_ShouldDrawWallet(&play));
}
