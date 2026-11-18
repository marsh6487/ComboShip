#include "global.h"
#include "mods/items/custom_bottles.h"
#include "mods/nei_save.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <iterator>
SaveContext gSaveContext{};
PlayState *gPlayState;
static NeiSaveData state{};
extern "C" NeiSaveData *Nei_Save(void) { return &state; }
static int iconReloads[8]{};
extern "C" void Interface_LoadItemIcon1(PlayState *, u16 button) {
  assert(button < 8);
  ++iconReloads[button];
}
#ifdef BOTTOMLESS_MM
extern "C" void Interface_LoadItemIcon(PlayState *, u8 button) {
  assert(button < 4);
  ++iconReloads[button];
}
extern "C" void Interface_LoadItemIconImpl(PlayState *, u8 button) {
  assert(button < 4);
  ++iconReloads[button];
}
extern "C" void Interface_Dpad_LoadItemIconImpl(PlayState *, u8 button) {
  assert(button < 4);
  ++iconReloads[button + 4];
}
extern "C" void Interface_StartBottleTimer(s16, s16) {}
#define INV gSaveContext.save.saveInfo.inventory.items
#define SET_C_SLOT(button, slot) C_SLOT_EQUIP(0, button) = slot
#define C_ITEM(button) BUTTON_ITEM_EQUIP(0, button)
#else
#define INV gSaveContext.inventory.items
#define SET_C_SLOT(button, slot)                                               \
  gSaveContext.equips.cButtonSlots[(button)-1] = slot
#define C_ITEM(button) gSaveContext.equips.buttonItems[button]
#define BUTTON_STATUS_INDEX(button) (button)
#endif
/* COUNTERS */
/* SYNC */
/* DRIVER */
#ifndef BOTTOMLESS_MM
constexpr int VB_EMPTY_BOTTLE_TO_HALF_MILK = 1, VB_UPDATE_BOTTLE_ITEM = 2;
static int chateauActivations;
extern "C" void MmMaskWear_ActivateChateauRomani(void) { ++chateauActivations; }
bool GameInteractor_Should(int behavior, bool result, ...) {
  if (behavior == VB_UPDATE_BOTTLE_ITEM) {
    va_list args;
    va_start(args, result);
    u8 button = va_arg(args, int);
    u8 item = va_arg(args, int);
    va_end(args);
    BottomlessCounterGate(button, item, &result);
  }
  return result;
}
#else
void GameInteractor_ExecuteOnBottleContentsUpdate(u8 item) {
  BottleItems_OnBottleUpdate(item);
}
#endif
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
/* UPDATE */
/* AMMO */
static void Bind(int button, bool dpad, int slot, int item) {
#ifdef BOTTOMLESS_MM
  if (dpad) {
    DPAD_SLOT_EQUIP(0, button) = slot;
    DPAD_SET_CUR_FORM_BTN_ITEM(button, item);
  } else {
    SET_C_SLOT(button, slot);
    C_ITEM(button) = item;
  }
#else
  SET_C_SLOT(button, slot);
  C_ITEM(button) = item;
#endif
}
static int Item(int button, bool dpad) {
#ifdef BOTTOMLESS_MM
  return dpad ? DPAD_GET_CUR_FORM_BTN_ITEM(button) : C_ITEM(button);
#else
  return C_ITEM(button);
#endif
}
static int Ammo(int button, bool dpad) {
#ifdef BOTTOMLESS_MM
  return Interface_BottomlessAmmo(button, dpad);
#else
  return Interface_BottomlessAmmo(button);
#endif
}
static void Use(PlayState *play, int button, bool dpad) {
#ifdef BOTTOMLESS_MM
  if (dpad)
    Inventory_Dpad_UpdateBottleItem(play, ITEM_BOTTLE, button);
  else
    Inventory_UpdateBottleItem(play, ITEM_BOTTLE, button);
#else
  Inventory_UpdateBottleItem(play, ITEM_BOTTLE, button);
#endif
}
int main() {
  PlayState play{};
  gPlayState = &play;
  for (int content : {int(ITEM_POTION_BLUE), int(BC_ITEM_CHATEAU)}) {
    for (bool dpad : {false, true}) {
#ifdef BOTTOMLESS_MM
      int first = dpad ? 0 : 1, last = dpad ? 3 : 3;
#else
      int first = dpad ? 4 : 1, last = dpad ? 7 : 3;
#endif
      for (int button = first; button <= last; ++button) {
        std::memset(&gSaveContext, 0, sizeof(gSaveContext));
        state = {};
        state.bottomlessBottleMode = 1;
        Bottle_BottomlessFill(content);
        const int total = Bottle_BottomlessCount();
        assert(total >= 3);
        INV[SLOT_BOTTLE_4] = content;
        INV[SLOT_BOTTLE_1] = content;
        // Ordinary duplicate-content bottle must never inherit charges or a
        // badge.
        Bind(2, false, SLOT_BOTTLE_1, content);
        Bind(button, dpad, SLOT_BOTTLE_4, content);
        assert(Ammo(button, dpad) == total);
        for (int remaining = total - 1; remaining >= 0; --remaining) {
          Use(&play, button, dpad);
          assert(Bottle_BottomlessCount() == remaining);
          for (int repeat = 0; repeat < 3; ++repeat)
            BottleItems_SyncSlot(&play, SLOT_BOTTLE_4,
                                 Bottle_BottomlessContent());
          const int want = remaining ? content : ITEM_BOTTLE;
          assert(INV[SLOT_BOTTLE_4] == want && Item(button, dpad) == want);
          assert(Ammo(button, dpad) == remaining &&
                 Bottle_BottomlessCount() == remaining);
          assert(INV[SLOT_BOTTLE_1] == content);
          if (button != 2 || dpad)
            assert(C_ITEM(2) == content && Ammo(2, false) == -1);
        }
        Bottle_BottomlessFill(content);
        Bottle_BottomlessSetCount(2);
        INV[SLOT_BOTTLE_4] = content;
        Bind(button, dpad, SLOT_BOTTLE_1, content);
        BottleItems_SyncSlot(&play, SLOT_BOTTLE_4, content);
        assert(Bottle_BottomlessCount() == 2 && Item(button, dpad) == content &&
               Ammo(button, dpad) == -1);
        Bind(button, dpad, SLOT_BOTTLE_4, ITEM_BOTTLE);
        BottleItems_SyncSlot(&play, SLOT_BOTTLE_4, content);
        assert(Bottle_BottomlessCount() == 2 && Item(button, dpad) == content &&
               Ammo(button, dpad) == 2);
        // Drinking another slot with identical contents spends no Bottomless
        // use.
        Bind(2, false, SLOT_BOTTLE_1, content);
        INV[SLOT_BOTTLE_1] = content;
        Use(&play, 2, false);
        assert(INV[SLOT_BOTTLE_1] == ITEM_BOTTLE &&
               Bottle_BottomlessCount() == 2);
        state.bottomlessBottleMode = 0;
        assert(Ammo(button, dpad) == -1);
      }
    }
  }
  state.bottomlessBottleMode = 1;
  assert(Ammo(-1, false) == -1 && Ammo(0, false) == -1 &&
         Ammo(99, false) == -1);
#ifdef BOTTOMLESS_MM
  assert(Ammo(-1, true) == -1 && Ammo(99, true) == -1);
#endif
  std::cout << "PASS actual bottle-use counters, C/D-pad identity, depletion, "
               "duplicate contents, re-equip and HUD bounds\n";
}
