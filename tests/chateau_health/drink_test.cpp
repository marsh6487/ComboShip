#include "include/z64item.h"
#include "mods/items/custom_bottles.h"
#include "mods/nei_save.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iterator>
using u8 = uint8_t;
using s16 = int16_t;
using s32 = int32_t;
/* NATIVE_ACTIONS */
struct PlayState {
  int playerCsIds[1]{};
  struct {
    int titleCtx{};
  } actorCtx;
} play;
PlayState *gPlayState = &play;
struct Player {
  int itemAction{}, transformation{}, unk_6AD{};
  struct {
    int actionVar2{};
  } av2;
  struct {
    float endFrame{}, animLength{};
  } skelAnime;
};
struct {
  int health = 48, healthCapacity = 48, magic = 0, magicCapacity = 48,
      magicState = 0, healthAccumulator = 0, jinxTimer = 7;
  struct {
    struct {
      struct {
        int health = 48, magic = 0;
      } playerData;
    } saveInfo;
  } save;
} gSaveContext;
NeiSaveData saved{};
extern "C" NeiSaveData *Nei_Save() { return &saved; }
bool animationFinished = true, week = false, infinite = false, hostile = false;
int magicFills = 0, emptyCalls = 0, flagSets = 0, flagUnsets = 0;
constexpr int VB_USE_BOTTLE_ITEM = 1, VB_EMPTY_BOTTLE_OI = 2,
              PLAYER_CS_ID_ITEM_BOTTLE = 0;
constexpr int MAGIC_FILL_TO_CAPACITY = 9, MAGIC_STATE_FILL = 1,
              PLAYER_FORM_DEKU = 3, PLAYER_ANIM_ADJUSTED_SPEED = 1;
constexpr int NA_SE_VO_LI_DRINK = 0, NA_SE_VO_LI_BREATH_DRINK = 0, SFX_FLAG = 0,
              MAX_HEALTH = 320, FULL_HEART_HEALTH = 16;
constexpr int RAND_INF_HAS_INFINITE_MAGIC_METER = 7,
              WEEKEVENTREG_DRANK_CHATEAU_ROMANI = 1;
int gPlayerAnim_pn_drink, gPlayerAnim_pn_drinkend,
    gPlayerAnim_link_bottle_drink_demo_wait,
    gPlayerAnim_link_bottle_drink_demo_end;
#define SET_WEEKEVENTREG(x) (week = true)
#define CHECK_WEEKEVENTREG(x) (week)
#define MAGIC_REQ(x) (x)
template <class... T> bool GameInteractor_Should(int, bool value, T...) {
  return value;
}
bool LinkAnimation_Update(PlayState *, decltype(Player::skelAnime) *) {
  return animationFinished;
}
bool PlayerAnimation_Update(PlayState *, decltype(Player::skelAnime) *) {
  return animationFinished;
}
template <class... T> bool LinkAnimation_OnFrame(T...) { return false; }
template <class... T> bool PlayerAnimation_OnFrame(T...) { return false; }
template <class... T> void func_808323C0(T...) {}
template <class... T> void func_8083C0E8(T...) {}
template <class... T> void func_8005B1A4(T...) {}
template <class... T> void func_808530E0(T...) {}
template <class... T> void func_80839E74(T...) {}
template <class... T> void Player_StopCutscene(T...) {}
template <class... T> void PlayerAnimation_Change(T...) {}
template <class... T> void Player_Anim_PlayLoopAdjusted(T...) {}
template <class... T> void Player_AnimPlayLoopAdjusted(T...) {}
template <class... T> void Player_Anim_PlayOnceMorphAdjusted(T...) {}
template <class... T> void Player_AnimChangeOnceMorphAdjusted(T...) {}
template <class... T> void Player_AnimSfx_PlayVoice(T...) {}
template <class... T> void Player_PlayVoiceSfx(T...) {}
int Play_GetCamera(PlayState *, int) { return 0; }
int fallbackGrants = 0;
void Item_Give(PlayState *, int) { ++fallbackGrants; }
int Rand_S16Offset(int, int) { return 1; }
void Health_ChangeBy(PlayState *, int) {}
void Magic_Fill(PlayState *) { ++magicFills; }
void Magic_Add(PlayState *, int type) {
  assert(type == MAGIC_FILL_TO_CAPACITY);
  ++magicFills;
}
void Player_UpdateBottleHeld(PlayState *, Player *, int item, int) {
  assert(item == ITEM_BOTTLE);
  ++emptyCalls;
}
bool Player_CheckHostileLockOn(Player *) { return hostile; }
template <class... T> void TitleCard_Clear(T...) {}
/* NATIVE_DRINK */
/* NATIVE_ENTRY */
/* NATIVE_GRANT */
bool Flags_GetRandomizerInf(int) { return infinite; }
void Flags_SetRandomizerInf(int) {
  infinite = true;
  ++flagSets;
}
void Flags_UnsetRandomizerInf(int) {
  infinite = false;
  ++flagUnsets;
}
bool RandoCapacity_InfiniteMagic() { return infinite; }
u8 ExtEquip_CapeOwned() { return 0; }
/* NATIVE_BUFF */
/* NATIVE_MAGIC */
int main() {
#ifdef HOST_MM
  const auto action = PLAYER_IA_BOTTLE_CHATEAU;
  const auto item = ITEM_CHATEAU;
#else
  const auto action = PLAYER_IA_BOTTLE_POTION_BLUE;
  const auto item = ITEM_CHATEAU_ROMANI;
#endif
  std::fill(std::begin(saved.bottleSlots), std::end(saved.bottleSlots),
            BOTTLE_SLOT_EMPTY);
  assert(Bottle_ContentItemId(BOTTLE_C_CHATEAU) == item);
#ifdef HOST_MM
  GrantFilledChateau();
  assert(fallbackGrants == 0);
#else
  assert(Bottle_GiveBottle(item));
#endif
  assert(saved.bottleSlots[0] == item);
  assert(FcBottle_OotToMm(0xB6) == 0x25 && FcBottle_MmToOot(0x25) == 0xB6);
  assert(FcBottle_OotToMm(FcBottle_MmToOot(0x25)) == 0x25);
  assert(Bottle_NetOwned() && Bottle_BottomlessOwned());
  assert(Bottle_ContentMaxUses(item) == 3);
  for (int capacity : {0, 12, 48, 96, 125})
    for (bool full : {false, true}) {
      Player player{};
      player.itemAction = action;
      gSaveContext.magicCapacity = capacity;
      gSaveContext.health = full ? 48 : 16;
      gSaveContext.magic = full ? capacity : 0;
      gSaveContext.save.saveInfo.playerData.magic = gSaveContext.magic;
      gSaveContext.healthAccumulator = 0;
      gSaveContext.magicState = 0;
      magicFills = emptyCalls = 0;
      week = false;
      animationFinished = true;
#ifndef HOST_MM
      NativeDrinkEntry(&player, &play, action);
      assert(player.unk_6AD == 4 && player.itemAction == action);
#endif
      /* RUN_NATIVE_DRINK */
      assert(magicFills == 1 && gSaveContext.healthAccumulator == 320);
#ifdef HOST_MM
      assert(week);
#endif
      animationFinished = false;
      gSaveContext.healthAccumulator = 0;
      /* RUN_NATIVE_DRINK */
      assert(emptyCalls == 1 && magicFills == 1);
      gSaveContext.magic = 2;
      gSaveContext.save.saveInfo.playerData.magic = 2;
#ifndef HOST_MM
      infinite = false;
      MmMaskWear_ActivateChateauRomani();
      assert(infinite);
#else
      infinite = false;
      assert(week);
#endif
      assert(ItemMagic_HasEnough(&play, 3) == (capacity > 0));
      ItemMagic_Consume(&play, 3);
      assert(gSaveContext.magic == 2 &&
             gSaveContext.save.saveInfo.playerData.magic == 2);
#ifndef HOST_MM
      MmMaskWear_DeactivateChateauRomani();
      assert(!infinite);
#endif
    }
#ifndef HOST_MM
  infinite = true;
  MmMaskWear_ActivateChateauRomani();
  MmMaskWear_DeactivateChateauRomani();
  assert(infinite);
#endif
  std::puts("PASS native Chateau drink at full/partial meters, fractional "
            "capacities, native refill and magic policy; bottle grant stores "
            "native ID");
}
