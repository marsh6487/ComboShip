// Integrate the real launcher BEGIN, foreign loader, OoT capture and Letter
// save seam, with native save/item types and boundary doubles for catalogs,
// grants, native serialization and hook registration.
#include "combo/vanilla/ComboVanillaFile.h"
#include "combo/vanilla/ComboVanillaRewards.h"
#include "combo/rando/StartupCheckReceiptState.h"
#include "combo/rando/CrossForeign.h"
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "soh/cvar_prefixes.h"
#include "soh/OTRGlobals.h"
#include "ComboGameMode.h"

using nlohmann::json;
#define COMBO_RELEASE_VERSION "9.3.0"
SaveContext gSaveContext{};
PlayState* gPlayState;
int gComboGameMode = COMBO_GAME_BOTH;
static std::filesystem::path directory;
static std::mutex g_containerMutex;
static std::map<int, json> g_containerCache;
static std::map<int, ComboVanilla::Creation> g_vanillaCreations;
static std::map<int, ComboVanilla::Receipt> g_vanillaReceipts;
static std::map<int, std::shared_ptr<ComboStartupReceipt::MmCompletion>> g_startupCompletions;
static std::map<int, uint32_t> g_startupAckRetries;
static uint32_t g_startupGeneration;
static std::vector<int> g_evictedSlots;
static int g_MmSaveInMemorySlot = -1;
static bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
static auto ComboContainerPath(int slot) { return directory / (std::to_string(slot) + ".combosav"); }
static void Invalidate() {}
static auto MM_InvalidateOwlBlobSlot = Invalidate;
static int32_t FinishMm(CwStartupReceipt*);
static CwFinishMmStartupReceipt MM_FinishStartupReceipt = FinishMm;
static int32_t CaptureOot(int32_t);
static auto SOH_CaptureStartupReceiptSave = CaptureOot;
static std::string g_ConsolidatedJson;

struct GameInteractor {
    struct OnRandoHintRevealed {};
    static GameInteractor* Instance;
    template<class Hook, class Callback> void RegisterGameHook(Callback) {}
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static void OOT_ComboHintRevealed() {}
namespace Rando::StaticData {
struct Name { std::string english = "Concrete Bow"; };
struct Item {
    std::shared_ptr<GetItemEntry> entry;
    Name name;
    auto GetGIEntryUnresolved() const { return entry; }
    const Name& GetName() const { return name; }
};
static Item item;
const Item& RetrieveItem(RandomizerGet id) {
    static Item empty;
    return id == RG_FAIRY_BOW ? item : empty;
}
struct Location { std::string GetName() const { return "Song from Impa"; } };
static Location location;
const Location* GetLocation(RandomizerCheck) { return &location; }
}
static std::unordered_map<std::string, ComboRando::ForeignItem> g_ootForeignMap;
static uint64_t g_ootForeignGen;
/* FOREIGN_FUNCTIONS */
static auto SOH_LoadComboRando = LoadOotForeign;
namespace ComboDungeonPickup {
bool IsMapOrCompass(const std::string& s) { return s.find("Map") != s.npos || s.find("Compass") != s.npos; }
}
/* HOST_FUNCTIONS */

static CwStartupReceiptHost sHost = Combo_StartupReceipt;
static void Cancel() {}
/* CAPTURE_FUNCTIONS */
static void ComboStartupOot_BeginFile() { BeginFile(); }
static bool ComboStartupOot_Capture(const GetItemEntry* e, int32_t check) { return Capture(e, check); }

static bool letter, shuffle;
static int nativeGrants, deferredGrants, marked, storyFlags;
static GetItemEntry placement;
static json mmLive, ootLive;
static bool Randomizer_GetSettingValue(RandomizerSettingKey key) {
    return key == RSK_STARTING_ZELDAS_LETTER ? letter : shuffle;
}
static GetItemEntry Randomizer_GetItemFromKnownCheck(RandomizerCheck check, GetItemID) {
    return check == RC_SONG_FROM_IMPA ? placement : *Rando::StaticData::item.entry;
}
struct CheckLocation { void SetCheckStatus(int status) { assert(status == RCSHOW_SAVED); ++marked; } };
struct Context { CheckLocation location; auto GetItemLocation(RandomizerCheck) { return &location; } };
struct Globals { Context* gRandoContext; };
static Context context;
static Globals globals{&context};
// The seam uses this pointer only to mark the native check.
#define OTRGlobals TestGlobals
struct TestGlobals { static Globals* Instance; };
Globals* TestGlobals::Instance = &globals;
static void StartingItemGive(GetItemEntry, RandomizerCheck) { ++nativeGrants; }
static void Flags_SetEventChkInfForTest(u16) { ++storyFlags; }
static void Flags_SetRandomizerInfForTest(RandomizerInf) { ++storyFlags; }
#define Flags_SetEventChkInf Flags_SetEventChkInfForTest
#define Flags_SetRandomizerInf Flags_SetRandomizerInfForTest
/* SAVE_SEAM */
#undef OTRGlobals

static int32_t CaptureOot(int32_t slot) {
    assert(Combo_StartupReceipt(CW_STARTUP_PREPARE_PEER_SNAPSHOT, 0, slot, nullptr) == 1);
    Combo_WriteGameSave(0, slot, ootLive.dump().c_str());
    return 1;
}
static int32_t FinishMm(CwStartupReceipt* r) {
    if (r->state == CW_STARTUP_WAIT_MM) {
        ++deferredGrants;
        mmLive["awards"] = deferredGrants;
        r->state = CW_STARTUP_READY;
        std::strcpy(r->itemName, "Concrete MM Bow");
        std::strcpy(r->displayName, "Concrete MM Bow (MM)");
    } else {
        Combo_WriteGameSave(1, g_MmSaveInMemorySlot, mmLive.dump().c_str());
    }
    return 1;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    directory = argv[1];
    gSaveContext.fileNum = 0;
    gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
    const GetItemEntry local = GET_ITEM(ITEM_BOW, OBJECT_GI_BOW, 0, 0x70, 0x80, CHEST_ANIM_LONG,
                                        ITEM_CATEGORY_MAJOR, MOD_NONE, GI_BOW);
    Rando::StaticData::item.entry = std::make_shared<GetItemEntry>(local);
    for (const char* start : {"MM", "OOT"}) for (bool foreign : {true, false})
    for (bool stale : {false, true}) for (bool startLetter : {true, false}) {
        g_containerCache.clear(); g_startupCompletions.clear(); g_startupAckRetries.clear();
        std::filesystem::remove(ComboContainerPath(0));
        std::filesystem::remove(ComboContainerPath(1));
        g_MmSaveInMemorySlot = -1;
        nativeGrants = deferredGrants = marked = storyFlags = 0;
        g_ootForeignMap.clear();
        ComboRando::Combo_SetForeignJson(nullptr);
        if (stale) LoadOotForeign(R"({"foreign":[{"checkGame":"oot","checkName":"Song from Impa","itemGame":"mm","itemName":"Wrong previous seed item"}]})");
        json seed = {{"startingGame", start}, {"foreign", json::array()}};
        if (foreign) seed["foreign"].push_back({{"checkGame","oot"},{"checkName","Song from Impa"},
            {"itemGame","mm"},{"itemName","Progressive Bow"},{"displayName","Bow (MM)"},{"category","major"}});
        g_ConsolidatedJson = seed.dump();
        placement = local;
        if (foreign) { placement.modIndex = MOD_RANDOMIZER; placement.getItemId = RG_COMBO_FOREIGN; }
        letter = startLetter; shuffle = false;
        InitImpa(); // Actual new-save order: BEGIN first, then Letter grant/capture.
        CwStartupReceipt receipt{};
        const bool pending = ComboStartupReceipt::PendingMm(g_containerCache[0], receipt);
        const bool ready = ComboStartupReceipt::Peek(g_containerCache[0], receipt);
        if ((pending || ready) != startLetter)
            std::fprintf(stderr, "Missing startup receipt: start=%s foreign=%d stale=%d letter=%d\n",
                         start, foreign, stale, startLetter);
        assert((pending || ready) == startLetter);
        assert(pending == (startLetter && foreign));
        assert(nativeGrants == (startLetter ? (foreign ? 1 : 2) : 0));
        assert(storyFlags == (startLetter ? 3 : 0));
        if (!startLetter) continue;
        assert(!std::strcmp(receipt.itemName, foreign ? "Progressive Bow" : "Concrete Bow"));
        ootLive = {{"letter",true}}; mmLive = {{"initialized",true}};
        Combo_WriteGameSave(0, 0, ootLive.dump().c_str());
        Combo_WriteGameSave(1, 0, mmLive.dump().c_str());
        // Copy before MM initialization/presentation. Preserve the pending
        // award in the copy; completing/acknowledging the source must not
        // mutate the copy or copy an uncommitted native reward.
        Combo_CopyContainer(0, 1);
        CwStartupReceipt copied{};
        if (foreign) assert(ComboStartupReceipt::PendingMm(g_containerCache[1], copied));
        else assert(ComboStartupReceipt::Peek(g_containerCache[1], copied));
        assert(!std::strcmp(copied.itemName, receipt.itemName));
        assert(!g_containerCache[1]["mm"].contains("awards"));
        g_MmSaveInMemorySlot = 0;
        Combo_CompleteStartupReceipt(0, true);
        const int first = !std::strcmp(start, "MM") ? 1 : 0;
        assert(Combo_StartupReceipt(CW_STARTUP_PEEK, first, 0, &receipt) == 1);
        assert(deferredGrants == (foreign ? 1 : 0));
        assert(std::string(ComboStartupReceipt::Record(g_containerCache[0])->at("source")) == CW_STARTUP_SOURCE);
        assert(Combo_StartupReceipt(CW_STARTUP_ACKNOWLEDGE, first, 0, &receipt) == 1);
        g_containerCache.clear(); g_startupCompletions.clear(); g_startupAckRetries.clear();
        Combo_CompleteStartupReceipt(0, true);
        assert(Combo_StartupReceipt(CW_STARTUP_PEEK, 1-first, 0, &receipt) == 0);
        assert(deferredGrants == (foreign ? 1 : 0)); // switch/reload never regrants
        auto& copy = LoadOrCreateContainer(1);
        if (foreign) assert(ComboStartupReceipt::PendingMm(copy, copied));
        else assert(ComboStartupReceipt::Peek(copy, copied));
        assert(!copy["mm"].contains("awards"));
    }
    // Invalid/original creation requests must not publish pending rando data.
    const auto beforeRejected = g_ootForeignGen;
    assert(Combo_StartupReceipt(CW_STARTUP_BEGIN_FILE, 1, 0, nullptr) == -1);
    assert(Combo_StartupReceipt(CW_STARTUP_BEGIN_FILE, 0, 255, nullptr) == -1);
    assert(g_vanillaCreations[0].Begin(0, json::object()));
    assert(Combo_StartupReceipt(CW_STARTUP_BEGIN_FILE, 0, 0, nullptr) == 0);
    assert(g_ootForeignGen == beforeRejected);
    g_vanillaCreations[0].Abort();
    // No pending seed may inherit another file's foreign owner metadata.
    g_ConsolidatedJson.clear();
    BeginFile();
    assert(g_ootForeignMap.empty() && ComboRando::g_comboForeignJson.empty());
    std::puts("PASS creation order: MM/OoT first, local/foreign Impa, cold/stale foreign maps, Letter negative, pending copy, exactly-once switch/reload");
}
