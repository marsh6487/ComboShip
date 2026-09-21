#include "rando/SharedItems.h"
#include "rando/CrossWorldRando.h"
#include <mutex>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <algorithm>
static int g_comboCompletionSlot = -1, g_MmSaveInMemorySlot = -1;
static uint32_t g_sharedMask = 0;
static bool g_sharedReconcilePending = false;
static int tiers[2][ComboRando::SF_COUNT] = {}, raises = 0;
static bool ComboIsValidSlot(int slot) {
    return slot >= 0 && slot < 3;
}
static int getOot(int f) {
    return tiers[0][f];
}
static int getMm(int f) {
    return tiers[1][f];
}
static void raiseOot(int f, int tier) {
    ++raises;
    tiers[0][f] = tier;
}
static void raiseMm(int f, int tier) {
    ++raises;
    tiers[1][f] = std::min(tier, ComboRando::SharedFamilyByIndex(f).mmTierCap);
}
static auto SOH_GetSharedTier = getOot;
static auto MM_GetSharedTier = getMm;
static auto SOH_RaiseSharedTier = raiseOot;
static auto MM_RaiseSharedTier = raiseMm;
static void (*SOH_GetCurrentPlayerName)(unsigned char*) = nullptr;
static void (*MM_SetCheckPrices)(const char*) = nullptr;
static int saveResult = -1;
static int initSave(int, const char*, const unsigned char*) {
    return saveResult;
}
static auto MM_InitRandoSaveFile = initSave;
static int crossGrants = 0;
static void grantOot(const char*) {
    ++crossGrants;
    tiers[0][ComboRando::SF_GORON_MASK] = 1;
}
static void grantMm(const char*) {
    ++crossGrants;
}
static void (*SOH_GrantCrossItem)(const char*) = grantOot;
static void (*MM_GrantCrossItem)(const char*) = grantMm;
static void Combo_OnTriforceProgress(int, int) {
}
namespace ComboAnchor {
static std::atomic<bool> sResyncPending{ false };
static std::atomic<int> sActiveGame{ 0 };
} // namespace ComboAnchor
static void (*SOH_Anchor_RequestResync)() = nullptr;
static void (*MM_Anchor_RequestResync)() = nullptr;
static int pumpFamily = ComboRando::SF_GORON_MASK;
static void pumpOot() {
    tiers[0][pumpFamily] = 1;
}
static void pumpMm() {
    tiers[1][pumpFamily] = 1;
}
static void (*SOH_Anchor_PumpDormant)() = pumpOot;
static void (*MM_Anchor_PumpDormant)() = pumpMm;
// This file is produced from the real launcher functions at test execution; no implementation copy.
#include "combo_shared_runtime_functions.h"
static void check(bool ok, const char* what) {
    if (!ok)
        throw std::runtime_error(what);
}
int main() {
    g_comboCompletionSlot = 1;
    g_sharedMask = 1u << ComboRando::SF_BOW;
    for (int slot : { -1, 0, 2, 3, 0xFE, 0xFF }) {
        Combo_OnSharedChanged(0, slot);
        check(!g_sharedReconcilePending, "foreign slot or sentinel must not schedule shared reconcile");
    }
    Combo_OnSharedChanged(2, 1);
    check(!g_sharedReconcilePending, "unknown game must not schedule shared reconcile");
    Combo_OnSharedChanged(0, 1);
    check(g_sharedReconcilePending, "loaded-slot pickup schedules shared reconcile");
    tiers[0][ComboRando::SF_BOW] = 1;
    g_MmSaveInMemorySlot = 0;
    Combo_SharedReconcileNow();
    Combo_SharedTick();
    check(raises == 0, "stale dormant slot must never receive an item");
    g_MmSaveInMemorySlot = 1;
    Combo_SharedTick();
    check(raises == 1 && tiers[1][ComboRando::SF_BOW] == 1, "matching dormant slot receives exactly one missing tier");
    Combo_SharedReconcileNow();
    check(raises == 1, "repeated reconciliation cannot add a progressive tier");
    g_sharedMask = 1u << ComboRando::SF_BOMBCHU_BAG;
    tiers[1][ComboRando::SF_BOMBCHU_BAG] = 1;
    Combo_SharedReconcileNow();
    check(raises == 1 && tiers[0][ComboRando::SF_BOMBCHU_BAG] == 0, "one-way family never raises OOT from MM");
    nlohmann::json seed = { { "mm", { { "placements", nlohmann::json::object() } } } };
    g_MmSaveInMemorySlot = -1;
    check(!Combo_WriteMMSaveForSlot(1, seed) && g_MmSaveInMemorySlot == -1,
          "failed MM creation must not claim residency");
    saveResult = 0;
    check(Combo_WriteMMSaveForSlot(1, seed) && g_MmSaveInMemorySlot == 1, "successful MM creation binds the real slot");
    g_sharedMask = 1u << ComboRando::SF_GORON_MASK;
    g_sharedReconcilePending = false;
    ResetCrossItemDedupForSeed(42);
    DeliverCrossItem(0, "Goron Mask", "mm:Mask Chest");
    check(crossGrants == 1 && g_sharedReconcilePending,
          "save-direct foreign native mask grant must queue shared reconcile for the launcher slot");
    Combo_SharedTick();
    check(tiers[1][ComboRando::SF_GORON_MASK] == 1, "foreign native mask reaches MM without a game transition");
    g_sharedReconcilePending = false;
    DeliverCrossItem(0, "Goron Mask", "mm:Mask Chest");
    check(crossGrants == 1 && !g_sharedReconcilePending, "duplicate cross delivery grants and schedules nothing");
    SOH_GrantCrossItem = nullptr;
    DeliverCrossItem(0, "Goron Mask", "mm:Retry Chest");
    check(crossGrants == 1 && !g_sharedReconcilePending,
          "missing grant callback must not schedule shared reconciliation");
    SOH_GrantCrossItem = grantOot;
    DeliverCrossItem(0, "Goron Mask", "mm:Retry Chest");
    check(crossGrants == 2 && g_sharedReconcilePending,
          "unavailable grant callback must not consume the check dedup key");
    tiers[0][ComboRando::SF_GORON_MASK] = tiers[1][ComboRando::SF_GORON_MASK] = 0;
    g_sharedReconcilePending = false;
    ComboAnchor::sActiveGame = 1;
    PumpDormant();
    check(g_sharedReconcilePending, "Anchor pump must schedule native shared items received by dormant OOT");
    Combo_SharedTick();
    check(tiers[1][ComboRando::SF_GORON_MASK] == 1, "dormant OOT network mask reaches active MM");
    pumpFamily = ComboRando::SF_ZORA_MASK;
    g_sharedMask = 1u << pumpFamily;
    g_sharedReconcilePending = false;
    ComboAnchor::sActiveGame = 0;
    PumpDormant();
    check(g_sharedReconcilePending, "Anchor pump must schedule native shared items received by dormant MM");
    Combo_SharedTick();
    check(tiers[0][pumpFamily] == 1, "dormant MM network mask reaches active OOT");
    g_sharedReconcilePending = false;
    MM_Anchor_PumpDormant = nullptr;
    PumpDormant();
    check(!g_sharedReconcilePending, "absent dormant pump must not schedule shared reconcile");
    std::cout << "combo shared launcher slot and cross-delivery checks passed\n";
}
