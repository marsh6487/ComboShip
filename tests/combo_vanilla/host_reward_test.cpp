#include "combo/vanilla/ComboVanillaFile.h"
#include "combo/vanilla/ComboVanillaRewards.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
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
static std::vector<int> g_evictedSlots;
static int g_comboCompletionSlot = -1, completionLoads, completionSaves;
static bool g_comboCompletion[2];
static void LoadComboCompletion(int slot) { g_comboCompletionSlot = slot; ++completionLoads; }
static void SaveComboCompletion(int) { ++completionSaves; }
static int g_MmSaveInMemorySlot = -1;
static int invalidations;
static void Invalidate() { ++invalidations; }
static auto MM_InvalidateOwlBlobSlot = Invalidate;
static bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
static auto ComboContainerPath(int slot) { return directory / (std::to_string(slot) + ".combosav"); }
/* HOST_FUNCTIONS */
static json ReadDisk(int slot) { json value; std::ifstream(ComboContainerPath(slot)) >> value; return value; }
static int Host(ComboVanilla::RewardAction action, int game, int slot, const char* id, const char* native = nullptr) {
    return Combo_VanillaReward(static_cast<int>(action), game, slot, id, native);
}
int main(int argc, char** argv) {
    assert(argc == 2); directory = argv[1];
    json original = {{"combo", {{"mode", "vanilla-nei"}, {"vanilla", {{"version", 1}, {"collected", json::object()}}}}},
        {"oot", {{"fileType", 0}, {"version", 1}, {"sections", {{"base", {{"data", {{"rupees", 99}}}}}}}}},
        {"mm", {{"type", "2S2H_SAVE"}, {"version", 1}, {"newCycleSave", {{"save", {{"shipSaveInfo", {{"saveType", 0}}}}}}}}}};
    g_containerCache[0] = original;
    assert(FlushContainer(0));
    using A = ComboVanilla::RewardAction;
    const char* id = "vanilla.nei.lantern";
    assert(Host(A::Query, 0, -1, id) == -1 && Host(A::Query, 1, 0, id) == -1);
    assert(Host(A::Query, 0, 0, "native.bow") == -1);
    // A real filesystem failure before Begin must leave no active receipt.
    const auto temporary = ComboContainerPath(0).string() + ".temp";
    std::filesystem::create_directory(temporary);
    assert(Host(A::Begin, 0, 0, id) == 0 && Host(A::Query, 0, 0, id) == 0);
    assert(ReadDisk(0)["oot"] == original["oot"]);
    std::filesystem::remove(temporary);
    assert(Host(A::Begin, 0, 0, id) == 1 && Host(A::Query, 0, 0, id) == 2);
    assert(Host(A::Begin, 0, 0, id) == 0);
    g_containerCache.clear(); g_vanillaReceipts.clear(); // True process-state restart.
    assert(Host(A::Query, 0, 0, id) == 0);
    assert(Host(A::Begin, 0, 0, id) == 1);
    auto granted = original["oot"];
    granted["sections"]["base"]["data"]["rupees"] = 59;
    granted["sections"]["nei"]["data"]["ownedItems"] = json::array({0xB5});
    Combo_WriteGameSave(0, 0, granted.dump().c_str());
    assert(ReadDisk(0)["oot"] == original["oot"]);
    assert(json::parse(Combo_ReadGameSave(0, 0)) == original["oot"]);
    Combo_CopyContainer(0, 1); // Copying staging copies the durable old native + pending.
    g_containerCache.erase(1);
    assert(Host(A::Query, 0, 1, id) == 0);
    assert(ReadDisk(1)["oot"] == original["oot"]);
    // The peer's independent save is retained by the final read/modify/write.
    auto mm = original["mm"]; mm["native-progress"] = 7;
    Combo_WriteGameSave(1, 0, mm.dump().c_str());
    assert(Host(A::Commit, 0, 0, id) == 1);
    assert(ReadDisk(0)["oot"] == granted && ReadDisk(0)["mm"] == mm);
    assert(Host(A::Query, 0, 0, id) == 1 && Host(A::Begin, 0, 0, id) == 0);
    g_containerCache.clear(); g_vanillaReceipts.clear();
    assert(Host(A::Query, 0, 0, id) == 1);
    // The actual copy callback carries permanent and pending metadata intact.
    Combo_CopyContainer(0, 2);
    assert(Host(A::Query, 0, 2, id) == 1 && invalidations == 2);
    // Failed commit keeps the durable pre-grant state and pending exact price.
    id = "vanilla.nei.slate.sensor";
    assert(Host(A::Begin, 0, 0, id) == 1);
    std::filesystem::create_directory(temporary);
    assert(Host(A::Commit, 0, 0, id, granted.dump().c_str()) == 0);
    assert(Host(A::Abort, 0, 0, id) == 1);
    assert(Host(A::Query, 0, 0, id) == 0);
    std::filesystem::remove(temporary);
    assert(Host(A::BeginOwned, 0, 0, id) == 1);
    assert(Host(A::Commit, 0, 0, id, granted.dump().c_str()) == 1);
    assert(ReadDisk(0)["combo"]["vanilla"]["collected"][id]["price"] == 0);
    // MM's final capture must be fresh, with an owl resume, not an incidental
    // earlier stage. A failed capture cannot reuse that stale snapshot.
    id = "vanilla.nei.cane.flip";
    assert(Host(A::Begin, 1, 0, id) == 1);
    Combo_WriteGameSave(1, 0, mm.dump().c_str());
    assert(Host(A::PrepareCapture, 1, 0, id) == 1);
    assert(Host(A::Commit, 1, 0, id) == 0);
    mm["owlSave"]["save"] = mm["newCycleSave"]["save"];
    mm["owlSave"]["save"]["nei"]["caneSkills"] = 8;
    Combo_WriteGameSave(1, 0, mm.dump().c_str());
    assert(Host(A::Commit, 1, 0, id) == 1);
    assert(ReadDisk(0)["mm"] == mm && Host(A::Query, 1, 0, id) == 1);
    assert(Combo_OnFinalBossDefeated(0, 0) == 1);
    assert(Combo_OnFinalBossDefeated(1, 0) == 1);
    assert(completionLoads == 0 && completionSaves == 0); // Native endings never wait for a peer.
    g_containerCache[1] = {{"combo", {{"rando", {{"masterSeed", 1}}}}}};
    assert(Combo_OnFinalBossDefeated(0, 1) == 0);
    assert(Combo_OnFinalBossDefeated(1, 1) == 1 && completionSaves == 2);
    std::cout << "PASS actual host receipt callbacks, filesystem failure, staged save isolation, peer writes, copy, zero-charge aliases and fresh MM capture\n";
}
