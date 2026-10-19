#include "combo/ComboGameMode.h"
#include "combo/entrances/LostWoodsEntrance.h"
#include "combo/entrances/SoloForest.h"
#include "combo/gameplay/MagicMirrorAnchor.h"
#include "combo/gameplay/MagicMirrorArrival.h"
#include "combo/gameplay/MagicMirrorItem.h"
#include "combo/gameplay/MagicMirrorPlayer.h"
#include "global.h"
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
static bool pending, stow;
static bool alternateModel;
static int animationStarts, idleRestores;
static int mirrorDraws, matrixDepth;
static Vec3f mirrorOrigin;
static int handoffs, prompts, pickups, errors, saves;
static u32 seed = 77;
static u16 buttonItem;
static u8 textState;
static bool advance;
static int closed;
static bool noFloor, obstructed;
static float floorY;
static CollisionPoly landingFloor;
extern "C" f32 BgCheck_EntityRaycastFloor3(CollisionContext*, CollisionPoly** poly, s32* bg, Vec3f*) {
    *poly = noFloor ? nullptr : &landingFloor;
    *bg = BGCHECK_SCENE;
    return noFloor ? BGCHECK_Y_MIN : floorY;
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext*, Vec3f*, Vec3f*, Vec3f*, CollisionPoly**,
                                       s32, s32, s32, s32, s32*) { return obstructed; }
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
bool TransitionPending() { return pending; }
bool AlternateModel() { return alternateModel; }
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
namespace ComboMagicMirrorVisual {
void Draw(PlayState *) { ++mirrorDraws; }
} // namespace ComboMagicMirrorVisual
extern "C" void Matrix_Push() { ++matrixDepth; }
extern "C" void Matrix_Pop() { --matrixDepth; }
#ifdef MIRROR_OOT
using TestMatrixMode = u8;
extern "C" void Matrix_RotateY(f32, u8) {}
#else
using TestMatrixMode = MatrixMode;
extern "C" void Matrix_RotateYF(f32, MatrixMode) {}
#endif
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, TestMatrixMode) {
  mirrorOrigin = {x, y, z};
}
extern "C" void Matrix_Scale(f32, f32, f32, TestMatrixMode) {}
extern "C" void Matrix_ReplaceRotation(MtxF *) {}
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

// Asset loading and native idle/put-away plumbing are the boundary doubles;
// the cast action, ownership checks, cancellation and travel below are real.
#ifdef MIRROR_OOT
static LinkAnimationHeader gPlayerAnim_link_magic_kaze1;
#else
static PlayerAnimationHeader gPlayerAnim_link_magic_kaze1;
#endif
void NativeIdle(Player *, PlayState *) {}
void NativeSetAction(Player *current, PlayerActionFunc action) {
  current->actionFunc = action;
  current->av2.actionVar2 = 0;
#ifdef MIRROR_OOT
  current->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
#else
  current->stateFlags1 &= ~PLAYER_STATE1_20000000;
#endif
}
void Player_Action_WaitForPutAway(Player *current, PlayState *state) {
  current->afterPutAwayFunc(state, current);
}
s32 Player_SetupWaitForPutAway(PlayState *, Player *current,
                               AfterPutAwayFunc after) {
  NativeSetAction(current, Player_Action_WaitForPutAway);
  current->afterPutAwayFunc = after;
  return stow;
}
extern "C" s16 Animation_GetLastFrame(void *) { return 55; }
#ifdef MIRROR_OOT
#undef R_UPDATE_RATE
#define R_UPDATE_RATE 3
extern "C" void LinkAnimation_AnimateFrame(PlayState *, SkelAnime *) {}
#else
extern "C" void PlayerAnimation_AnimateFrame(PlayState *, SkelAnime *) {}
#endif
#include "native_animation.inc.h"
#ifdef MIRROR_OOT
s32 Player_SetupAction(PlayState *, Player *current, PlayerActionFunc action,
                       s32) {
  NativeSetAction(current, action);
  return 1;
}
LinkAnimationHeader *Player_GetIdleAnim(Player *) {
  return &gPlayerAnim_link_magic_kaze1;
}
void func_80839FFC(Player *current, PlayState *) {
  NativeSetAction(current, NativeIdle);
  ++idleRestores;
}
void Player_AnimChangeOnceMorph(PlayState *, Player *, LinkAnimationHeader *) {}
extern "C" void LinkAnimation_Change(PlayState *, SkelAnime *skel,
                                     LinkAnimationHeader *animation, f32 speed,
                                     f32 start, f32 end, u8 mode, f32 morph) {
#else
s32 Player_SetAction(PlayState *, Player *current, PlayerActionFunc action,
                     s32) {
  NativeSetAction(current, action);
  return 1;
}
void func_80839E74(Player *current, PlayState *) {
  NativeSetAction(current, NativeIdle);
  ++idleRestores;
}
extern "C" void PlayerAnimation_Change(PlayState *, SkelAnime *skel,
                                       PlayerAnimationHeader *animation,
                                       f32 speed, f32 start, f32 end, u8 mode,
                                       f32 morph) {
#endif
  assert(animation == &gPlayerAnim_link_magic_kaze1 && mode == ANIMMODE_ONCE &&
         morph < 0);
  skel->animation = animation;
  skel->curFrame = start;
  skel->endFrame = end;
  skel->playSpeed = speed;
  skel->animLength = 56;
#ifdef MIRROR_OOT
  skel->update.link = LinkAnimation_Once;
#else
  skel->update.player = PlayerAnimation_Once;
#endif
  ++animationStarts;
}
#include "combo/gameplay/MagicMirrorPlayer.inc.c"
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
    play.state.framerateDivisor = 3;
#endif
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.actionFunc = NativeIdle;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 1;
    owned = true; blocked = false; pack = true;
    pending = stow = false;
    alternateModel = false;
    animationStarts = idleRestores = 0;
    mirrorDraws = matrixDepth = 0;
    handoffs = prompts = pickups = errors = saves = 0;
    closed = 0; advance = false; textState = TEXT_STATE_NONE;
    buttonItem = EXT_ITEM_MAGIC_MIRROR;
    noFloor = obstructed = false;
    floorY = 0;
    landingFloor = {};
    landingFloor.normal.y = 32767;
    MirrorRuntime::sArrivalPending = false;
    MirrorRuntime::sArrivalPlay = nullptr;
#ifdef MIRROR_OOT
    gComboGameMode = COMBO_GAME_OOT;
    Scene(SCENE_HYRULE_FIELD);
    gSaveContext.health = 0x30;
    gSaveContext.magic = 0x30;
#else
    gComboGameMode = COMBO_GAME_MM;
    Scene(SCENE_00KEIKOKU);
    gSaveContext.save.saveInfo.playerData.health = 0x30;
    gSaveContext.save.saveInfo.playerData.magic = 0x30;
#endif
    gSaveContext.respawn[RESPAWN_MODE_TOP].data = 1;
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {123, 456, 789};
    gSaveContext.respawn[RESPAWN_MODE_RETURN].data = 2;
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = {-4, 5, -6};
    MirrorRuntime::Reset();
}
void ConfirmCast() {
  play.state.input[0].press.button = BTN_CLEFT;
  MirrorRuntime::Update();
  play.state.input[0].press.button = BTN_A;
  play.msgCtx.choiceIndex = 0;
  advance = true;
  MirrorRuntime::Update();
  advance = false;
  play.state.input[0].press.button = 0;
}

void LoadForest(float x, float y, float z) {
    static Vec3s vertices[13];
    static CollisionPoly polys[12];
    static CollisionHeader collision;
    vertices[0] = {static_cast<s16>(x), static_cast<s16>(y), static_cast<s16>(z)};
    for (int i = 0; i < 12; ++i) {
        const float angle = i * 3.14159265359f / 6;
        vertices[i + 1] = {static_cast<s16>(std::round(x + 55 * std::cos(angle))), static_cast<s16>(y),
                           static_cast<s16>(std::round(z + 56 * std::sin(angle)))};
        polys[i] = {};
        polys[i].vtxData[0] = 0;
        polys[i].vtxData[1] = i + 1;
        polys[i].vtxData[2] = (i + 1) % 12 + 1;
        polys[i].normal.y = 32767;
    }
    collision = {};
    collision.numVertices = 13; collision.vtxList = vertices;
    collision.numPolygons = 12; collision.polyList = polys;
    play.colCtx.colHeader = &collision;
    play.roomCtx.curRoom.num = 0;
    play.transitionTrigger = TRANS_TRIGGER_END;
    gSaveContext.sceneLayer = 0;
    gSaveContext.respawnFlag = 0;
    floorY = y - 80;
#ifdef MIRROR_OOT
    Scene(COMBO_SOLO_FOREST_OOT_SCENE);
    gSaveContext.entranceIndex = COMBO_SOLO_FOREST_OOT_ENTRANCE;
    gSaveContext.cutsceneIndex = 0;
#else
    Scene(Combo_IsCombinedGame() ? SCENE_LOST_WOODS : COMBO_SOLO_FOREST_MM_SCENE);
    gSaveContext.save.entrance = Combo_IsCombinedGame() ? ENTRANCE(LOST_WOODS, 0) : COMBO_SOLO_FOREST_MM_ENTRANCE;
    gSaveContext.save.cutsceneIndex = 0;
#endif
}

void TestArrival() {
    // Queue in an outdoor departure, then resolve from the destination resource.
    for (bool combined : {false, true}) {
#ifdef MIRROR_OOT
        if (combined) continue; // Combined OoT queues on the destination MM side.
#endif
        Reset();
        if (combined) gComboGameMode = COMBO_GAME_BOTH;
        assert(MirrorRuntime::Travel(&play));
        MirrorRuntime::Destroy(); // The request survives scene arena destruction.
        LoadForest(1500, 320, -1300);
        ActorEntry original{};
        original.pos = {11, 22, 33}; original.rot.y = 123;
        const auto copy = original;
        const auto respawns = gSaveContext;
        play.linkActorEntry = &original;
        MagicMirror_PrepareArrival(&play);
        assert(play.linkActorEntry != &original);
        assert(std::memcmp(&original, &copy, sizeof(original)) == 0);
        assert(play.linkActorEntry->pos.x == 1500 && play.linkActorEntry->pos.y == 240 &&
               play.linkActorEntry->pos.z == -1420);
        assert(MagicMirrorAnchor::CanClaim({1500, 240, -1420}, {1500, 320, -1300}));
#ifdef MIRROR_OOT
        assert(play.linkActorEntry->rot.y == 0);
#else
        assert(((play.linkActorEntry->rot.y >> 7) & 0x1FF) == 0 && (play.linkActorEntry->rot.y & 127) == 127);
#endif
        assert(std::memcmp(&gSaveContext.respawn, &respawns.respawn, sizeof(respawns.respawn)) == 0);
        MagicMirror_FinishArrival(&play);
        assert(player.actionFunc == NativeIdle && idleRestores == 1);
        assert(play.linkActorEntry == &original); // Preserve legacy scene-derived doorway anchors.
        MagicMirror_FinishArrival(&play);
        assert(idleRestores == 1);
        play.linkActorEntry = &original;
        MagicMirror_PrepareArrival(&play);
        assert(play.linkActorEntry == &original); // Consumed exactly once.
    }
    // Moved geometry replaces the previous scene's cached anchor at the same address.
    Reset(); LoadForest(-377, 80, 416); assert(MirrorRuntime::Locate(&play));
    MagicMirror_QueueArrival(); LoadForest(-1500, 400, 1500);
    ActorEntry original{}; play.linkActorEntry = &original;
    MagicMirror_PrepareArrival(&play);
    assert(play.linkActorEntry != &original && play.linkActorEntry->pos.x == -1500 &&
           play.linkActorEntry->pos.z == 1380);
    // Slot/seed/mode/scene mismatches, cutscenes, absent floor and walls fail closed.
    for (int reason = 0; reason < 8; ++reason) {
        Reset(); MagicMirror_QueueArrival(); LoadForest(-377, 80, 416);
        play.linkActorEntry = &original;
        switch (reason) {
        case 0: ++gSaveContext.fileNum; break;
        case 1: ++seed; break;
        case 2: gComboGameMode = COMBO_GAME_OOT == gComboGameMode ? COMBO_GAME_MM : COMBO_GAME_OOT; break;
        case 3: Scene(0xFFFF); break;
        case 4: gSaveContext.cutsceneTrigger = 1; break;
        case 5: noFloor = true; break;
        case 6: obstructed = true; break;
        case 7: play.colCtx.colHeader = nullptr; break;
        }
        MagicMirror_PrepareArrival(&play);
        assert(play.linkActorEntry == &original && !MirrorRuntime::sArrivalPending);
        MagicMirror_FinishArrival(&play);
        assert(idleRestores == 0);
    }
    puts("PASS native stump-relative arrival, scene moves, safe fallback, cached-resource/respawn preservation and one-shot context");
}

int main() {
    TestArrival();
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
    // Confirming commits to a visible cast before changing worlds.
    assert(closed == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
    assert(animationStarts == 1 &&
           (player.stateFlags1 & MirrorRuntime::kPromptFreeze));
    player.bodyPartsPos[PLAYER_BODYPART_HEAD] = {10, 60, 30};
    MirrorRuntime::DrawWorld();
    assert(mirrorDraws ==
           0); // The item appears when the hands rise, not during the windup.
    const auto castSave = gSaveContext;
    // New presses cannot reopen the prompt or restart/skip the committed
    // motion.
    play.state.input[0].press.button = BTN_B | BTN_CLEFT;
    for (int tick = 0; tick < 80 && play.transitionTrigger == TRANS_TRIGGER_OFF;
         ++tick) {
      player.actionFunc(&player, &play);
      MirrorRuntime::Update();
      if (MagicMirrorPlayer_State(&player) != MAGIC_MIRROR_CAST_READY)
        assert(play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    assert(play.transitionTrigger == TRANS_TRIGGER_START && prompts == 1 &&
           animationStarts == 1);
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY);
    MirrorRuntime::DrawWorld();
    assert(mirrorDraws == 1 && matrixDepth == 0);
    assert(mirrorOrigin.x == 10 && mirrorOrigin.y > 60 && mirrorOrigin.z == 30);
    alternateModel = true;
    player.actor.world.pos = {100, 20, -50};
    MirrorRuntime::DrawWorld();
    assert(mirrorDraws == 2 && matrixDepth == 0);
    assert(mirrorOrigin.x == 100 && mirrorOrigin.y > 80 &&
           mirrorOrigin.z == -50);
    alternateModel = false;
    assert(std::memcmp(&castSave.respawn, &gSaveContext.respawn,
                       sizeof(castSave.respawn)) == 0);
#ifdef MIRROR_OOT
    assert(castSave.magic == gSaveContext.magic);
#else
    assert(castSave.save.saveInfo.playerData.magic ==
           gSaveContext.save.saveInfo.playerData.magic);
#endif
    MirrorRuntime::Update(); // Fade holds the raised-hands pose until scene
                             // teardown.
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY);
    MirrorRuntime::Reset();
    assert(idleRestores == 1 &&
           !(player.stateFlags1 & MirrorRuntime::kPromptFreeze));
    Reset();
    gComboGameMode = COMBO_GAME_BOTH;
    ConfirmCast();
    for (int tick = 0; tick < 80 && !MirrorRuntime::sDeparted; ++tick) {
      player.actionFunc(&player, &play);
      MirrorRuntime::Update();
    }
    assert(MirrorRuntime::sDeparted);
    MirrorRuntime::Update();
#ifdef MIRROR_OOT
    assert(handoffs == 1 && play.transitionTrigger == TRANS_TRIGGER_OFF);
#else
    assert(play.nextEntrance == ENTRANCE(LOST_WOODS, 0));
#endif
    // Native held-item put-away completes before the flourish begins.
    Reset();
    stow = true;
    assert(MagicMirrorPlayer_Begin(&play, &player));
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PUT_AWAY &&
           animationStarts == 0);
    player.actionFunc(&player, &play);
    assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING &&
           animationStarts == 1);
    MagicMirrorPlayer_End(&play, &player);
    assert(player.actionFunc == NativeIdle);
    // An external native action keeps ownership if it interrupts the cast.
    assert(MagicMirrorPlayer_Begin(&play, &player));
    NativeSetAction(&player, NativeIdle);
    MagicMirrorPlayer_End(&play, &player);
    assert(idleRestores == 1);
    // A cast is revoked if its ownership, scene safety or private context
    // changes.
    for (int reason = 0; reason < 6; ++reason) {
      Reset();
      ConfirmCast();
      assert(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING);
      switch (reason) {
      case 0:
        owned = false;
        break;
      case 1:
        player.actor.bgCheckFlags = 0;
        break;
      case 2:
        play.pauseCtx.state = 1;
        break;
      case 3:
        play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
        break;
      case 4:
        pending = true;
        break;
      case 5:
        ++seed;
        break;
      }
      MirrorRuntime::Update();
      assert(!MirrorRuntime::sCastPlay && player.actionFunc == NativeIdle);
      assert(play.transitionTrigger == TRANS_TRIGGER_OFF && handoffs == 0);
    }
    Reset();
    ConfirmCast();
    NativeSetAction(&player, NativeIdle);
    MirrorRuntime::Update();
    assert(!MirrorRuntime::sCastPlay && idleRestores == 0);
    Reset();
    stow = true;
    ConfirmCast();
    for (int tick = 0; tick < 200; ++tick)
      MirrorRuntime::Update();
    assert(!MirrorRuntime::sCastPlay && player.actionFunc == NativeIdle);
    assert(play.transitionTrigger == TRANS_TRIGGER_OFF);
    // MM's destroy hook runs after arena cleanup: never access a freed player.
    Reset();
    auto *retiredPlayer = new Player(player);
#ifdef MIRROR_OOT
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &retiredPlayer->actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &retiredPlayer->actor;
#endif
    ConfirmCast();
    delete retiredPlayer;
    MirrorRuntime::Destroy();
    assert(!MirrorRuntime::sCastPlay && idleRestores == 0);
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
    puts("PASS native mirror: committed Farore cast, overhead item, fade hold, "
         "safe cancellation/teardown, preserved magic/respawns, travel and "
         "pickup");
}
