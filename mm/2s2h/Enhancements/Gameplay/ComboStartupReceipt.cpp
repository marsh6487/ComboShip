#ifdef COMBO_BUILD
#include "global.h"
#include "ComboExport.h"
#include "rando/StartupCheckReceipt.h"
#include "vanilla/ComboVanillaMode.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/ShipInit.hpp"
#include "mods/items/helpers/equip_helper.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cstring>

void CustomItem00_Destroy(Actor* actor, PlayState* play);

namespace {
// The save/receipt ABI uses cross-world game IDs: OoT=0, MM=1.
constexpr int kMmGame = 1;
CwStartupReceiptHost sHost = nullptr;
struct ReceiptState {
    CwStartupReceipt value{};
    PlayState* play = nullptr;
    Actor* actor = nullptr;
    int slot = -1;
    int scene = -1;
    int room = -1;
    uint32_t seed = 0;
    s16 token = 0;
    bool presented = false;
    bool acknowledged = false;
} sReceipt;
unsigned sSerial = 0;

bool Active() {
    return sHost && Combo_IsCombinedGame() && !Combo_IsVanilla() && IS_RANDO && gSaveContext.fileNum <= 2;
}
bool Valid(const CwStartupReceipt& value) {
    return value.version == CW_STARTUP_RECEIPT_VERSION && value.generation && value.state == CW_STARTUP_READY &&
           (value.itemGame == 0 || value.itemGame == kMmGame) && value.importance >= CW_STARTUP_JUNK &&
           value.importance <= CW_STARTUP_MAJOR && value.itemName[0] && value.displayName[0] &&
           std::memchr(value.itemName, 0, sizeof(value.itemName)) &&
           std::memchr(value.displayName, 0, sizeof(value.displayName)) &&
           (value.itemGame != kMmGame || Rando::StaticData::Items.contains(static_cast<RandoItemId>(value.getItemId)));
}
bool IdentityMatches() {
    return Active() && sReceipt.value.generation && sReceipt.slot == gSaveContext.fileNum &&
           sReceipt.seed == gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool PendingMatches() {
    if (!IdentityMatches())
        return false;
    // PEEK deliberately hides a visible receipt with a failed durable ACK,
    // including from the other game. Its actor still owns this frozen value.
    if (sReceipt.presented)
        return true;
    CwStartupReceipt current{};
    return sHost(CW_STARTUP_PEEK, kMmGame, sReceipt.slot, &current) == 1 && Valid(current) &&
           std::memcmp(&current, &sReceipt.value, sizeof(current)) == 0;
}
void Acknowledge() {
    if (!sReceipt.presented || sReceipt.acknowledged)
        return;
    const int result = sHost(CW_STARTUP_ACKNOWLEDGE, kMmGame, sReceipt.slot, &sReceipt.value);
    if (result == 1)
        sReceipt.acknowledged = true;
    else if (result < 0)
        sReceipt.value.generation = 0; // Host rejected this generation/context permanently.
}
bool ContextMatches(PlayState* play) {
    return Active() && play && gPlayState == play && gGameState == &play->state && play->state.running &&
           sReceipt.play == play && sReceipt.slot == gSaveContext.fileNum && sReceipt.scene == play->sceneId &&
           sReceipt.room == play->roomCtx.curRoom.num &&
           sReceipt.seed == gSaveContext.save.shipSaveInfo.rando.finalSeed &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE;
}
bool Safe(PlayState* play) {
    if (!Active() || !play || gGameState != &play->state || !play->state.running || !GET_PLAYER(play) ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != PAUSE_STATE_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || play->csCtx.state != CS_STATE_IDLE ||
        gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying || play->msgCtx.msgMode != MSGMODE_NONE ||
        ItemInput_IsBlocked(GET_PLAYER(play), play))
        return false;
    auto* player = GET_PLAYER(play);
    if (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || player->stateFlags1 & PLAYER_STATE1_DEAD)
        return false;
    for (auto state : gSaveContext.timerStates)
        if (state != TIMER_STATE_OFF)
            return false;
    return true;
}
void GiveReceipt(Actor* actor, PlayState* play);
void DrawReceipt(Actor* actor, PlayState* play);
void ReleaseMarker(s16 token) {
    if (!GameInteractor::Instance)
        return;
    auto& current = GameInteractor::Instance->currentEvent;
    const auto* event = std::get_if<GIEventGiveItem>(&current);
    if (token && event && event->giveItem == GiveReceipt && event->param == token)
        current = GIEventNone{};
}
void Reset() {
    auto* actor = sReceipt.actor;
    ReleaseMarker(sReceipt.token);
    const bool retainVisible = sReceipt.presented && !sReceipt.acknowledged && PendingMatches();
    if (retainVisible) {
        sReceipt.actor = nullptr;
        sReceipt.token = 0;
    } else {
        sReceipt = {};
    }
    if (actor)
        Actor_Kill(actor);
}
void ReceiptDestroyed(Actor* actor, PlayState* play) {
    if (actor == sReceipt.actor) {
        ReleaseMarker(sReceipt.token);
        sReceipt.actor = nullptr;
        sReceipt.token = 0;
        if (!sReceipt.presented || sReceipt.acknowledged)
            sReceipt = {};
    }
    CustomItem00_Destroy(actor, play);
}
bool ReceiptMatches(Actor* actor, PlayState* play) {
    if (!ContextMatches(play) || !actor || actor != sReceipt.actor || !sReceipt.token ||
        CUSTOM_ITEM_PARAM != sReceipt.token)
        return false;
    if (sReceipt.acknowledged)
        return true; // Keep the same held model until the native actor releases it.
    return PendingMatches();
}
void GiveReceipt(Actor* actor, PlayState* play) {
    if (!ReceiptMatches(actor, play) || sReceipt.presented)
        return;
    CustomMessage::Entry message;
    message.textboxType = TEXTBOX_TYPE_2;
    message.msg = "You got " + std::string(sReceipt.value.displayName) + "!\nFrom " + CW_STARTUP_SOURCE + ".";
    if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE)
        CustomMessage::SetActiveCustomMessage(message.msg, message);
    else
        Notification::Emit({ .message = "You received",
                             .suffix = std::string(sReceipt.value.displayName) + " — " + CW_STARTUP_SOURCE });
    // These callbacks are presentation only: inventory, traps, story and sharing
    // already completed before the durable receipt became READY.
    sReceipt.presented = true;
    Acknowledge();
}
void DrawReceipt(Actor* actor, PlayState* play) {
    if (!ReceiptMatches(actor, play))
        return;
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    if (sReceipt.value.itemGame == kMmGame)
        Rando::DrawResolvedItem(static_cast<RandoItemId>(sReceipt.value.getItemId), RC_UNKNOWN, actor);
    else
        Rando::DrawFrozenForeignItem(sReceipt.value, actor);
}
bool ShowCutscene(const CwStartupReceipt& value) {
    if (value.itemGame == kMmGame)
        return Rando::StaticData::ShouldShowGetItemCutscene(static_cast<RandoItemId>(value.getItemId));
    const int skip = CVarGetInteger("gEnhancements.Cutscenes.SkipGetItemCutscenes", 0);
    return skip == 0 || skip < value.importance + 1;
}
void Update() {
    auto* play = gPlayState;
    // A failed disk acknowledgement must not relaunch a receipt that was
    // already visible. Its value survives native actor destruction and retries
    // independently while the same slot, seed and generation remain pending.
    if (sReceipt.presented && !sReceipt.acknowledged) {
        if (!PendingMatches()) {
            Reset();
        } else {
            Acknowledge();
            if (!sReceipt.actor) {
                if (sReceipt.acknowledged)
                    sReceipt = {};
                return;
            }
        }
    }
    if (sReceipt.actor) {
        if (!ReceiptMatches(sReceipt.actor, play))
            Reset();
        else
            return;
    }
    if (sReceipt.presented || !Safe(play) || !GameInteractor::Instance || !GameInteractor::Instance->events.empty() ||
        !std::holds_alternative<GIEventNone>(GameInteractor::Instance->currentEvent))
        return;
    CwStartupReceipt value{};
    if (sHost(CW_STARTUP_PEEK, kMmGame, gSaveContext.fileNum, &value) != 1 || !Valid(value))
        return;
    const bool full = ShowCutscene(value);
    const s16 flags = CustomItem::HIDE_TILL_OVERHEAD | CustomItem::KEEP_ON_PLAYER |
                      (full ? CustomItem::GIVE_ITEM_CUTSCENE : CustomItem::GIVE_OVERHEAD);
    const s16 token = static_cast<s16>(sSerial = sSerial % 32766 + 1);
    auto* player = GET_PLAYER(play);
    auto* actor = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ITEM00, player->actor.world.pos.x,
                              player->actor.world.pos.y, player->actor.world.pos.z, flags, 0, token, ITEM00_NOTHING);
    if (!actor)
        return;
    reinterpret_cast<EnItem00*>(actor)->actionFunc = reinterpret_cast<EnItem00ActionFunc>(GiveReceipt);
    actor->draw = DrawReceipt;
    actor->destroy = ReceiptDestroyed;
    sReceipt = { value,
                 play,
                 actor,
                 gSaveContext.fileNum,
                 play->sceneId,
                 play->roomCtx.curRoom.num,
                 gSaveContext.save.shipSaveInfo.rando.finalSeed,
                 token,
                 false,
                 false };
    GameInteractor::Instance->currentEvent = GIEventGiveItem{ full, token, GiveReceipt, DrawReceipt };
}
void Register() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>(Update);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16) { Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16, s8) { Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(Reset);
}
RegisterShipInitFunc init(Register, {});
} // namespace
extern "C" COMBO_EXPORT void MM_SetStartupReceiptHost(CwStartupReceiptHost host) {
    Reset();
    sHost = host;
}
#endif
