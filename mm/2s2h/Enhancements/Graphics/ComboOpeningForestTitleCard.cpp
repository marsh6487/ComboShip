// Reuse OoT's Lost Woods place card for ordinary ComboShip visits to MM's
// opening forest. The donor ResourceManager selects Djipi/Alt or native art.
#ifdef COMBO_BUILD
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <string>

namespace {
constexpr char kLostWoodsCard[] = "textures/g_pn_36/gLostWoodsTitleCardENGTex";
// Raw texture commands reserve odd addresses for segmented-pointer tags.
alignas(2) constexpr char kRoutedLostWoodsCard[] = "__OTR__@oot:textures/g_pn_36/gLostWoodsTitleCardENGTex";
bool sPendingOpeningTitle = false;

bool HasLostWoodsCard() {
    // A missing donor must never resolve through MM's active resource manager.
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    const auto archives = owner ? owner->GetArchiveManager() : nullptr;
    if (!archives)
        return false;
    const auto available = [&](const std::string& path) {
        return archives->HasFile(path) || archives->HasFile(path + ".meta");
    };
    return available(kLostWoodsCard) ||
           (owner->IsAltAssetsEnabled() && available(std::string("alt/") + kLostWoodsCard));
}

void QueueOpeningTitle(s8 sceneId, s8 spawnNum) {
    sPendingOpeningTitle = sceneId == SCENE_LOST_WOODS && spawnNum == 0 && ComboOpeningForest_IsExploration(gPlayState);
}

void PresentOpeningTitle() {
    if (!sPendingOpeningTitle)
        return;
    PlayState* play = gPlayState;
    if (!play || gGameState != &play->state || !play->state.running || !ComboOpeningForest_IsExploration(play) ||
        play->transitionTrigger == TRANS_TRIGGER_START) {
        sPendingOpeningTitle = false;
        return;
    }
    // Wait for the entry fade and player start mode. Flashback/layer/story
    // eligibility is checked again above; no history flag is read or written.
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state != PAUSE_STATE_OFF || play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying ||
        Player_InCsMode(play))
        return;

    sPendingOpeningTitle = false;
    TitleCardContext* card = &play->actorCtx.titleCtx;
    if (!GameInteractor_Should(VB_SHOW_TITLE_CARD, gSaveContext.showTitleCard) || card->durationTimer ||
        card->delayTimer || card->alpha || !HasLostWoodsCard())
        return;

    // MM retains the native IA8 title-card renderer and fade. These are OoT's
    // place-card dimensions, position and delay, rather than boss-card layout.
    TitleCard_InitBossName(&play->state, card, const_cast<char*>(kRoutedLostWoodsCard), 160, 120, 144, 24);
    card->delayTimer = 20;
    card->alpha = card->intensity = 0;
}

void RegisterOpeningTitleCard() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(QueueOpeningTitle);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(PresentOpeningTitle);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { sPendingOpeningTitle = false; });
}
static RegisterShipInitFunc initFunc(RegisterOpeningTitleCard, {});
} // namespace
#endif
