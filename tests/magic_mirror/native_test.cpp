#include "global.h"
#include "combo/ComboGameMode.h"
#include "combo/entrances/SoloForest.h"
#include "combo/entrances/LostWoodsEntrance.h"
#include "combo/gameplay/MagicMirrorItem.h"
#include "combo/gameplay/MagicMirrorAnchor.h"
#ifdef MIRROR_OOT
#include "soh/soh/FleetShipCombo/FleetComboIds.h"
#else
#include "mm/2s2h/FleetShipCombo/FleetComboIds.h"
#endif
#ifndef MIRROR_OOT
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>

extern "C" { int gComboGameMode; }
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
static PlayState play;
static Player player;
static bool owned, blocked, pack = true;
static int handoffs, prompts, pickups, errors, saves;
static u32 seed = 77;
static u16 buttonItem;
static u8 textState;
static bool advance;
static int closed;
void SetText(u16 text) {
#ifdef MIRROR_OOT
    play.msgCtx.textId = text;
#else
    play.msgCtx.currentTextId = text;
#endif
}
namespace MirrorHost {
u32 Seed() { return seed; }
bool PackAvailable() { return pack; }
bool Busy(PlayState*) { return blocked; }
bool TimedActivity() { return false; }
bool QueuePickup(int) { ++pickups; return true; }
void OpenPrompt() {
    ++prompts;
    SetText(0x3FEC);
    play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
    textState = TEXT_STATE_CHOICE;
}
bool RequestHandoff() { ++handoffs; return true; }
void Save() { ++saves; }
void Error() { ++errors; }
bool ButtonClaimed(u16) { return false; }
u16 ButtonItem(s32) { return buttonItem; }
}
namespace ComboMagicMirrorVisual { void Draw(PlayState*) {} }
extern "C" uint8_t MagicMirror_IsOwned(void) { return owned; }
extern "C" void TravelTools_Grant(uint16_t) { owned = true; }
extern "C" auto Message_GetState(MessageContext*) -> decltype(Message_GetState((MessageContext*)nullptr)) {
    return textState;
}
extern "C" auto Message_ShouldAdvance(PlayState*) -> decltype(Message_ShouldAdvance((PlayState*)nullptr)) {
    return advance;
}
extern "C" void Message_CloseTextbox(PlayState* current) {
    ++closed;
    current->msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
    textState = TEXT_STATE_NONE;
}
#ifdef MIRROR_OOT
extern "C" s32 Player_InCsMode(PlayState*) { return blocked; }
#else
extern "C" bool Player_InCsMode(PlayState*) { return blocked; }
#define COMBO_MAGIC_MIRROR_MM
#endif

#include "combo/gameplay/MagicMirrorRuntime.inc.h"

void Scene(int scene) {
#ifdef MIRROR_OOT
    play.sceneNum = scene;
#else
    play.sceneId = scene;
#endif
}
void Reset() {
    play = {}; player = {}; gSaveContext = {};
    gPlayState = &play; gGameState = &play.state;
    play.state.running = true;
#ifdef MIRROR_OOT
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 1;
    owned = true; blocked = false; pack = true;
    handoffs = prompts = pickups = errors = saves = 0;
    closed = 0; advance = false; textState = TEXT_STATE_NONE;
    buttonItem = EXT_ITEM_MAGIC_MIRROR;
#ifdef MIRROR_OOT
    gComboGameMode = COMBO_GAME_OOT;
    Scene(SCENE_HYRULE_FIELD);
    gSaveContext.health = 0x30;
#else
    gComboGameMode = COMBO_GAME_MM;
    Scene(SCENE_00KEIKOKU);
    gSaveContext.save.saveInfo.playerData.health = 0x30;
#endif
    MirrorRuntime::Reset();
}

int main() {
    assert(FcEquip_OotToMm16(EXT_ITEM_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
    assert(FcEquip_MmToOot16(EXT_ITEM_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
    assert(FcEquip_OotToMm16(EXT_ITEM_MAGIC_MIRROR + 1) == 0xFF);
    assert(MirrorRuntime::kPromptText < 0x4E20);
    Reset(); play.state.input[0].press.button = BTN_CLEFT;
    MirrorRuntime::Update();
    assert(prompts == 1 && !(play.state.input[0].press.button & BTN_CLEFT));
    assert(player.stateFlags1 & MirrorRuntime::kPromptFreeze);
    // An unsafe change must release the private choice box and its owned freeze.
    player.actor.bgCheckFlags = 0;
    MirrorRuntime::Update();
    assert(closed == 1 && play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING);
    assert(!(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
    Reset(); play.state.input[0].press.button = BTN_CLEFT; MirrorRuntime::Update();
    SetText(1234); MirrorRuntime::Update();
    assert(closed == 0); // never close another actor's replacement message
    assert(!(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
    Reset(); play.state.input[0].press.button = BTN_CLEFT; MirrorRuntime::Update();
    play.state.input[0].press.button = BTN_B; advance = true; MirrorRuntime::Update();
    assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
    Reset(); play.state.input[0].press.button = BTN_CLEFT; MirrorRuntime::Update();
    play.state.input[0].press.button = BTN_A; play.msgCtx.choiceIndex = 1; advance = true; MirrorRuntime::Update();
    assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
    Reset(); play.state.input[0].press.button = BTN_CLEFT; MirrorRuntime::Update();
    play.state.input[0].press.button = BTN_A; play.msgCtx.choiceIndex = 0; advance = true; MirrorRuntime::Update();
    assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_START);
    assert(!(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
    Reset();
    assert(MirrorRuntime::Safe(&play, false));
    assert(MirrorRuntime::AllowedScene(&play));
    const auto previousReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    const auto previousTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
#ifdef MIRROR_OOT
    gSaveContext.cutsceneIndex = 0xFFF3;
#else
    gSaveContext.save.cutsceneIndex = 0xFFF3;
#endif
    assert(MirrorRuntime::Travel(&play));
    assert(play.transitionTrigger == TRANS_TRIGGER_START);
#ifdef MIRROR_OOT
    assert(play.nextEntranceIndex == COMBO_SOLO_FOREST_OOT_ENTRANCE);
    assert(gSaveContext.cutsceneIndex == 0);
#else
    assert(play.nextEntrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    assert(gSaveContext.save.cutsceneIndex == 0);
#endif
    assert(std::memcmp(&previousReturn, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(previousReturn)) == 0);
    assert(std::memcmp(&previousTop, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(previousTop)) == 0);
    Reset(); gComboGameMode = COMBO_GAME_BOTH;
    assert(MirrorRuntime::Travel(&play));
#ifdef MIRROR_OOT
    assert(handoffs == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
#else
    assert(play.nextEntrance == ENTRANCE(LOST_WOODS, 0));
#endif
    Reset(); owned = false; assert(!MirrorRuntime::Travel(&play));
    Reset(); pack = false; assert(!MirrorRuntime::Travel(&play));
    Reset(); blocked = true; assert(!MirrorRuntime::Travel(&play));
    Reset(); player.actor.bgCheckFlags = 0; assert(!MirrorRuntime::Travel(&play));
    Reset(); play.pauseCtx.state = 1; assert(!MirrorRuntime::Travel(&play));
    Reset(); play.transitionTrigger = TRANS_TRIGGER_START; assert(!MirrorRuntime::Travel(&play));
    Reset(); play.csCtx.state = 1; assert(!MirrorRuntime::Travel(&play));
    Reset(); gSaveContext.fileNum = 0xFF; assert(!MirrorRuntime::Travel(&play));
    Reset(); gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN; assert(!MirrorRuntime::Travel(&play));
    Reset();
#ifdef MIRROR_OOT
    Scene(SCENE_DEKU_TREE);
    gComboGameMode = COMBO_GAME_MM;
#else
    Scene(SCENE_MITURIN);
    gComboGameMode = COMBO_GAME_OOT;
#endif
    assert(!MirrorRuntime::Travel(&play));
    // A stale deferred pickup never grants into another file, seed, scene, or mode.
    Reset(); owned = false; MirrorRuntime::Bind(&play);
    auto token = MirrorRuntime::BeginPickup();
    assert(token > 0);
    gSaveContext.fileNum = 2; MirrorRuntime::FinishPickup(token); assert(!owned);
    Reset(); owned = false; MirrorRuntime::Bind(&play); token = MirrorRuntime::BeginPickup();
    ++seed; MirrorRuntime::FinishPickup(token); assert(!owned);
    Reset(); owned = false; MirrorRuntime::Bind(&play); token = MirrorRuntime::BeginPickup();
    MirrorRuntime::Reset(); MirrorRuntime::FinishPickup(token); assert(!owned);
    Reset(); owned = false; MirrorRuntime::Bind(&play); token = MirrorRuntime::BeginPickup();
    MirrorRuntime::FinishPickup(token); assert(owned && saves == 1);
    MirrorRuntime::FinishPickup(token); assert(saves == 1);
    puts("PASS native mirror: valid destinations, blocked states, preserved respawns, and slot/seed-bound one-time pickup");
}
