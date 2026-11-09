#include "Rando/InfiniteUpgrades.h"
#include "Rando/Logic/Logic.h"
#include "tests/test_require.h"
#include <cstdio>
#include <cstring>
extern "C" {
#include "functions.h"
#include "variables.h"
SaveContext gSaveContext{};
PlayState *gPlayState;
static RegEditor registers{};
RegEditor *gRegEditor = &registers;
void Magic_Update(PlayState *);
void ItemMagic_Consume(PlayState *, s16);
s32 ItemMagic_HasEnough(PlayState *, s16);
}
#include "wallet_flush.inc"

int main() {
  const int items[] = {ITEM_BOW, ITEM_BOMB, ITEM_BOMBCHU, ITEM_DEKU_STICK,
                       ITEM_DEKU_NUT};
  const int upgrades[] = {UPG_QUIVER, UPG_BOMB_BAG, UPG_BOMB_BAG,
                          UPG_DEKU_STICKS, UPG_DEKU_NUTS};
  const int maxima[] = {50, 40, 40, 30, 40};
  const RandoInf flags[] = {RANDO_INF_INFINITE_BOW, RANDO_INF_INFINITE_BOMB_BAG,
                            RANDO_INF_INFINITE_BOMB_BAG,
                            RANDO_INF_INFINITE_STICKS, RANDO_INF_INFINITE_NUTS};
  for (int i = 0; i < 5; ++i) {
    gSaveContext = {};
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.inventory.items));
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_DEKU_STICK_BAG] = 1;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_DEKU_NUT_BAG] = 1;
    Flags_SetRandoInf(flags[i]);
    Inventory_ChangeAmmo(items[i], 99);
    REQUIRE(AMMO(items[i]) == 0 && CUR_UPG_VALUE(upgrades[i]) == 0);
    Inventory_ChangeUpgrade(upgrades[i], 1);
    INV_CONTENT(items[i]) = items[i];
    if (i == 3)
      Flags_SetRandoInf(RANDO_INF_DEKU_STICK_BAG);
    if (i == 4)
      Flags_SetRandoInf(RANDO_INF_DEKU_NUT_BAG);
    AMMO(items[i]) = maxima[i];
    Inventory_ChangeAmmo(items[i], -1);
    REQUIRE(AMMO(items[i]) == maxima[i]);
    REQUIRE(CUR_CAPACITY(upgrades[i]) == maxima[i]);
    REQUIRE(CUR_UPG_VALUE(upgrades[i]) == 1);
    Inventory_ChangeAmmo(items[i], -32768);
    REQUIRE(AMMO(items[i]) == maxima[i]);
    Inventory_ChangeAmmo(items[i], 32767);
    REQUIRE(AMMO(items[i]) == maxima[i]);
    Flags_ClearRandoInf(flags[i]);
    AMMO(items[i]) = 1;
    Inventory_ChangeAmmo(items[i], -1);
    REQUIRE(AMMO(items[i]) == 0);
  }
  gSaveContext = {};
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET] = 1;
  Flags_SetRandoInf(RANDO_INF_INFINITE_WALLET);
  REQUIRE(CUR_CAPACITY(UPG_WALLET) == 0);
  Rupees_ChangeBy(50);
  REQUIRE(gSaveContext.rupeeAccumulator == 0);
  Flags_SetRandoInf(RANDO_INF_WALLET_CHILD);
  REQUIRE(CUR_CAPACITY(UPG_WALLET) == 500);
  gSaveContext.save.saveInfo.playerData.rupees = 1;
  Rupees_ChangeBy(-100);
  Combo_MM_FlushAccumulators();
  REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 1);
  Rupees_ChangeBy(1000);
  Combo_MM_FlushAccumulators();
  REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 500);
  REQUIRE(CUR_UPG_VALUE(UPG_WALLET) == 0);

  static PlayState play{};
  gSaveContext = {};
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  Flags_SetRandoInf(RANDO_INF_INFINITE_MAGIC);
  gSaveContext.magicCapacity = 12; // fractional RPG meter boundary
  REQUIRE(!Magic_Consume(&play, 4, MAGIC_CONSUME_DEITY_BEAM));
  gSaveContext.save.saveInfo.playerData.isMagicAcquired = 1;
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  REQUIRE(Magic_Consume(&play, 48, MAGIC_CONSUME_DEITY_BEAM));
  REQUIRE(gSaveContext.save.saveInfo.playerData.magic == 0);
  REQUIRE(!CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI));
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  REQUIRE(Magic_Consume(&play, 0, MAGIC_CONSUME_LENS));
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  REQUIRE(Magic_Consume(&play, 0, MAGIC_CONSUME_GORON_ZORA));
  BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_LENS_OF_TRUTH;
  for (int state :
       {MAGIC_STATE_CONSUME, MAGIC_STATE_CONSUME_LENS,
        MAGIC_STATE_CONSUME_GORON_ZORA_SETUP, MAGIC_STATE_CONSUME_GORON_ZORA,
        MAGIC_STATE_CONSUME_GIANTS_MASK}) {
    gSaveContext.magicState = state;
    gSaveContext.magicToConsume = 4;
    gSaveContext.save.saveInfo.playerData.magic = 12;
    play.interfaceCtx.magicConsumptionTimer = 1;
    play.actorCtx.lensActive = true;
    Magic_Update(&play);
    REQUIRE(gSaveContext.save.saveInfo.playerData.magic == 12);
    REQUIRE(!CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI));
  }
  gSaveContext.save.saveInfo.playerData.magic = 0;
  REQUIRE(ItemMagic_HasEnough(&play, 48));
  ItemMagic_Consume(&play, 48);
  REQUIRE(gSaveContext.save.saveInfo.playerData.magic == 0);
  Flags_ClearRandoInf(RANDO_INF_INFINITE_MAGIC);
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  gSaveContext.save.saveInfo.playerData.magic = 8;
  REQUIRE(Magic_Consume(&play, 4, MAGIC_CONSUME_DEITY_BEAM));
  REQUIRE(gSaveContext.save.saveInfo.playerData.magic == 4);
  SET_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI);
  gSaveContext.magicState = MAGIC_STATE_CONSUME;
  gSaveContext.magicToConsume = 4;
  gSaveContext.save.saveInfo.playerData.magic = 12;
  Magic_Update(&play);
  REQUIRE(gSaveContext.save.saveInfo.playerData.magic == 12);
  std::puts("PASS native infinite ammo/wallet/magic, base ownership, unchanged "
            "raw tiers and Chateau isolation");
}
