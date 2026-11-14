#include "gameplay/ComboFaroresWindJson.h"
#include "vanilla/ComboVanillaFile.h"
#include "vanilla/ComboVanillaSave.h"
#include "vanilla/ComboVanillaRewards.h"
#include "rando/StartupCheckReceiptState.h"
#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#define COMBO_RELEASE_VERSION "0.3.0"
namespace ComboRando { enum { GAME_OOT, GAME_MM }; }
static std::mutex g_containerMutex;
static std::map<int, nlohmann::json> g_containerCache;
// Existing Farore controls run with no in-flight native creation or reward.
static std::map<int, ComboVanilla::Creation> g_vanillaCreations;
static std::map<int, ComboVanilla::Receipt> g_vanillaReceipts;
static std::map<int, std::shared_ptr<ComboStartupReceipt::MmCompletion>> g_startupCompletions;
static std::map<int, uint32_t> g_startupAckRetries;
static std::vector<int> g_evictedSlots;
static int g_MmSaveInMemorySlot = -1;
static void (*MM_InvalidateOwlBlobSlot)() = nullptr;
static int g_PendingMMFileNum = -1;
static int arrivalGame = -1, arrivalSlot = -1, resumeEntry = 0;
static ComboFwPoint arrival{};
static int setOot(int slot, const ComboFwPoint* p) {
    arrivalGame = 0; arrivalSlot = slot; arrival = *p; return 1;
}
static int setMm(int slot, const ComboFwPoint* p) {
    arrivalGame = 1; arrivalSlot = slot; arrival = *p; return 1;
}
static int (*SOH_SetFwArrival)(int, const ComboFwPoint*) = setOot;
static int (*MM_SetFwArrival)(int, const ComboFwPoint*) = setMm;
static void (*SOH_ResumeGame)(int) = [](int) {};
static void (*MM_ResumeGame)(int) = [](int) {};
static void (*MM_SetComboEntryIsResume)(int) = [](int value) { resumeEntry = value; };

// PRODUCTION_CONTAINER_FUNCTIONS

static void TestStartupSaveBoundaries() {
    Combo_WriteGameSave(0, 0, R"({"progress":17})");
    Combo_WriteGameSave(1, 0, R"({"progress":23})");
    ComboFwPoint point{1, 0x1230, 3, -300, -1, 111.5f, 222, -333, 55, 77, 123};
    assert(Combo_WriteFwPoint(0, &point));
    auto& document = LoadOrCreateContainer(0);
    CwStartupReceipt pending{};
    pending.version = CW_STARTUP_RECEIPT_VERSION;
    pending.state = CW_STARTUP_WAIT_MM;
    pending.itemGame = ComboRando::GAME_MM;
    std::strcpy(pending.itemName, "Bow");
    assert(ComboStartupReceipt::BeginFile(document, 7));
    assert(ComboStartupReceipt::Capture(document, pending));
    assert(FlushContainer(0));
    const auto durable = document;
    auto completion = std::make_shared<ComboStartupReceipt::MmCompletion>();
    assert(completion->Begin(document));
    completion->running = true;
    g_startupCompletions[0] = completion;
    g_startupAckRetries[0] = 7;

    // Native saves during a pending award cannot publish a partial transaction.
    Combo_WriteGameSave(1, 0, R"({"progress":99})");
    Combo_WriteGameSave(0, 0, R"({"progress":98})");
    assert(completion->nativeSave.is_null() && !completion->peerCaptureComplete);
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 0))["progress"] == 23);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 0))["progress"] == 17);
    completion->receipt.state = CW_STARTUP_READY;
    std::strcpy(completion->receipt.displayName, "Hero's Bow");
    completion->running = false;
    Combo_WriteGameSave(1, 0, R"({"progress":99})");
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 0))["progress"] == 99);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 0))["progress"] == 17);
    completion->PreparePeerCapture();
    Combo_WriteGameSave(0, 0, R"({"progress":98})");
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 0))["progress"] == 98);
    assert(document == durable);
    nlohmann::json stored;
    std::ifstream(ComboContainerPath(0)) >> stored;
    assert(stored == durable);

    // Copy carries the durable pending state and Farore point, and drops only
    // destination transient state. Source staged reads remain available.
    g_startupCompletions[2] = completion;
    g_startupAckRetries[2] = 6;
    g_MmSaveInMemorySlot = 2;
    Combo_CopyContainer(0, 2);
    assert(!g_startupCompletions.count(2) && !g_startupAckRetries.count(2));
    assert(g_startupCompletions.at(0) == completion && g_startupAckRetries.at(0) == 7);
    assert(g_MmSaveInMemorySlot == -1);
    g_containerCache.erase(2);
    auto copied = durable;
    copied["slot"] = 2;
    assert(LoadOrCreateContainer(2) == copied);
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 2))["progress"] == 23);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 2))["progress"] == 17);
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 0))["progress"] == 99);
    ComboFwPoint read{};
    assert(Combo_ReadFwPoint(2, &read) == 1 && read.game == point.game && read.x == point.x);

    // A replacement generation ignores the old completion for both read and write.
    assert(ComboStartupReceipt::BeginFile(document, 8));
    assert(ComboStartupReceipt::Capture(document, pending));
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 0))["progress"] == 23);
    Combo_WriteGameSave(1, 0, R"({"progress":55})");
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 0))["progress"] == 55);
    assert(completion->nativeSave["progress"] == 99);
    std::ifstream(ComboContainerPath(0)) >> stored;
    assert(stored["mm"]["progress"] == 55);
}

int main() {
    ComboFwPoint p{1, 0x1230, 3, -300, -1, 111.5f, 222, -333, 55, 77, 123};
    p.dungeonCheckpoint = 0x1230;
    ComboFwPoint read{};
    assert(Combo_ReadFwPoint(1, &read) == -1);
    Combo_WriteGameSave(0, 1, R"({"progress":17})");
    Combo_WriteGameSave(1, 1, R"({"progress":23})");
    assert(Combo_WriteFwPoint(1, &p));
    Combo_WriteGameSave(0, 1, R"({"progress":31})");
    g_containerCache.clear(); // Real disk round-trip, independent of cached objects.
    assert(Combo_ReadFwPoint(1, &read) == 1 && read.game == 1 && read.x == 111.5f);
    assert(read.tempCollectFlagsLow == 123 && read.dungeonCheckpoint == 0x1230);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 1))["progress"] == 31);
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 1))["progress"] == 23);
    assert(Combo_RequestFwReturn(0, 1) && arrivalGame == 1 && arrivalSlot == 1);
    assert(g_PendingMMFileNum == 1 && resumeEntry == 1 && arrival.entrance == p.entrance);
    assert(arrival.dungeonCheckpoint == p.dungeonCheckpoint);
    assert(!Combo_RequestFwReturn(1, 1) && !Combo_RequestFwReturn(99, 1));
    MM_ResumeGame = nullptr;
    arrivalGame = -1;
    assert(!Combo_RequestFwReturn(0, 1) && arrivalGame == -1);
    assert(Combo_ReadFwPoint(1, &read) == 1);
    Combo_CopyContainer(1, 2);
    g_containerCache.clear();
    assert(Combo_ReadFwPoint(2, &read) == 1 && read.room == p.room);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 2))["progress"] == 31);
    p.game = 0; p.age = 1; p.entrance = 0x123;
    assert(Combo_WriteFwPoint(1, &p));
    assert(Combo_RequestFwReturn(1, 1) && arrivalGame == 0 && arrivalSlot == 1);
    assert(Combo_ReadFwPoint(2, &read) == 1 && read.game == 1); // Files stay independent.
    assert(Combo_WriteFwPoint(1, nullptr));
    g_containerCache.clear();
    assert(Combo_ReadFwPoint(1, &read) == 0 && !Combo_RequestFwReturn(1, 1));
    assert(Combo_ReadFwPoint(2, &read) == 1);
    assert(!Combo_WriteFwPoint(255, &p) && !Combo_WriteFwPoint(-1, &p));
    assert(!Combo_ReadFwPoint(255, &read));
    assert(!std::filesystem::exists(ComboContainerPath(255)));
    auto before = LoadOrCreateContainer(2);
    p.yaw = 100000;
    assert(!Combo_WriteFwPoint(2, &p) && LoadOrCreateContainer(2) == before);
    LoadOrCreateContainer(1)["combo"]["faroresWind"] = "malformed";
    assert(!Combo_ReadFwPoint(1, &read));
    LoadOrCreateContainer(1)["combo"] = "malformed";
    assert(!Combo_ReadFwPoint(1, &read) && !Combo_WriteFwPoint(1, nullptr));
    TestStartupSaveBoundaries();
    std::cout << "container: disk reload, both saves, both handoffs, copy, clear, rejected writes passed\n";
    std::cout << "container: startup staging, durable copy, transient cleanup, generation boundaries passed\n";
}
