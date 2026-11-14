#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
extern "C" {
// clang-format off
#include "z64save.h"
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "2s2h/Rando/Types.h"
#include "macros.h"
// clang-format on
SaveContext gSaveContext;
// TABLES
// UPGRADE
void Inventory_ChangeAmmo(s16 item, s16 change);
}
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define FS_TRACE(...) ((void)0)
#define MM_INV (gSaveContext.save.saveInfo.inventory)

namespace mm {
NeiSaveData *Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
static int replayGrants;
static int gFcCombo_SuppressRecord;
static bool Combo_IsVanilla() { return false; }
static bool ComboVanilla_MMFirstCycle(bool, int) { return false; }
namespace ComboTrade {
static bool AcknowledgeConsumedGrant(NeiSaveData &, int) { return false; }
} // namespace ComboTrade
namespace spdlog {
struct Logger {
  void flush() {}
};
static Logger *default_logger() {
  static Logger logger;
  return &logger;
}
} // namespace spdlog
static int FcCombo_NativeForItem(int id) {
  if (id == FCI_STICK_CAPACITY)
    return RI_OOT_PROGRESSIVE_STICK_CAPACITY;
  if (id == FCI_NUT_CAPACITY)
    return RI_OOT_PROGRESSIVE_NUT_CAPACITY;
  return id == FCI_BOMBCHU_BAG ? RI_OOT_BOMBCHU_BAG : FCI_NO_ITEM;
}
namespace Rando {
// Grant callbacks are the engine boundary for this applier test. A native
// bag receipt refills ammo; the production applier must preserve an imported
// count around that side effect while still acknowledging the receipt once.
static void GiveItem(RandoItemId item) {
  if (item == RI_OOT_BOMBCHU_BAG) {
    ++replayGrants;
    INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
    AMMO(ITEM_BOMBCHU) = 20;
  } else if (item == RI_OOT_PROGRESSIVE_STICK_CAPACITY ||
             item == RI_OOT_PROGRESSIVE_NUT_CAPACITY) {
    const bool stick = item == RI_OOT_PROGRESSIVE_STICK_CAPACITY;
    const int upgrade = stick ? UPG_DEKU_STICKS : UPG_DEKU_NUTS;
    const int ammoItem = stick ? ITEM_DEKU_STICK : ITEM_DEKU_NUT;
    Inventory_ChangeUpgrade(upgrade,
                            std::min<unsigned>(3, CUR_UPG_VALUE(upgrade) + 1));
    AMMO(ammoItem) = CUR_CAPACITY(upgrade);
  }
}
} // namespace Rando
// PRODUCTION
} // namespace mm

static const int items[] = {ITEM_DEKU_STICK, ITEM_DEKU_NUT, ITEM_BOMBCHU};
void MmReset() {
  std::memset(&gSaveContext, 0, sizeof gSaveContext);
  std::memset(MM_INV.items, ITEM_NONE, sizeof MM_INV.items);
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  gSaveContext.save.shipSaveInfo.rando
      .randoSaveOptions[RO_SHUFFLE_DEKU_STICK_BAG] = 1;
  gSaveContext.save.shipSaveInfo.rando
      .randoSaveOptions[RO_SHUFFLE_DEKU_NUT_BAG] = 1;
  Inventory_ChangeUpgrade(UPG_DEKU_STICKS, 1);
  Inventory_ChangeUpgrade(UPG_DEKU_NUTS, 1);
  Inventory_ChangeUpgrade(UPG_BOMB_BAG, 1);
  mm::replayGrants = 0;
}
void MmBag(int which, bool owned) {
  const RandoInf flag =
      which == 0 ? RANDO_INF_DEKU_STICK_BAG : RANDO_INF_DEKU_NUT_BAG;
  auto &value = gSaveContext.save.shipSaveInfo.rando.randoInf[flag / 16];
  if (owned)
    value |= 1u << (flag % 16);
  else
    value &= ~(1u << (flag % 16));
}
void MmRepair() { RandoCapacity_RepairBagAmmo(); }
void MmCapacitySnapshot(const nlohmann::json &sh) {
  mm::ApplyBombchuCapacity(sh);
}
void MmNativeBombBag(int level) { Inventory_ChangeUpgrade(UPG_BOMB_BAG, level); }
void MmSpendBombchu() { Inventory_ChangeAmmo(ITEM_BOMBCHU, -1); }
int MmIndependentCapacity() {
  return gSaveContext.save.shipSaveInfo.nei
      .comboObtained[FC_OOT_BOMBCHU_CAPACITY];
}
int MmIndependentInfinite() {
  return gSaveContext.save.shipSaveInfo.nei
      .comboObtained[FC_OOT_INFINITE_BOMBCHUS];
}
void MmLegacyReceipt() {
  gSaveContext.save.shipSaveInfo.nei.comboObtainedFc[FCI_BOMBCHU_BAG] = 1;
  mm::RepairBombchuCapacity();
}
void MmReplayReceipt() {
  gSaveContext.save.shipSaveInfo.nei.comboObtainedFc[FCI_BOMBCHU_BAG] = 1;
  mm::ApplyFcRegistryToNatives();
}
int MmReplayGrants() { return mm::replayGrants; }
void MmReplayCapacity(int which) {
  auto &nei = gSaveContext.save.shipSaveInfo.nei;
  const int id = which == 0 ? FCI_STICK_CAPACITY : FCI_NUT_CAPACITY;
  nei.comboObtainedFc[id] = 1;
  mm::ApplyFcRegistryToNatives();
}
int MmTier(int which) {
  return CUR_UPG_VALUE(which == 0 ? UPG_DEKU_STICKS : UPG_DEKU_NUTS);
}
void MmSetTier(int which, int tier) {
  Inventory_ChangeUpgrade(which == 0 ? UPG_DEKU_STICKS : UPG_DEKU_NUTS, tier);
}
bool MmHasBag(int which) {
  return RandoCapacity_HasBag(which == 0 ? UPG_DEKU_STICKS : UPG_DEKU_NUTS);
}
void MmSet(int which, int ammo, bool owned) {
  INV_CONTENT(items[which]) = owned ? items[which] : ITEM_NONE;
  AMMO(items[which]) = ammo;
}
int MmAmmo(int which) { return AMMO(items[which]); }
void MmExtract(nlohmann::json &inv, int which) {
  const char *names[] = {"stick", "nut", "bombchu"};
  mm::PutInvItem(inv, names[which], SLOT(items[which]), true);
}
void MmApply(const nlohmann::json &inv, int which) {
  const char *names[] = {"stick", "nut", "bombchu"};
  mm::ApplyInvItem(inv, names[which], SLOT(items[which]), items[which], true);
}
