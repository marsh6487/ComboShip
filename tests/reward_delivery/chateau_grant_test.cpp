#include "soh/include/z64item.h"
#include "soh/mods/nei_save.h"
#include "soh/mods/items/custom_bottles.h"
#include "soh/soh/FleetShipCombo/FleetSharedItems.h"
#include "soh/soh/FleetShipCombo/FleetComboItems.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(name) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
using u8 = uint8_t;
using u16 = uint16_t;
using s16 = int16_t;
struct PlayState { struct { int state = 0; } pauseCtx; } play;
PlayState* gPlayState = &play;
struct {
    struct { u8 items[24]; } inventory;
    struct { u8 cButtonSlots[3], buttonItems[4]; } equips;
    bool isMagicAcquired = false, isDoubleMagicAcquired = false;
} gSaveContext;
NeiSaveData saved{};
extern "C" NeiSaveData* Nei_Save() { return &saved; }
int persisted = 0, peerGrants = 0, blueRupees = 0, iconReloads = 0;
constexpr int MOD_NONE = 0, MOD_RANDOMIZER = 1, CAN_OBTAIN = 0, CANT_OBTAIN_ALREADY_HAVE = 1;
constexpr int ITEMTYPE_ITEM = 0, LOGIC_BOTTLES = 1, RHT_NONE = 0, OBJECT_GI_MILK = 1;
constexpr int TEXT_RANDOMIZER_CUSTOM_ITEM = 2, CHEST_ANIM_LONG = 1, ITEM_CATEGORY_MAJOR = 1;
struct GetItemEntry { int getItemId = RG_NONE, itemId = ITEM_NONE, modIndex = MOD_NONE; };
void Randomizer_DrawChateauRomani(PlayState*, GetItemEntry*) {}
struct Text {
    std::string english, french, german;
    Text() = default;
    Text(const char* en, const char* fr, const char* de) : english(en), french(fr), german(de) {}
    const std::string& GetEnglish() const { return english; }
    const std::string& GetFrench() const { return french; }
    const std::string& GetGerman() const { return german; }
};
namespace Rando::StaticData {
struct Item {
    RandomizerGet rg = RG_NONE;
    Text name;
    GetItemEntry entry;
    const char* icon = nullptr;
    void (*draw)(PlayState*, GetItemEntry*) = nullptr;
    Item() = default;
    Item(RandomizerGet id, Text n, int, int, bool, int, int, int content, int, int, int, int, int, int, int mod)
        : rg(id), name(n), entry{ id, content, mod } {}
    Item& CustomIcon(const char* path) { icon = path; return *this; }
    void SetCustomDrawFunc(void (*fn)(PlayState*, GetItemEntry*)) { draw = fn; }
    const Text& GetName() const { return name; }
    RandomizerGet GetRandomizerGet() const { return rg; }
    GetItemEntry GetGIEntry_Copy() const { return entry; }
};
std::array<Item, RG_MAX> itemTable{};
std::unordered_map<std::string, RandomizerGet> itemNameToEnum;
Item& RetrieveItem(RandomizerGet rg) { return itemTable[rg]; }
void Initialize() {
    /* CATALOG */
}
}

const int sFcNative[] = {
#define X(id, len, flags, rg, ri, cn, on, mn) int(rg),
    FC_COMBO_ITEM_LIST(X)
#undef X
};
/* FC_MAP */
bool Combo_IsCombinedGame() { return true; }
bool Combo_IsVanilla() { return false; }
bool SOH_ShareSoulReceipt(int) { return false; }
void FleetShared_RequestPullFromPeer() { assert(false); }
void ShareNativeMagicFloor(int) { assert(false); }
int FleetCombo_ChainAliasFor(int) { return 0; }
const char* FcCombo_PeerNameForItem(int fc) {
    assert(fc == FCI_BOTTLE_CHATEAU);
    return "RI_BOTTLE_CHATEAU_ROMANI";
}
using FnGrantSharedItem = void (*)(const char*);
FnGrantSharedItem ResolvePeerGrant() { return [](const char*) { ++peerGrants; }; }
static int sReceiveDepth = 0;
/* RECEIVE_GUARD */

/* CAPACITY */
struct RandomizerBoundary {
    int GetItemObtainabilityFromRandomizerGet(RandomizerGet item) const {
        switch (item) {
            /* OBTAINABILITY */
            default: return CAN_OBTAIN;
        }
    }
} randomizer;
struct OTRGlobals { static OTRGlobals* Instance; RandomizerBoundary* gRandomizer; } globals{ &randomizer };
OTRGlobals* OTRGlobals::Instance = &globals;
bool FleetSync_IsApplyingFc() { return false; }
u16 Return_Item_Entry(GetItemEntry, RandomizerGet) { return 0; }
u16 Randomizer_Item_Give(PlayState*, GetItemEntry giEntry) {
    auto item = static_cast<RandomizerGet>(giEntry.getItemId);
    /* NATIVE_GRANT */
    return 0xFFFF; // an unhandled native grant must fail this probe
}
void Combo_GrantResolvedOOT(const GetItemEntry& entry) {
    if (entry.modIndex == MOD_RANDOMIZER)
        assert(Randomizer_Item_Give(gPlayState, entry) != 0xFFFF);
    else
        ++blueRupees;
    ++persisted;
}
struct ItemTableManager {
    static ItemTableManager* Instance;
    GetItemEntry RetrieveItemEntry(int, int) const { return {}; }
} itemTableManager;
ItemTableManager* ItemTableManager::Instance = &itemTableManager;
namespace FleetRpg { struct State { int nativeMagicLevel = 0; }; State Read() { return {}; } }
constexpr int COMBO_RPG_MAGIC = 0;
bool ComboRpgState_Enabled(const FleetRpg::State*, int) { return false; }
/* NAME_GRANT */
void Interface_LoadItemIcon1(PlayState*, u16) { ++iconReloads; }
/* PROJECTION */
void ApplySharedBottles(const nlohmann::json& sh) {
    auto* nei = Nei_Save();
    const auto& inv = sh.at("inv");
    /* SHARED_BOTTLES */
}
nlohmann::json SharedSnapshot() {
    return { { "inv", { { "net", saved.netEquipped != 0 },
                        { "bottomlessMode", saved.bottomlessBottleMode },
                        { "bottomlessContent", saved.bottomlessContent },
                        { "bottomlessCount", saved.bottomlessCount } } },
             { "bottleSlots", saved.bottleSlots }, { "comboObtainedFc", saved.comboObtainedFc } };
}
void Reset() {
    saved = {};
    std::fill(std::begin(saved.bottleSlots), std::end(saved.bottleSlots), BOTTLE_SLOT_EMPTY);
    gSaveContext = {};
    std::fill(std::begin(gSaveContext.inventory.items), std::end(gSaveContext.inventory.items), ITEM_NONE);
    std::fill(std::begin(gSaveContext.equips.cButtonSlots), std::end(gSaveContext.equips.cButtonSlots), ITEM_NONE);
    Bottle_WheelResetTracking();
    persisted = peerGrants = blueRupees = iconReloads = 0;
    sReceiveDepth = 0;
    play.pauseCtx.state = 0;
}
int CountBottles() {
    return std::count_if(std::begin(saved.bottleSlots), std::end(saved.bottleSlots),
                         [](u8 item) { return item != BOTTLE_SLOT_EMPTY; });
}
int main() {
    Rando::StaticData::Initialize();
    const char* peerName = gFcComboItems[FCI_BOTTLE_CHATEAU].ootName;
    Reset();
    RandomizerGet granted = RG_NONE;
    if (!GrantOotItemByName(peerName, &granted)) {
        std::cerr << "FAIL: the MM shared Chateau name has no working OoT catalog grant\n";
        return 1;
    }
    assert(granted == RG_CHATEAU_ROMANI && CountBottles() == 1);
    assert(saved.bottleSlots[0] == ITEM_CHATEAU_ROMANI);
    assert(saved.bottomlessBottleMode == 1 && saved.netEquipped == 1);
    assert(saved.comboObtainedFc[FCI_BOTTLE_CHATEAU] == 1 && saved.comboAppliedFc[FCI_BOTTLE_CHATEAU] == 1);
    assert(persisted == 1 && peerGrants == 0 && sReceiveDepth == 0);
    const auto& row = Rando::StaticData::RetrieveItem(RG_CHATEAU_ROMANI);
    assert(row.entry.modIndex == MOD_RANDOMIZER && row.entry.itemId == ITEM_CHATEAU_ROMANI);
    assert(row.draw == Randomizer_DrawChateauRomani && row.icon);

    // This is an ordinary gameplay frame before opening either inventory or switching games.
    BottleItems_Enforce(&play);
    assert(gSaveContext.inventory.items[SLOT_BOTTLE_1] == ITEM_CHATEAU_ROMANI);
    assert(gSaveContext.inventory.items[SLOT_BOTTLE_3] == ITEM_NET);
    assert(gSaveContext.inventory.items[SLOT_BOTTLE_4] == ITEM_BOTTLE);
    auto shared = SharedSnapshot();
    ApplySharedBottles(shared);
    ApplySharedBottles(shared);
    BottleItems_Enforce(&play);
    assert(CountBottles() == 1 && persisted == 1 && peerGrants == 0);

    // A later distinct bottle is another owned container, never a refill of the first one.
    assert(GrantOotItemByName(peerName, nullptr));
    assert(CountBottles() == 2 && saved.bottleSlots[0] == ITEM_CHATEAU_ROMANI &&
           saved.bottleSlots[1] == ITEM_CHATEAU_ROMANI);
    assert(saved.comboObtainedFc[FCI_BOTTLE_CHATEAU] == 2 && saved.comboAppliedFc[FCI_BOTTLE_CHATEAU] == 2);
    shared = SharedSnapshot();
    ApplySharedBottles(shared);
    BottleItems_Enforce(&play);
    assert(CountBottles() == 2 && peerGrants == 0);

    // Consumed contents are volatile state. Reconciliation must not resurrect the receipt.
    shared["bottleSlots"][0] = ITEM_BOTTLE;
    shared["bottleSlots"][1] = ITEM_BOTTLE;
    ApplySharedBottles(shared);
    Bottle_WheelResetTracking();
    gSaveContext.inventory.items[SLOT_BOTTLE_1] = ITEM_BOTTLE;
    BottleItems_Enforce(&play);
    ApplySharedBottles(shared);
    assert(CountBottles() == 2 && saved.bottleSlots[0] == ITEM_BOTTLE && saved.bottleSlots[1] == ITEM_BOTTLE);
    assert(FcBottle_MmToOot(0x25) == ITEM_CHATEAU_ROMANI && FcBottle_OotToMm(ITEM_CHATEAU_ROMANI) == 0x25);

    Reset();
    Randomizer_Item_Give(&play, row.GetGIEntry_Copy());
    assert(CountBottles() == 1 && peerGrants == 1); // a native acquisition shares exactly once
    std::fill(std::begin(saved.bottleSlots), std::end(saved.bottleSlots), ITEM_POTION_RED);
    BottleItems_Enforce(&play);
    const auto before = saved;
    assert(GrantOotItemByName(peerName, nullptr));
    assert(blueRupees == 1 && CountBottles() == 8);
    assert(std::memcmp(before.bottleSlots, saved.bottleSlots, sizeof(saved.bottleSlots)) == 0);
    assert(saved.comboObtainedFc[FCI_BOTTLE_CHATEAU] == before.comboObtainedFc[FCI_BOTTLE_CHATEAU]);
    // A dormant/reset projection may still be blank while all eight real containers are owned.
    std::fill(std::begin(gSaveContext.inventory.items), std::end(gSaveContext.inventory.items), ITEM_NONE);
    assert(GrantOotItemByName(peerName, nullptr));
    assert(blueRupees == 2 && CountBottles() == 8);
    assert(std::memcmp(before.bottleSlots, saved.bottleSlots, sizeof(saved.bottleSlots)) == 0);
    assert(saved.comboObtainedFc[FCI_BOTTLE_CHATEAU] == before.comboObtainedFc[FCI_BOTTLE_CHATEAU]);
    Randomizer_Item_Give(&play, row.GetGIEntry_Copy()); // defensive native bypass also cannot overwrite
    assert(std::memcmp(before.bottleSlots, saved.bottleSlots, sizeof(saved.bottleSlots)) == 0);
    assert(!GrantOotItemByName("Unknown bottle", nullptr));
    std::cout << "PASS Chateau peer/native grant, immediate wheel/Bottomless projection, no echo, "
                 "repeated sync, distinct bottles, consumption and full inventory\n";
}
