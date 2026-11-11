#include "combo/vanilla/ComboVanillaFile.h"
#include "combo/vanilla/ComboVanillaRewards.h"
#include "combo/rando/StartupCheckReceiptState.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ctime>
#include <vector>
using nlohmann::json;
#define COMBO_RELEASE_VERSION "9.3.0"
namespace ComboRando { enum { GAME_OOT = 0, GAME_MM = 1 }; }
static std::filesystem::path directory;
static std::mutex g_containerMutex;
static std::map<int, json> g_containerCache;
static std::map<int, ComboVanilla::Creation> g_vanillaCreations;
static std::map<int, ComboVanilla::Receipt> g_vanillaReceipts;
static std::map<int, std::shared_ptr<ComboStartupReceipt::MmCompletion>> g_startupCompletions;
static std::map<int, uint32_t> g_startupAckRetries;
static uint32_t g_startupGeneration = 0;
static std::vector<int> g_evictedSlots;
static int g_MmSaveInMemorySlot = -1;
static int invalidations;
static void Invalidate() { ++invalidations; }
static auto MM_InvalidateOwlBlobSlot = Invalidate;
static bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
static auto ComboContainerPath(int slot) { return directory / (std::to_string(slot) + ".combosav"); }
static int32_t FinishMm(CwStartupReceipt* receipt);
static CwFinishMmStartupReceipt MM_FinishStartupReceipt = FinishMm;
static int32_t CaptureOot(int32_t slot);
static int32_t (*SOH_CaptureStartupReceiptSave)(int32_t) = CaptureOot;
static std::string g_ConsolidatedJson;
static auto SOH_LoadComboRando = +[](const char*) {};
/* HOST_FUNCTIONS */

static json mmLive, ootLive;
static int grants, captures, peerGrants, failuresBeforeCapture, peerSnapshotFailures;
static int resolvedDrawItemId;
static int32_t CaptureOot(int32_t slot) {
    assert(Combo_StartupReceipt(CW_STARTUP_PREPARE_PEER_SNAPSHOT, 0, slot, nullptr) == 1);
    if (peerSnapshotFailures > 0) {
        --peerSnapshotFailures;
        return 0; // A queued serializer failed: an earlier peer write cannot stand in for it.
    }
    Combo_WriteGameSave(0, slot, ootLive.dump().c_str());
    return 1;
}
static int32_t FinishMm(CwStartupReceipt* receipt) {
    assert(g_MmSaveInMemorySlot >= 0);
    if (receipt->state == CW_STARTUP_WAIT_MM) {
        ++grants;
        mmLive["grants"] = grants;
        mmLive["bowTier"] = mmLive.value("bowTier", 0) + 1;
        mmLive["bottles"] = mmLive.value("bottles", 0) + 1;
        receipt->getItemId = 42;
        receipt->drawItemId = resolvedDrawItemId;
        receipt->state = CW_STARTUP_READY;
        std::snprintf(receipt->itemName, sizeof(receipt->itemName), "%s", "Bow");
        std::snprintf(receipt->displayName, sizeof(receipt->displayName), "%s", "Hero's Bow");
        ++peerGrants;
        ootLive["sharedTier"] = ootLive.value("sharedTier", 0) + 1;
        ootLive["bottles"] = ootLive.value("bottles", 0) + 1;
        Combo_WriteGameSave(0, g_MmSaveInMemorySlot, ootLive.dump().c_str());
        return 1;
    }
    if (failuresBeforeCapture > 0) {
        --failuresBeforeCapture;
        throw std::runtime_error("native snapshot failed after award");
    }
    ++captures;
    Combo_WriteGameSave(1, g_MmSaveInMemorySlot, mmLive.dump().c_str());
    return 1;
}

static json ReadDisk(int slot) { json value; std::ifstream(ComboContainerPath(slot)) >> value; return value; }
static int Host(int action, int game, int slot, CwStartupReceipt* receipt = nullptr) {
    return Combo_StartupReceipt(action, game, slot, receipt);
}
static CwStartupReceipt Receipt(int game) {
    CwStartupReceipt receipt{};
    receipt.version = CW_STARTUP_RECEIPT_VERSION;
    receipt.state = game ? CW_STARTUP_WAIT_MM : CW_STARTUP_READY;
    receipt.itemGame = game;
    receipt.importance = CW_STARTUP_MAJOR;
    std::snprintf(receipt.itemName, sizeof(receipt.itemName), "%s", "Progressive Bow");
    std::snprintf(receipt.displayName, sizeof(receipt.displayName), "%s", "Fairy Bow");
    return receipt;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    directory = argv[1];
    const json original = {{"combo", {{"notes", "keep"}, {"rando", {{"seed", 17}}}}},
                           {"oot", {{"rupees", 99}}}, {"mm", {{"rupees", 25}}}};
    g_containerCache[0] = original;
    assert(FlushContainer(0));
    CwStartupReceipt found{};
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 0); // No old-save invention.
    auto absent = Receipt(0);
    absent.generation = 1;
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 1, 0, &absent) == -1);
    assert(Host(CW_STARTUP_PEEK, 0, 255, &found) == -1);
    assert(Host(CW_STARTUP_BEGIN_FILE, 1, 0) == -1);
    assert(Host(CW_STARTUP_BEGIN_FILE, 0, 0) == 1);
    auto local = Receipt(0);
    assert(Host(CW_STARTUP_CAPTURE, 0, 0, &local) == 1);
    assert(Host(CW_STARTUP_CAPTURE, 0, 0, &local) == 0);
    Combo_WriteGameSave(0, 0, original["oot"].dump().c_str());
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 1);
    auto stale = found;
    ++stale.generation;
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 0, 0, &stale) == -1);
    stale = found;
    ++stale.version;
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 0, 0, &stale) == -1);
    const auto temporary = ComboContainerPath(0).string() + ".temp";
    std::filesystem::create_directory(temporary);
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 0, 0, &found) == 0);
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 0);
    assert(Host(CW_STARTUP_PEEK, 1, 0, &found) == 0); // Visible receipt cannot replay through the peer.
    assert(ReadDisk(0)["combo"]["startupReceipt"]["shown"] == false);
    std::filesystem::remove(temporary);
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 1, 0, &found) == 1);
    g_containerCache.clear();
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 0);
    assert(ReadDisk(0)["combo"]["notes"] == "keep");

    assert(Host(CW_STARTUP_BEGIN_FILE, 0, 0) == 1);
    auto foreign = Receipt(1);
    assert(Host(CW_STARTUP_CAPTURE, 0, 0, &foreign) == 1);
    Combo_WriteGameSave(0, 0, original["oot"].dump().c_str());
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 0);
    Combo_CompleteStartupReceipt(0, true);
    assert(grants == 0); // Never award to an unrelated or uninitialized MM save.
    g_MmSaveInMemorySlot = 0;
    mmLive = original["mm"];
    ootLive = original["oot"];
    std::filesystem::create_directory(temporary);
    Combo_CompleteStartupReceipt(0, true);
    assert(grants == 1 && captures == 1);
    assert(ReadDisk(0)["mm"] == original["mm"]);
    assert(ReadDisk(0)["oot"] == original["oot"]); // No progressive peer before paired commit.
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 0 && grants == 1);
    assert(json::parse(Combo_ReadGameSave(1, 0))["grants"] == 1);
    Combo_CopyContainer(0, 1); // Copy durable pending, not an uncommitted award.
    assert(ReadDisk(1)["mm"] == original["mm"]);
    assert(ComboStartupReceipt::PendingMm(ReadDisk(1), found));
    mmLive["progress"] = 7;
    Combo_WriteGameSave(1, 0, mmLive.dump().c_str());
    ootLive["rupees"] = 123;
    Combo_WriteGameSave(0, 0, ootLive.dump().c_str());
    std::filesystem::remove(temporary);
    assert(Host(CW_STARTUP_PEEK, 1, 0, &found) == 1);
    assert(grants == 1 && captures == 1);
    assert(ReadDisk(0)["mm"] == mmLive && ReadDisk(0)["oot"]["rupees"] == 123);
    assert(ReadDisk(0)["oot"]["sharedTier"] == 1 && peerGrants == 1);
    assert(ReadDisk(0)["combo"]["notes"] == "keep");
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 1, 0, &found) == 1);
    g_containerCache.clear();
    g_startupCompletions.clear();
    assert(Host(CW_STARTUP_PEEK, 0, 0, &found) == 0 && grants == 1);

    // An award succeeded but native snapshot threw: retry only the snapshot.
    g_MmSaveInMemorySlot = 1;
    mmLive = original["mm"];
    ootLive = original["oot"];
    failuresBeforeCapture = 2;
    Combo_CompleteStartupReceipt(1, true);
    assert(grants == 2 && captures == 1);
    assert(!Combo_PreserveStartupMmState()); // A reload cannot replace the only awarded MM context.
    assert(mmLive["grants"] == 2 && grants == 2 && captures == 1);
    assert(Combo_PreserveStartupMmState());
    mmLive = json::parse(Combo_ReadGameSave(1, 1)); // Safe context replacement after the snapshot.
    assert(mmLive["grants"] == 2 && peerGrants == 2);
    assert(Host(CW_STARTUP_PEEK, 1, 1, &found) == 1);
    assert(grants == 2 && captures == 2);
    const auto oldGeneration = found.generation;
    assert(Host(CW_STARTUP_BEGIN_FILE, 0, 1) == 1);
    assert(Host(CW_STARTUP_CAPTURE, 0, 1, &local) == 1);
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 1, 1, &found) == -1);
    assert(Host(CW_STARTUP_PEEK, 0, 1, &found) == 1 && found.generation != oldGeneration);
    assert(invalidations == 1);

    // A stale/partial peer writer cannot complete the pair. Retrying the
    // explicit capture only persists the existing award, with no pickup replay.
    g_containerCache[2] = original;
    assert(Host(CW_STARTUP_BEGIN_FILE, 0, 2) == 1);
    foreign = Receipt(1);
    assert(Host(CW_STARTUP_CAPTURE, 0, 2, &foreign) == 1);
    Combo_WriteGameSave(0, 2, original["oot"].dump().c_str());
    g_MmSaveInMemorySlot = 2;
    mmLive = original["mm"];
    ootLive = original["oot"];
    peerSnapshotFailures = 1;
    Combo_CompleteStartupReceipt(2, true);
    assert(grants == 3 && peerGrants == 3);
    assert(ReadDisk(2)["oot"] == original["oot"] && ReadDisk(2)["mm"] == original["mm"]);
    assert(Combo_PreserveStartupMmState());
    assert(grants == 3 && peerGrants == 3);
    assert(ReadDisk(2)["oot"]["sharedTier"] == 1 && ReadDisk(2)["oot"]["bottles"] == 1);

    // Restart after the native award but before a failed paired disk commit.
    // Both durable inventories remain pre-award, so one recovered grant gives
    // exactly one progressive tier and one non-scalar shared bottle per side.
    assert(Host(CW_STARTUP_BEGIN_FILE, 0, 2) == 1);
    assert(Host(CW_STARTUP_CAPTURE, 0, 2, &foreign) == 1);
    Combo_WriteGameSave(0, 2, original["oot"].dump().c_str());
    Combo_WriteGameSave(1, 2, original["mm"].dump().c_str());
    mmLive = original["mm"];
    ootLive = original["oot"];
    const auto failedPair = ComboContainerPath(2).string() + ".temp";
    std::filesystem::create_directory(failedPair);
    Combo_CompleteStartupReceipt(2, true);
    assert(grants == 4 && peerGrants == 4);
    g_startupCompletions.clear();
    g_containerCache.clear();
    mmLive = json::parse(Combo_ReadGameSave(1, 2));
    ootLive = json::parse(Combo_ReadGameSave(0, 2));
    std::filesystem::remove(failedPair);
    Combo_CompleteStartupReceipt(2, true);
    assert(grants == 5 && peerGrants == 5);
    assert(ReadDisk(2)["oot"]["sharedTier"] == 1 && ReadDisk(2)["mm"]["bowTier"] == 1);
    assert(ReadDisk(2)["oot"]["bottles"] == 1 && ReadDisk(2)["mm"]["bottles"] == 1);
    assert(Host(CW_STARTUP_PEEK, 0, 2, &found) == 1);
    // If the displaying adapter goes away before its ACK can be written, any
    // later foreground poll retries persistence without presenting it again.
    std::filesystem::create_directory(failedPair);
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 0, 2, &found) == 0);
    g_containerCache.clear();
    std::filesystem::remove(failedPair);
    assert(Host(CW_STARTUP_PEEK, 1, 2, &found) == 0);
    assert(ReadDisk(2)["combo"]["startupReceipt"]["shown"] == true);
    assert(Host(CW_STARTUP_ACKNOWLEDGE, 0, 2, &found) == 1); // Original actor can finish its ACK retry.

    // An actual MM completion may carry native GID_NONE (-1). The launcher
    // must publish READY and commit the award, rather than leave WAIT behind
    // and grant the same reward again on a later matching load.
    g_containerCache[2] = original;
    assert(Host(CW_STARTUP_BEGIN_FILE, 0, 2) == 1);
    foreign = Receipt(1);
    assert(Host(CW_STARTUP_CAPTURE, 0, 2, &foreign) == 1);
    Combo_WriteGameSave(0, 2, original["oot"].dump().c_str());
    g_MmSaveInMemorySlot = 2;
    mmLive = original["mm"];
    ootLive = original["oot"];
    resolvedDrawItemId = -1;
    const int beforeSentinel = grants;
    Combo_CompleteStartupReceipt(2, true);
    assert(Host(CW_STARTUP_PEEK, 1, 2, &found) == 1 && found.drawItemId == -1);
    assert(ReadDisk(2)["combo"]["startupReceipt"]["state"] == CW_STARTUP_READY);
    g_startupCompletions.clear();
    g_containerCache.clear();
    Combo_CompleteStartupReceipt(2, true);
    assert(grants == beforeSentinel + 1 && peerGrants == grants);
    assert(Host(CW_STARTUP_PEEK, 0, 2, &found) == 1 && found.drawItemId == -1);
    std::cout << "PASS production host: paired MM commit, write/snapshot failure retries, no duplicate award, reload/copy, generation isolation and one-time receipt\n";
}
