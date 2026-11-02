#include "global.h"
#include "combo/vanilla/ComboVanillaRewards.h"
#include "2s2h/Rando/Types.h"
#include "2s2h/FleetShipCombo/FleetComboIds.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/items/custom_bottles.h"
#include "mods/items/logic/twilight_upgrade.h"
#include "entrances/LostWoodsEntrance.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_set>

using namespace ComboVanilla;
#define SPDLOG_ERROR(...) ((void)0)
SaveContext gSaveContext{};
PlayState* gPlayState;
u32 gBitFlags[32];
/* ITEM_SLOTS */
static bool vanilla = true;
static int writes, stageCalls;
static bool failBegin, failCommit, failCapture;
static nlohmann::json document;
static Receipt receipt;
static bool Combo_IsVanilla() {
    return vanilla;
}
extern "C" s32 CVarGetInteger(const char*, s32 fallback) {
    return fallback;
}
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {
}
extern "C" void ItemGrantAudit_End() {
}
extern "C" void ExtEquip_ResyncFromSave() {
}
u8 ExtInv_GetOotSlotItem(int) {
    assert(false);
    return ITEM_NONE;
}
void ExtInv_SetOotSlotItem(int, u8) {
    assert(false);
}
extern "C" s32 TradeAdult_OwnedCount() {
    assert(false);
    return 0;
}
extern "C" u8 TradeAdult_CellItem() {
    assert(false);
    return ITEM_NONE;
}
/* NATIVE_GRANTS */
/* ADAPTER_FUNCTIONS */

static nlohmann::json NativeSnapshot() {
    return { { "type", "2S2H_SAVE" },
             { "version", 6 },
             { "newCycleSave", { { "save", { { "shipSaveInfo", { { "saveType", 0 } } } } } } },
             { "testRupees", gSaveContext.save.saveInfo.playerData.rupees },
             { "testBottomless", Nei_Save()->bottomlessBottleMode },
             { "testMirror", Nei_Save()->magicMirrorOwned } };
}
static int Host(RewardAction action, const Reward& reward, int) {
    switch (action) {
        case RewardAction::Query:
            return receipt.Active() ? 2 : RewardStatus(document, reward);
        case RewardAction::Begin:
        case RewardAction::BeginOwned:
            return !failBegin && receipt.Begin(document, reward, action == RewardAction::BeginOwned, [](const auto&) {
                ++writes;
                return true;
            });
        case RewardAction::PrepareCapture:
            receipt.PrepareCapture();
            return 1;
        case RewardAction::Commit:
            return receipt.Commit(document, [](const auto&) {
                ++writes;
                return !failCommit;
            });
        case RewardAction::Abort:
            receipt.Abort();
            return 1;
    }
    return -1;
}
void SaveManager_SaveCurrentForCombo(bool preserveSession) {
    assert(preserveSession);
    ++stageCalls;
    if (!failCapture)
        assert(receipt.Stage(1, NativeSnapshot()));
}
/* TRANSACTION_FUNCTION */
/* MIRROR_CLAIM */
namespace MirrorRuntime {
static int sPickupToken;
static bool contextMatches = true, mode = true;
static bool Matches(PlayState*) {
    return contextMatches;
}
static bool Mode() {
    return mode;
}
/* MIRROR_FINISH */
} // namespace MirrorRuntime

static void EmptySave() {
    gSaveContext = {};
    gSaveContext.fileNum = 0;
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    gSaveContext.save.saveInfo.playerData.rupees = 99;
    gSaveContext.save.saveInfo.playerData.health = 0x50;
    gSaveContext.save.saveInfo.playerData.healthCapacity = 0x80;
    std::fill(std::begin(gSaveContext.save.saveInfo.inventory.items),
              std::end(gSaveContext.save.saveInfo.inventory.items), ITEM_NONE);
    std::fill(std::begin(Nei_Save()->ownedItems), std::end(Nei_Save()->ownedItems), ITEM_NONE);
}
static void EmptyDocument() {
    document = { { "combo",
                   { { "mode", "vanilla-nei" },
                     { "vanilla", { { "version", 1 }, { "collected", nlohmann::json::object() } } } } } };
    document["mm"] = NativeSnapshot();
    receipt.Abort();
    writes = stageCalls = 0;
    failBegin = failCommit = failCapture = false;
}

int main() {
    for (unsigned i = 0; i < 32; ++i)
        gBitFlags[i] = 1u << i;
    unsigned rows = 0;
    for (const auto& reward : kRewards) {
        if (reward.game != Game::Mm)
            continue;
        ++rows;
        assert(HasPresentation(reward));
        assert(std::any_of(std::begin(kBindings), std::end(kBindings), [&](const auto& binding) {
            return reward.location == std::string_view(binding.location) && binding.entrance < 32 && binding.room >= 0;
        }));
        EmptySave();
        auto nativeBefore = gSaveContext.save.saveInfo;
        assert(!Owns(reward));
        assert(Grant(reward) && Owns(reward));
        assert(std::memcmp(&nativeBefore, &gSaveContext.save.saveInfo, sizeof(nativeBefore)) == 0);
        const auto neiBefore = *Nei_Save();
        assert(Grant(reward));
        assert(std::memcmp(&neiBefore, Nei_Save(), sizeof(neiBefore)) == 0);
        if (reward.kind == RewardKind::Cane)
            assert(Nei_Save()->caneSkills == (1u << reward.payload));
        if (reward.kind == RewardKind::Wand)
            assert(Nei_Save()->wandRodsOwned == (1u << reward.payload));
        if (reward.kind == RewardKind::Rune)
            assert(Nei_Save()->slateRunesOwned == (1u << reward.payload));
        if (reward.kind == RewardKind::Season)
            assert(Nei_Save()->seasonsOwned == (1u << reward.payload));
        if (reward.kind == RewardKind::Item && reward.payload == 0x221)
            assert(Nei_GetOwnedItem(SLOT_PHANTOM_HOURGLASS) == 0x221);
        // Every named prerequisite has both a locked and native-earned case.
        EmptySave();
        const bool unconditional = std::string_view(reward.prerequisite) == "first_cycle";
        assert(NativePrerequisite(reward.prerequisite) == unconditional);
        gSaveContext.save.saveInfo.inventory.questItems = 0xFFFFFFFF;
        for (const auto item :
             { ITEM_MASK_DEKU, ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_HOOKSHOT, ITEM_MASK_GIBDO, ITEM_MASK_CAPTAIN })
            INV_CONTENT(item) = item;
        WEEKEVENTREG(WEEKEVENTREG_SAVED_KOUME >> 8) |= WEEKEVENTREG_SAVED_KOUME & 255;
        gSaveContext.save.isNight = 1;
        assert(NativePrerequisite(reward.prerequisite));
    }
    assert(rows == 30); // Version-one MM sources; the mislabeled legacy Scale is deferred.
    EmptySave();
    assert(Active());
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 0;
    assert(!Active());
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    vanilla = false;
    assert(!Active());
    vanilla = true;
    assert(!NativePrerequisite("unrecognized_or_randomizer_gate"));
    const auto* bottle = FindReward("vanilla.nei.bottomless_bottle");
    assert(bottle && bottle->price == 90);
    for (int failure = 0; failure < 3; ++failure) {
        EmptySave();
        EmptyDocument();
        failBegin = failure == 0;
        failCommit = failure == 1;
        failCapture = failure == 2;
        assert(!Transact(*bottle, 0));
        assert(!Nei_Save()->bottomlessBottleMode && gSaveContext.save.saveInfo.playerData.rupees == 99);
        assert(RewardStatus(document, *bottle) == 0 && !receipt.Active());
    }
    EmptySave();
    EmptyDocument();
    assert(Transact(*bottle, 0));
    assert(Nei_Save()->bottomlessBottleMode && gSaveContext.save.saveInfo.playerData.rupees == 9);
    assert(RewardStatus(document, *bottle) == 1 && document["mm"]["testRupees"] == 9);
    assert(Transact(*bottle, 0));
    assert(gSaveContext.save.saveInfo.playerData.rupees == 9 && stageCalls == 1);
    EmptySave();
    EmptyDocument();
    Bottle_SetBottomlessOwned(1);
    assert(Transact(*bottle, 0));
    assert(gSaveContext.save.saveInfo.playerData.rupees == 99);
    assert(document["combo"]["vanilla"]["collected"][bottle->checkId]["price"] == 0);
    // Authored stump delivery uses its native token/context gate and the same
    // permanent journal identity as the stock fallback.
    const auto* mirror = FindReward("vanilla.nei.magic_mirror");
    EmptySave();
    EmptyDocument();
    MirrorRuntime::sPickupToken = 44;
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 0;
    assert(!MirrorRuntime::FinishVanillaPickup(44));
    assert(!Nei_Save()->magicMirrorOwned && writes == 0);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    MirrorRuntime::sPickupToken = 45;
    assert(!MirrorRuntime::FinishVanillaPickup(44));
    MirrorRuntime::contextMatches = false;
    assert(!MirrorRuntime::FinishVanillaPickup(45));
    MirrorRuntime::contextMatches = true;
    MirrorRuntime::mode = false;
    assert(!MirrorRuntime::FinishVanillaPickup(45));
    MirrorRuntime::mode = true;
    gSaveContext.fileNum = 3;
    assert(!MirrorRuntime::FinishVanillaPickup(45));
    gSaveContext.fileNum = 0;
    assert(MirrorRuntime::FinishVanillaPickup(45));
    assert(Nei_Save()->magicMirrorOwned && RewardStatus(document, *mirror) == 1);
    assert(document["mm"]["testMirror"] == 1);
    assert(!MirrorRuntime::FinishVanillaPickup(45));
    EmptySave();
    EmptyDocument();
    failCommit = true;
    MirrorRuntime::sPickupToken = 46;
    assert(!MirrorRuntime::FinishVanillaPickup(46));
    assert(!Nei_Save()->magicMirrorOwned && !MirrorRuntime::sPickupToken);
    failCommit = false;
    MirrorRuntime::sPickupToken = 47;
    assert(MirrorRuntime::FinishVanillaPickup(47));
    assert(Nei_Save()->magicMirrorOwned && RewardStatus(document, *mirror) == 1);
    std::cout << "PASS " << rows
              << " MM reward bindings, exact real grants, native prerequisites, cycle/mode guards, atomic failures and "
                 "single debit\n";
}
