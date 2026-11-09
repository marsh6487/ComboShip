#include "Rando/Logic/Logic.h"
#include "Rando/Bags.h"
#include "Rando/KeyRings.h"
#include "Rando/Wallets.h"
#include "combo/menu/ItemGrantAuditBridge.h"
#include "FleetShipCombo/FleetSharedItems.h"
#include "FleetShipCombo/FleetShipCombo.h"
#include "tests/test_require.h"
#include <cstdio>
#include <cstring>
using json = nlohmann::json;
#include "bag_json.inc"

// Logging, dormant persistence and unrelated starting-kit grants are controlled
// boundaries. Use real native bag writes and saved starting-list traversal.
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
static int receiveDepth, dormantGrants;

extern "C" {
#include "variables.h"
#include "functions.h"
SaveContext gSaveContext{};
PlayState* gPlayState;
u8 NativeRefill(PlayState*, u8);
s32 Ship_Random(s32 first,s32) { return first; }
void Ship_Random_Seed(uint32_t) {}
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
void ItemGrantAudit_Begin(const char*,int,int,int) {}
void ItemGrantAudit_End() {}
#endif
void FleetShared_BeginReceive() { ++receiveDepth; }
void FleetShared_EndReceive() { REQUIRE(receiveDepth > 0); --receiveDepth; }
}
namespace Rando {
#include "wallet_convert.inc"
// Conversion at this boundary only admits an unowned base bag; other starting
// items are passed through. Full ConvertItem also has a production-TU check.
RandoItemId ConvertItem(RandoItemId item,RandoCheckId) {
    if (item == RI_PROGRESSIVE_WALLET)
        return NativeWalletObtainable(false) ? NativeWalletResolve() : RI_JUNK;
    if (item == RI_WALLET_CHILD && RandoCapacity_HasWallet()) return RI_JUNK;
    const auto* bag = Bags::Find(item);
    return bag && Flags_GetRandoInf(bag->flag) ? RI_JUNK : item;
}
void GiveItem(RandoItemId item,RandoCheckId) {
    if (Bags::Give(item) || Bags::GiveCapacity(item) || Wallets::Give(item)) return;
    // Native concrete-wallet inventory mutation is an engine boundary here.
    if (item == RI_WALLET_ADULT) Inventory_ChangeUpgrade(UPG_WALLET,1);
    if (item == RI_WALLET_GIANT) Inventory_ChangeUpgrade(UPG_WALLET,2);
    if (item == RI_WALLET_TYCOON) Inventory_ChangeUpgrade(UPG_WALLET,3);
    if (item == RI_DEKU_STICK) NativeRefill(nullptr, ITEM_DEKU_STICK);
    if (item == RI_DEKU_NUT) NativeRefill(nullptr, ITEM_DEKU_NUT);
}
#include "bag_starting.inc"
}
#ifdef COMBO_BUILD
#include "wallet_flush.inc"
void Combo_MM_GiveDormantResolved(RandoItemId item) {
    REQUIRE(receiveDepth == 1);
    ++dormantGrants;
    Rando::GiveItem(item);
}
#include "bag_bridge.inc"
#endif

int main() {
    const int items[] = {ITEM_DEKU_STICK, ITEM_DEKU_NUT};
    const int upgrades[] = {UPG_DEKU_STICKS, UPG_DEKU_NUTS};
    const RandoOptionId options[] = {RO_SHUFFLE_DEKU_STICK_BAG, RO_SHUFFLE_DEKU_NUT_BAG};
    const RandoInf flags[] = {RANDO_INF_DEKU_STICK_BAG, RANDO_INF_DEKU_NUT_BAG};
    const int refills[][3] = {{ITEM_DEKU_STICK,ITEM_DEKU_STICKS_5,ITEM_DEKU_STICKS_10},
                             {ITEM_DEKU_NUT,ITEM_DEKU_NUTS_5,ITEM_DEKU_NUTS_10}};
    for (int which = 0; which < 2; ++which) {
        gSaveContext = {};
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[options[which]] = 1;
        // A capacity arriving through FleetSync does not grant the base bag.
        Inventory_ChangeUpgrade(upgrades[which], 3);
        INV_CONTENT(items[which]) = items[which];
        REQUIRE(!HAS_ITEM(items[which]));
        REQUIRE(CUR_CAPACITY(upgrades[which]) == 0);
        for (int refill : refills[which]) {
            AMMO(items[which]) = 0;
            NativeRefill(nullptr, refill);
            REQUIRE(AMMO(items[which]) == 0);
        }
        for (int delta : {-32768,-100,-1,0,1,100,32767}) {
            Inventory_ChangeAmmo(items[which], delta);
            REQUIRE(AMMO(items[which]) == 0);
        }
        Flags_SetRandoInf(flags[which]);
        REQUIRE(HAS_ITEM(items[which]));
        REQUIRE(CUR_CAPACITY(upgrades[which]) == (which == 0 ? 30 : 40));
        AMMO(items[which]) = 0;
        Inventory_ChangeAmmo(items[which], 32767);
        REQUIRE(AMMO(items[which]) == CUR_CAPACITY(upgrades[which]));
        Inventory_ChangeAmmo(items[which], -32768);
        REQUIRE(AMMO(items[which]) == 0);
        // Real native word-indexed flags persist independently of capacities.
        auto saved = gSaveContext.save;
        gSaveContext = {};
        gSaveContext.save = saved;
        REQUIRE(HAS_ITEM(items[which]));
        Flags_ClearRandoInf(flags[which]);
        REQUIRE(!HAS_ITEM(items[which]));
        // Old seeds and native saves preserve ordinary pickup behavior.
        gSaveContext = {};
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                    sizeof(gSaveContext.save.saveInfo.inventory.items));
        NativeRefill(nullptr, items[which]);
        REQUIRE(HAS_ITEM(items[which]) && AMMO(items[which]) == 1);
        REQUIRE(CUR_CAPACITY(upgrades[which]) == (which == 0 ? 10 : 20));
    }
    // Execute the exact grant helper used by real receipts and the headless
    // oracle, including a capacity arriving before the bag and repeat grants.
    for (const auto& bag : Rando::Bags::Catalog) {
        gSaveContext = {};
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[bag.option] = 1;
        std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                    sizeof(gSaveContext.save.saveInfo.inventory.items));
        const auto capacity = bag.item == RI_DEKU_STICK_BAG ? RI_OOT_PROGRESSIVE_STICK_CAPACITY : RI_OOT_PROGRESSIVE_NUT_CAPACITY;
        REQUIRE(Rando::Bags::GiveCapacity(capacity));
        REQUIRE(CUR_UPG_VALUE(bag.upgrade) == 2);
        REQUIRE(!HAS_ITEM(bag.inventoryItem) && CUR_CAPACITY(bag.upgrade) == 0);
        REQUIRE(AMMO(bag.inventoryItem) == 0);
        REQUIRE(Rando::Bags::Give(bag.item));
        REQUIRE(Flags_GetRandoInf(bag.flag) && HAS_ITEM(bag.inventoryItem));
        REQUIRE(CUR_UPG_VALUE(bag.upgrade) == 2);
        REQUIRE(AMMO(bag.inventoryItem) == (bag.upgrade == UPG_DEKU_STICKS ? 20 : 30));
        AMMO(bag.inventoryItem) = 1;
        REQUIRE(Rando::Bags::Give(bag.item));
        REQUIRE(AMMO(bag.inventoryItem) == 1);
        REQUIRE(Rando::Bags::GiveCapacity(capacity));
        REQUIRE(CUR_UPG_VALUE(bag.upgrade) == 3);
        REQUIRE(Rando::Bags::Remove(bag.item));
        REQUIRE(!HAS_ITEM(bag.inventoryItem) && AMMO(bag.inventoryItem) == 0);
        REQUIRE(CUR_UPG_VALUE(bag.upgrade) == 3);
        REQUIRE(Rando::Bags::Give(bag.item));
        REQUIRE(CUR_UPG_VALUE(bag.upgrade) == 3);
        REQUIRE(AMMO(bag.inventoryItem) == CUR_CAPACITY(bag.upgrade));
    }
    for (int mask = 0; mask < 4; ++mask) {
        gSaveContext = {};
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        auto& save = gSaveContext.save.shipSaveInfo.rando;
        for (const auto& [id,row] : Rando::StaticData::Options) save.randoSaveOptions[id] = row.defaultValue;
        save.randoSaveOptions[RO_SHUFFLE_DEKU_STICK_BAG] = 1;
        save.randoSaveOptions[RO_SHUFFLE_DEKU_NUT_BAG] = 1;
        save.randoSaveOptions[RO_STARTING_CONSUMABLES] = 1;
        std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                    sizeof(gSaveContext.save.saveInfo.inventory.items));
        std::vector<RandoItemId> starts;
        for (int i = 0; i < 2; ++i)
            if (mask & (1 << i)) starts.push_back(Rando::Bags::Catalog[i].item);
        nlohmann::json spoiler;
        Rando::SetStartingItemsInSpoiler(spoiler,starts);
        auto imported = Rando::GetStartingItemsFromSpoiler(spoiler);
        REQUIRE(imported == starts);
        Rando::SetStartingItemsInSave(save,imported);
        REQUIRE(Rando::GetStartingItemsFromSave(save) == starts);
        Rando::GrantStartingItems();
        for (int i = 0; i < 2; ++i) {
            REQUIRE(bool(HAS_ITEM(items[i])) == bool(mask & (1 << i)));
            REQUIRE(AMMO(items[i]) == ((mask & (1 << i)) ? (i == 0 ? 10 : 20) : 0));
        }
        // Execute the actual RandoSaveInfo JSON adapters used by SaveManager,
        // including a shorter old option list loaded into a dirty context.
        save.randoInf[0] = 0x1234;
        json persisted = save;
        auto reloaded = persisted.get<RandoSaveInfo>();
        REQUIRE(Rando::GetStartingItemsFromSave(reloaded) == starts);
        REQUIRE(reloaded.randoInf[0] == 0x1234);
        for (const auto& bag : Rando::Bags::Catalog) {
            REQUIRE(reloaded.randoSaveOptions[bag.option] == 1);
            REQUIRE(bool(reloaded.randoInf[bag.flag / 16] & (1u << (bag.flag % 16))) ==
                    bool(Flags_GetRandoInf(bag.flag)));
        }
        auto legacy = persisted;
        auto oldOptions = legacy["randoSaveOptions"].get<std::vector<u32>>();
        oldOptions.resize(RO_SHUFFLE_DEKU_STICK_BAG);
        legacy["randoSaveOptions"] = oldOptions;
        auto oldFlags = legacy["randoInf"].get<std::vector<u16>>();
        for (const auto& bag : Rando::Bags::Catalog)
            oldFlags[bag.flag / 16] &= ~(1u << (bag.flag % 16));
        legacy["randoInf"] = oldFlags;
        std::memset(&reloaded,0xFF,sizeof(reloaded));
        legacy.get_to(reloaded);
        REQUIRE(reloaded.randoInf[0] == 0x1234);
        REQUIRE(reloaded.randoSaveOptions[RO_SHUFFLE_CHILD_WALLET] == 0);
        REQUIRE(!(reloaded.randoInf[RANDO_INF_WALLET_CHILD / 16] & (1u << (RANDO_INF_WALLET_CHILD % 16))));
        for (const auto& bag : Rando::Bags::Catalog) {
            REQUIRE(reloaded.randoSaveOptions[bag.option] == 0);
            REQUIRE(!(reloaded.randoInf[bag.flag / 16] & (1u << (bag.flag % 16))));
        }
#ifdef COMBO_BUILD
        // Friendly and enum names use the real native lookup and receive guard.
        for (const auto& bag : Rando::Bags::Catalog) {
            Rando::Bags::Remove(bag.item);
            RandoItemId received = RI_UNKNOWN;
            REQUIRE(GrantMmItemByName(Rando::StaticData::Items.at(bag.item).name,&received));
            REQUIRE(received == bag.item && HAS_ITEM(bag.inventoryItem));
            AMMO(bag.inventoryItem) = 1;
            REQUIRE(GrantMmItemByName(Rando::StaticData::Items.at(bag.item).spoilerName,&received));
            REQUIRE(received == RI_RUPEE_RED && AMMO(bag.inventoryItem) == 1);
            REQUIRE(receiveDepth == 0);
        }
#endif
    }
    for (bool enabled : {false,true}) {
        for (bool tycoon : {false,true}) {
            for (int count = 0; count <= 4; ++count) {
                gSaveContext = {};
                gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
                auto& save = gSaveContext.save.shipSaveInfo.rando;
                for (const auto& [id,row] : Rando::StaticData::Options) save.randoSaveOptions[id] = row.defaultValue;
                save.randoSaveOptions[RO_SHUFFLE_CHILD_WALLET] = enabled;
                save.randoSaveOptions[RO_SHUFFLE_TYCOON_WALLET] = tycoon;
                std::vector<RandoItemId> starts(count, RI_PROGRESSIVE_WALLET);
                Rando::SetStartingItemsInSave(save,starts);
                Rando::GrantStartingItems();
                const int finite = std::min(count, 2 + int(enabled) + int(tycoon));
                REQUIRE(CUR_UPG_VALUE(UPG_WALLET) == unsigned(std::max(0, finite - int(enabled))));
                REQUIRE(bool(Flags_GetRandoInf(RANDO_INF_WALLET_CHILD)) == (enabled && count > 0));
                REQUIRE(Rando::NativeWalletObtainable(false) == (count < 2 + int(enabled) + int(tycoon)));
                REQUIRE(!Rando::NativeWalletObtainable(true));
                json persisted = save;
                auto restored = persisted.get<RandoSaveInfo>();
                REQUIRE(restored.randoSaveOptions[RO_SHUFFLE_CHILD_WALLET] == enabled);
                const int f = RANDO_INF_WALLET_CHILD;
                REQUIRE(restored.randoInf[f / 16] == save.randoInf[f / 16]);
                REQUIRE(Rando::GetStartingItemsFromSave(restored) == starts);
            }
        }
    }
    std::puts("PASS actual wallet progressive conversion, capped tiers, starting grants and saved ownership");
    std::puts("PASS native bag gates, imports, refills, signed ammo, grant/remove, starting/spoiler/JSON saves and old defaults");
#ifdef COMBO_BUILD
    gSaveContext = {};
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET] = 1;
    RandoItemId received = RI_UNKNOWN;
    REQUIRE(GrantMmItemByName("Child's Wallet",&received));
    REQUIRE(received == RI_WALLET_CHILD && Flags_GetRandoInf(RANDO_INF_WALLET_CHILD));
    Combo_MM_FlushAccumulators();
    gSaveContext.save.saveInfo.playerData.rupees = 1;
    REQUIRE(GrantMmItemByName("RI_WALLET_CHILD",&received));
    REQUIRE(received == RI_RUPEE_RED && gSaveContext.rupeeAccumulator == 0);
    REQUIRE(dormantGrants == 18 && receiveDepth == 0);
    std::puts("PASS real friendly/enum foreign-name lookup, guarded dormant grant and duplicate conversion");
#endif
}
