#include "assets/soh_assets.h"
#include "global.h"
#include "mods/transformation_masks/mm_mask_wear.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "textures/do_action_static/do_action_static.h"
#include <cassert>
#include <cstdio>
#include <cstring>

SaveContext gSaveContext{};
u32 gUpgradeMasks[8]{};
u8 gUpgradeShifts[8]{};
uintptr_t gSegments[16]{};
static PlayState play;
static Player player;
static GameInfo registers;
GameInfo *gGameInfo = &registers;
static GraphicsContext graphics;
static Gfx commands[64];
PlayState *gPlayState = &play;
static char *actionSegments[4];
static s32 sCurrentMmMask = ITEM_MM_MASK_BLAST;
static s32 sBlastMaskCooldown;
static bool sBlastARequested;
static bool sBlastBRequested;
#define BLAST_MASK_COOLDOWN 310
static bool instant, spawnFails;
static int explosions, scales;
static EnBom bomb;
static s16 sFloorType;
static f32 D_80854784[] = {120.0f, 240.0f, 360.0f};
static bool submerged;

extern "C" int32_t CVarGetInteger(const char *key, int32_t fallback) {
  return std::strcmp(key, "gMods.BlastMask.Instant") == 0 ? instant : fallback;
}
extern "C" s32 MmMaskWear_GetCurrent(void) { return sCurrentMmMask; }
extern "C" MmPlayerTransformation MmForm_GetCurrentForm(void) {
  return MM_PLAYER_FORM_HUMAN;
}
u8 GaroForm_VanillaWantsAButton(Player *) { return 0; }
u8 GerudoMhr_LOwnsB(void) { return 0; }
extern "C" u8 Message_GetState(MessageContext *context) {
  return context->msgMode == MSGMODE_NONE ? TEXT_STATE_NONE : TEXT_STATE_DONE;
}
extern "C" s32 Player_ActionToMagicSpell(Player *, s32) { return -1; }
extern "C" s32 Player_CheckHostileLockOn(Player *current) {
  return (current->stateFlags1 & PLAYER_STATE1_HOSTILE_LOCK_ON) != 0;
}
extern "C" s32 Player_IsChildWithHylianShield(Player *) { return 0; }
extern "C" s32 Player_HoldsTwoHandedWeapon(Player *) { return 0; }
extern "C" s32 Player_IsFDHoldingSword(Player *) { return 0; }
extern "C" s32 func_80834B5C(Player *, PlayState *) { return 0; }
extern "C" void Player_Action_8084370C(Player *, PlayState *) {}
s32 CustomItems_HasStowableHeldItem(Player *) { return 0; }
extern "C" s32 func_8002DD78(Player *) { return 0; }
static s32 func_808332B8(Player *) { return submerged; }
static s32 func_8083A0D4(Player *) { return 0; }
static s32 Player_CanThrowCarriedActor(Player *, Actor *) { return 0; }
static void Player_Action_8084E3C4(Player *, PlayState *) {}
static void Player_Action_8084D3E4(Player *, PlayState *) {}
static void Player_Action_Talk(Player *, PlayState *) {}
static void Player_SetupActionPreserveAnimMovement(PlayState *, Player *current,
                                                  PlayerActionFunc action, s32) {
  current->actionFunc = action;
}
extern "C" void Message_StartTextbox(PlayState *current, u16, Actor *) {
  current->msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
}
extern "C" void Interface_SetNaviCall(PlayState *, u16) {}
extern "C" void Interface_LoadActionLabel(InterfaceContext *, u16, s16);
extern "C" void BlastTest_DrawB(PlayState *);
extern "C" int BlastTest_BIconCount;
extern "C" void GameInteractor_ExecuteOnSetDoAction(u16) {}
extern "C" Actor *Actor_Spawn(ActorContext *context, PlayState *current, s16 id,
                              f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz,
                              s16 params) {
  assert(context == &play.actorCtx && current == &play && id == ACTOR_EN_BOM);
  assert(rx == 0 && ry == 0 && rz == 0 && params == BOMB_BODY);
  assert(x == 123.0f && y == 456.0f && z == -789.0f);
  if (spawnFails)
    return nullptr;
  ++explosions;
  bomb = {};
  bomb.actor.world.pos = {x, y, z};
  // Actor_Spawn runs EnBom_Init, whose Collider_SetJntSph binds this storage.
  bomb.explosionCollider.elements = bomb.explosionColliderItems;
  bomb.explosionCollider.count = 1;
  return &bomb.actor;
}
extern "C" void Actor_SetScale(Actor *actor, f32 scale) {
  assert(actor == &bomb.actor);
  actor->scale = {scale, scale, scale};
  ++scales;
}

/* BLAST_HELPERS */
/* BLAST_UPDATE */
/* INTERFACE */
/* PLAYER_INTERFACE */

static void Reset() {
  play = {};
  player = {};
  graphics = {};
  registers = {};
  play.state.gfxCtx = &graphics;
  for (int language = 0; language < 3; ++language) {
    WREG(37 + language) = 100;
    R_B_LABEL_X(language) = 160;
    R_B_LABEL_Y(language) = 20;
  }
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  play.interfaceCtx.doActionSegment = actionSegments;
  play.interfaceCtx.unk_1F0 = 0xFFFF;
  player.actor.category = ACTORCAT_PLAYER;
  player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
  player.actor.world.pos = {123.0f, 456.0f, -789.0f};
  player.doorType = PLAYER_DOORTYPE_NONE;
  player.getItemId = GI_NONE;
  player.heldItemAction = PLAYER_IA_NONE;
  player.controlStickDirections[0] = PLAYER_STICK_DIR_NONE;
  sCurrentMmMask = ITEM_MM_MASK_BLAST;
  sBlastMaskCooldown = 0;
  sBlastARequested = false;
  sBlastBRequested = false;
  instant = spawnFails = submerged = false;
  explosions = scales = 0;
  gSaveContext = {};
  gSaveContext.linkAge = LINK_AGE_ADULT;
}

static Input Press(u16 buttons) {
  Input raw{};
  raw.cur.button = raw.press.button = buttons;
  return raw;
}
static Input Tick(Input raw, bool clearContext = false) {
  play.state.input[0] = raw;
  Input filtered = raw;
  TransformMasks_FilterB(&filtered);
  Player_UpdateInterface(&play, &player);
  if (clearContext) {
    // UpdateCommon clears these before the later worn-mask updater runs.
    player.stateFlags2 = 0;
    player.interactRangeActor = nullptr;
  }
  if (sCurrentMmMask == ITEM_MM_MASK_BLAST)
    MmMaskWear_UpdateBlast(&play, &player);
  return filtered;
}
static void AssertDetonation() {
  assert(explosions == 1 && scales == 1);
  assert(bomb.timer == 1 && bomb.actor.scale.x == 0.01f);
  const auto &center =
      bomb.explosionCollider.elements[0].dim.worldSphere.center;
  assert(center.x == 123 && center.y == 456 && center.z == -789);
}

static const char *DrawB() {
  const char *segments[] = {"native A current", "native A pending / B label",
                            "native third segment", "native fourth segment"};
  for (int i = 0; i < 4; ++i)
    actionSegments[i] = const_cast<char *>(segments[i]);
  graphics.overlay.p = commands;
  BlastTest_DrawB(&play);
  assert(graphics.overlay.p < commands + ARRAY_COUNT(commands));
  for (int i = 0; i < 4; ++i)
    assert(actionSegments[i] == segments[i] && "B drawing must preserve A transition textures");
  const char *texture = nullptr;
  for (Gfx *command = commands; command < graphics.overlay.p; ++command) {
    if ((command->words.w0 >> 24) == G_SETTIMG) {
      assert(texture == nullptr && "B must emit exactly one label texture");
      texture = reinterpret_cast<const char *>(command->words.w1);
    }
  }
  return texture;
}

static void AssertBTexture(const char *expected) {
  const char *texture = DrawB();
  assert(texture != nullptr && std::strcmp(texture, expected) == 0);
  assert(BlastTest_BIconCount == 0);
}

int main() {
  Reset();
  Input simultaneous = Press(BTN_A | BTN_B);
  play.state.input[0] = simultaneous;
  TransformMasks_FilterB(&simultaneous);
  assert((simultaneous.press.button & BTN_A) && !(simultaneous.press.button & BTN_B));
  assert(sBlastBRequested && "B starts as an eligible request before the A action");
  player.actor.textId = 0x1234;
  Player_SetupTalk(&play, &player);
  assert(player.stateFlags1 & PLAYER_STATE1_TALKING);
  assert(play.msgCtx.msgMode == MSGMODE_TEXT_DISPLAYING);
  MmMaskWear_UpdateBlast(&play, &player);
  assert(explosions == 0 && "An A interaction begun this frame must suppress the queued B blast");
  player.stateFlags1 = 0;
  play.msgCtx.msgMode = MSGMODE_NONE;
  MmMaskWear_UpdateBlast(&play, &player);
  assert(explosions == 0 && "A discarded request must not detonate after the interaction ends");

  Reset();
  gSaveContext.buttonStatus[0] = BTN_DISABLED;
  assert(DrawB() == nullptr && BlastTest_BIconCount == 1 &&
         "A disabled native B button must not advertise Explode");
  Tick(Press(BTN_B));
  assert(explosions == 0 && "A disabled native B button must not detonate");
  Reset();
  Input restricted = Press(BTN_B);
  play.state.input[0] = restricted;
  TransformMasks_FilterB(&restricted);
  assert(sBlastBRequested);
  gSaveContext.buttonStatus[0] = BTN_DISABLED;
  MmMaskWear_UpdateBlast(&play, &player);
  assert(explosions == 0 && "A newly disabled B button must discard its queued request");
  gSaveContext.buttonStatus[0] = BTN_ENABLED;
  Tick(Press(0));
  assert(explosions == 0);

  Reset();
  auto input = Tick(Press(BTN_A | BTN_R));
  assert((input.cur.button & BTN_A) && (input.press.button & BTN_A) &&
         "Blast Mask must leave the native A action available");
  assert(input.cur.button & BTN_R);
  assert(explosions == 0 && "A must never detonate the Blast Mask");
  assert(play.interfaceCtx.unk_1F0 != 0x1F &&
         "Blast Mask must never advertise Explode on the A button");
  for (u8 locale : {u8(LANGUAGE_ENG), u8(LANGUAGE_FRA), u8(LANGUAGE_GER),
                    u8(LANGUAGE_JPN)}) {
    Reset();
    gSaveContext.language = locale;
    AssertBTexture("__OTR__do_action_static/gDoActionExplodeENGTex");
    play.interfaceCtx.unk_1FA = 1;
    // An existing native B action keeps priority over the mask label.
    AssertBTexture("native A pending / B label");
  }
  Reset();
  sBlastMaskCooldown = 2;
  assert(DrawB() == nullptr && BlastTest_BIconCount == 1);
  Reset();
  sCurrentMmMask = ITEM_MM_MASK_STONE;
  assert(DrawB() == nullptr && BlastTest_BIconCount == 1);
  Reset();
  play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
  assert(DrawB() == nullptr && BlastTest_BIconCount == 1);
  for (u8 locale : {u8(LANGUAGE_ENG), u8(LANGUAGE_FRA), u8(LANGUAGE_GER),
                    u8(LANGUAGE_JPN)}) {
    gSaveContext.language = locale;
    Interface_LoadActionLabel(&play.interfaceCtx, 0x1F, 0);
    assert(std::strcmp(actionSegments[0],
                       "__OTR__do_action_static/gDoActionExplodeENGTex") == 0);
    Interface_LoadActionLabel(&play.interfaceCtx, DO_ACTION_SPEAK, 1);
    const char *suffix[] = {"ENGTex", "GERTex", "FRATex", "JPNTex"};
    assert(std::strstr(actionSegments[1], suffix[locale]));
  }
  for (u16 unavailable : {u16(0x1D), u16(0x1E), u16(0xFFFF)}) {
    Interface_LoadActionLabel(&play.interfaceCtx, unavailable, 0);
    assert(std::strcmp(actionSegments[0], gEmptyTexture) == 0);
  }
  Reset();
  input = Tick(Press(BTN_B | BTN_R));
  assert(!(input.cur.button & BTN_B) && !(input.press.button & BTN_B));
  assert(input.cur.button & BTN_R);
  AssertDetonation();
  assert(sBlastMaskCooldown == 310);
  assert(play.state.input[0].press.button &
         BTN_B); // no mutation of shared raw controls
  Input heldB = Press(BTN_B);
  heldB.press.button = 0;
  for (int i = 0; i < 309; ++i)
    Tick(heldB);
  assert(sBlastMaskCooldown == 1 && explosions == 1);
  Tick(Press(BTN_B));
  assert(sBlastMaskCooldown == 310 && explosions == 2);

  Reset();
  input = Tick(Press(BTN_B));
  assert(!(input.press.button & BTN_B));
  AssertDetonation();
  Reset();
  instant = true;
  input = Tick(Press(BTN_A | BTN_B));
  assert(input.press.button & BTN_A);
  AssertDetonation();
  assert(sBlastMaskCooldown == 1);
  Tick(Press(BTN_A));
  assert(explosions == 1 && sBlastMaskCooldown == 0);
  Tick(Press(BTN_B));
  assert(explosions == 2);
  Tick(Press(BTN_B));
  assert(explosions == 3);
  Reset();
  spawnFails = true;
  Tick(Press(BTN_B));
  assert(explosions == 0 && sBlastMaskCooldown == 0);

  // Higher-priority native prompts own both the label and A input.
  struct Offer {
    u32 flags1, flags2;
    u16 label;
  };
  const Offer offers[] = {
      {0, PLAYER_STATE2_DO_ACTION_GRAB, DO_ACTION_GRAB},
      {0, PLAYER_STATE2_DO_ACTION_CLIMB, DO_ACTION_CLIMB},
      {0, PLAYER_STATE2_DO_ACTION_ENTER, DO_ACTION_ENTER},
      {PLAYER_STATE1_HANGING_OFF_LEDGE, 0, DO_ACTION_DOWN},
  };
  for (const auto &offer : offers) {
    Reset();
    player.stateFlags1 = offer.flags1;
    player.stateFlags2 = offer.flags2;
    input = Tick(Press(BTN_A), true);
    assert(play.interfaceCtx.unk_1F0 == offer.label);
    assert(input.press.button & BTN_A);
    assert(explosions == 0);
  }
  Reset();
  player.doorType = PLAYER_DOORTYPE_HANDLE;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 == DO_ACTION_OPEN &&
         (input.press.button & BTN_A) && explosions == 0);
  for (u8 category : {u8(ACTORCAT_NPC), u8(ACTORCAT_PROP)}) {
    Reset();
    Actor npc{};
    npc.category = category;
    player.talkActor = &npc;
    player.stateFlags2 = PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER;
    input = Tick(Press(BTN_A), true);
    assert(play.interfaceCtx.unk_1F0 ==
           (category == ACTORCAT_NPC ? DO_ACTION_SPEAK : DO_ACTION_CHECK));
    assert((input.press.button & BTN_A) && explosions == 0);
  }
  Reset();
  Actor rock{};
  player.interactRangeActor = &rock;
  input = Tick(Press(BTN_A), true);
  assert(play.interfaceCtx.unk_1F0 == DO_ACTION_GRAB &&
         (input.press.button & BTN_A) && explosions == 0);
  Reset();
  player.interactRangeActor = &rock;
  player.getItemId = -GI_RUPEE_GREEN;
  input = Tick(Press(BTN_A), true);
  assert(play.interfaceCtx.unk_1F0 == DO_ACTION_OPEN &&
         (input.press.button & BTN_A) && explosions == 0);
  Reset();
  player.heldActor = &rock;
  player.stateFlags1 = PLAYER_STATE1_CARRYING_ACTOR;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 == DO_ACTION_DROP &&
         (input.press.button & BTN_A) && explosions == 0);
  Reset();
  EnHorse horse{};
  player.rideActor = &horse.actor;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 == DO_ACTION_CLIMB &&
         (input.press.button & BTN_A) && explosions == 0);
  Reset();
  horse.action = ENHORSE_ACT_MOUNTED_WALK;
  player.rideActor = &horse.actor;
  player.stateFlags1 = PLAYER_STATE1_ON_HORSE;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 == DO_ACTION_FASTER &&
         (input.press.button & BTN_A) && explosions == 0);

  // Availability restrictions never advertise or consume A.
  for (u32 state :
       {u32(PLAYER_STATE1_FIRST_PERSON), u32(PLAYER_STATE1_IN_WATER),
        u32(PLAYER_STATE1_INPUT_DISABLED), u32(PLAYER_STATE1_DEAD),
        u32(PLAYER_STATE1_TALKING), u32(PLAYER_STATE1_GETTING_ITEM),
        u32(PLAYER_STATE1_IN_ITEM_CS), u32(PLAYER_STATE1_IN_CUTSCENE)}) {
    Reset();
    player.stateFlags1 = state;
    input = Tick(Press(BTN_A));
    assert(play.interfaceCtx.unk_1F0 != 0x1F && (input.press.button & BTN_A) &&
           explosions == 0);
  }
  Reset();
  player.stateFlags1 = PLAYER_STATE1_SHIELDING;
  input = Tick(Press(BTN_B | BTN_R));
  AssertDetonation();
  assert(player.stateFlags1 & PLAYER_STATE1_SHIELDING);
  assert(input.press.button & BTN_R);
  Reset();
  sBlastMaskCooldown = 2;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 != 0x1F && (input.press.button & BTN_A) &&
         explosions == 0);
  Reset();
  play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
  Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 != 0x1F && explosions == 0);
  Reset();
  play.pauseCtx.state = 6;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 != 0x1F && (input.press.button & BTN_A) &&
         explosions == 0);
  Reset();
  sCurrentMmMask = ITEM_MM_MASK_STONE;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 != 0x1F && (input.press.button & BTN_A) &&
         explosions == 0);
  for (s8 action :
       {s8(PLAYER_IA_OCARINA_FAIRY), s8(PLAYER_IA_OCARINA_OF_TIME)}) {
    Reset();
    player.itemAction = action;
    input = Tick(Press(BTN_A));
    assert(play.interfaceCtx.unk_1F0 != 0x1F && (input.press.button & BTN_A) &&
           explosions == 0);
  }
  Reset();
  player.stateFlags1 = PLAYER_STATE1_START_CHANGING_HELD_ITEM;
  input = Tick(Press(BTN_A));
  assert(play.interfaceCtx.unk_1F0 != 0x1F && (input.press.button & BTN_A) &&
         explosions == 0);
  // B must obey gameplay ownership as well. The legacy raw-pad path could
  // detonate while B belonged to a textbox or blocked player state.
  for (u32 state :
       {u32(PLAYER_STATE1_INPUT_DISABLED), u32(PLAYER_STATE1_DEAD),
        u32(PLAYER_STATE1_TALKING), u32(PLAYER_STATE1_GETTING_ITEM),
        u32(PLAYER_STATE1_IN_ITEM_CS), u32(PLAYER_STATE1_IN_CUTSCENE),
        u32(PLAYER_STATE1_FIRST_PERSON), u32(PLAYER_STATE1_IN_WATER)}) {
    Reset();
    player.stateFlags1 = state;
    Tick(Press(BTN_B));
    assert(explosions == 0);
  }
  Reset();
  play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
  Tick(Press(BTN_B));
  assert(explosions == 0);
  Reset();
  play.pauseCtx.state = 6;
  Tick(Press(BTN_B));
  assert(explosions == 0);
  Reset();
  player.textboxBtnCooldownTimer = 1;
  Tick(Press(BTN_B));
  assert(explosions == 0);
  Reset();
  sCurrentMmMask = ITEM_MM_MASK_STONE;
  input = Tick(Press(BTN_B));
  assert((input.press.button & BTN_B) && explosions == 0);

  puts("PASS OoT B-only Blast Mask, native B HUD commands and A interactions, "
       "late context/disabled-B ownership, blast collider and normal/instant cooldown");
}
