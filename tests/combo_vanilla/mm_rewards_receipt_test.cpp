#include "global.h"
#include "combo/vanilla/ComboVanillaRewards.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <cassert>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
using namespace ComboVanilla;
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
GameInteractor* GameInteractor::Instance;
static bool active = true, safe = true, owned;
static bool Combo_IsVanilla() {
    return active;
}
extern "C" u8 ItemInput_IsBlocked(Player* player, PlayState*) {
    return !safe || (player->stateFlags1 & PLAYER_STATE1_TALKING) != 0;
}
/* ACTIVE_SAFE */
static bool NativePrerequisite(std::string_view) {
    return true;
}
static bool Owns(const Reward&) {
    return owned;
}
static int status, grants, aborts, draws, destroyed, killed, offers, closes;
static int Host(RewardAction action, const Reward&, int) {
    if (action == RewardAction::Query)
        return status;
    if (action == RewardAction::Abort)
        ++aborts;
    return 1;
}
static bool Transact(const Reward&, int) {
    ++grants;
    owned = true;
    status = 1;
    return true;
}
static std::string shown;
void CustomMessage::StartTextbox(std::string text, Entry) {
    shown = text;
}
void CustomMessage::SetActiveCustomMessage(std::string text, Entry) {
    shown = text;
}
void CustomMessage::LoadCustomMessageIntoFont(Entry entry) {
    shown = entry.msg;
}
static int messageState = TEXT_STATE_NONE;
static bool advance;
extern "C" u8 Message_GetState(MessageContext*) {
    return messageState;
}
extern "C" bool Message_ShouldAdvance(PlayState*) {
    return advance;
}
extern "C" void Message_CloseTextbox(PlayState*) {
    ++closes;
    messageState = TEXT_STATE_CLOSING;
}
extern "C" void Actor_Kill(Actor*) {
    ++killed;
}
extern "C" void Actor_SetFocus(Actor*, f32) {
}
extern "C" s32 Actor_TalkOfferAccepted(Actor* actor, GameState*) {
    const bool accepted = (actor->flags & ACTOR_FLAG_TALK) != 0;
    actor->flags &= ~ACTOR_FLAG_TALK;
    return accepted;
}
extern "C" s32 Actor_OfferTalk(Actor*, PlayState*, f32) {
    ++offers;
    return true;
}
extern "C" void CustomItem00_Destroy(Actor*, PlayState*) {
    ++destroyed;
}
extern "C" void Matrix_Scale(f32, f32, f32, MatrixMode) {
}
static void DrawReward(const Reward&, Actor*, PlayState*) {
    ++draws;
}
static std::vector<std::unique_ptr<EnItem00>> spawned;
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz,
                              s32 params) {
    auto item = std::make_unique<EnItem00>();
    item->actor.id = id;
    item->actor.world.pos = { x, y, z };
    item->actor.home.rot = { rx, ry, rz };
    item->actor.params = params;
    auto* actor = &item->actor;
    spawned.emplace_back(std::move(item));
    return actor;
}
constexpr u16 kOfferText = 0x8E80;
/* STATE */
/* FUNCTIONS */
static void OtherReceipt(Actor*, PlayState*) {
}

int main() {
    GameInteractor interactor;
    GameInteractor::Instance = &interactor;
    PlayState play{};
    Player player{};
    gPlayState = &play;
    gGameState = &play.state;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.state.running = 1;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    gSaveContext.fileNum = 0;
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    gSaveContext.save.saveInfo.playerData.rupees = 99;
    sContext = { &play, 0, play.sceneId, play.roomCtx.curRoom.num, 7 };
    const unsigned index = FindReward("vanilla.nei.net") - kRewards;
    EnItem00 pickup{};
    auto* actor = &pickup.actor;
    sPickups.emplace(actor, Pickup{ index, 7 });
    // Native open-text happens before the actor observes accepted talk.
    u16 text = kOfferText + index;
    bool table = true;
    OpenText(&text, &table);
    assert(!table && shown.find("Net") != std::string::npos);
    actor->flags |= ACTOR_FLAG_TALK;
    player.stateFlags1 |= PLAYER_STATE1_TALKING | PLAYER_STATE1_20000000;
    play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
    play.msgCtx.currentTextId = text;
    play.msgCtx.talkActor = actor;
    assert(!Safe(&play));
    PickupUpdate(actor, &play);
    assert(sPickups[actor].talking);
    play.msgCtx.currentTextId = text;
    play.msgCtx.choiceIndex = 0;
    messageState = TEXT_STATE_CHOICE;
    advance = true;
    PickupUpdate(actor, &play);
    assert(closes == 1 && grants == 0 && spawned.empty());
    // Native talk cleanup can lag the closed message by one frame.
    messageState = TEXT_STATE_NONE;
    play.msgCtx.currentTextId = 0;
    safe = false;
    play.msgCtx.msgMode = MSGMODE_NONE;
    player.stateFlags1 = 0;
    PickupUpdate(actor, &play);
    safe = true;
    PickupUpdate(actor, &play);
    assert(spawned.size() == 1 && sReceipt.actor == &spawned.back()->actor);
    auto* receiptActor = sReceipt.actor;
    GiveReceipt(receiptActor, &play);
    assert(grants == 1 && sReceipt.finished);
    DrawReceipt(receiptActor, &play);
    assert(draws == 1); // Held-up model lives until native receipt destruction.
    GiveReceipt(receiptActor, &play);
    assert(grants == 1);
    ReceiptDestroyed(receiptActor, &play);
    assert(aborts == 0 && destroyed == 1 && !sReceipt.token);
    assert(std::holds_alternative<GIEventNone>(interactor.currentEvent));
    status = 0;
    owned = false;
    assert(OfferReceipt(index, &play));
    receiptActor = sReceipt.actor;
    ++receiptActor->home.rot.z;
    GiveReceipt(receiptActor, &play);
    assert(grants == 1);
    --receiptActor->home.rot.z;
    gSaveContext.fileNum = 1;
    GiveReceipt(receiptActor, &play);
    assert(grants == 1);
    gSaveContext.fileNum = 0;
    Reset();
    assert(aborts == 1 && !sReceipt.token && sPickups.empty());
    assert(std::holds_alternative<GIEventNone>(interactor.currentEvent));
    // Reset releases only our own native scheduler reservation.
    interactor.currentEvent = GIEventGiveItem{ true, 15, OtherReceipt, OtherReceipt };
    sReceipt = { index, 0, 15, sContext.generation, nullptr };
    CancelReceipt();
    assert(std::get<GIEventGiveItem>(interactor.currentEvent).giveItem == OtherReceipt);
    interactor.events.emplace_back(GIEventGiveItem{ true, 15, OtherReceipt, OtherReceipt });
    assert(!OfferReceipt(index, &play));
    assert(interactor.events.size() == 1);
    active = false;
    PickupUpdate(actor, &play);
    assert(killed > 0);
    puts("PASS MM receipt actor ownership, delayed native close, exact slot/token, held model, duplicate callback and "
         "cancellation controls");
}
