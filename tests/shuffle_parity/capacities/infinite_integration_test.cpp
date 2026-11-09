#include "Rando/Logic/Logic.h"
#include "Rando/Bags.h"
#include "Rando/KeyRings.h"
#include "Rando/Wallets.h"
#include "Rando/InfiniteUpgrades.h"
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
// items are passed through. Full ConvertItem remains a production-TU check.
#include "infinite_obtainable.inc"
RandoItemId ConvertItem(RandoItemId item,RandoCheckId) {
    if (InfiniteUpgrades::Find(item)) return NativeInfiniteObtainable(item,false) ? item : RI_JUNK;
    if (item == RI_PROGRESSIVE_WALLET)
        return NativeWalletObtainable(false) ? NativeWalletResolve() : RI_JUNK;
    if (item == RI_WALLET_CHILD && RandoCapacity_HasWallet()) return RI_JUNK;
    const auto* bag = Bags::Find(item);
    return bag && Flags_GetRandoInf(bag->flag) ? RI_JUNK : item;
}
void GiveItem(RandoItemId item,RandoCheckId) {
    if (InfiniteUpgrades::Give(item) || Bags::Give(item) || Bags::GiveCapacity(item) || Wallets::Give(item)) return;
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
    using namespace Rando;
    REQUIRE(!std::strcmp(StaticData::Options.at(RO_INFINITE_UPGRADES).cvar,
                         "gRando.Options.RO_INFINITE_UPGRADES"));
    REQUIRE(StaticData::Options.at(RO_INFINITE_UPGRADES).defaultValue == 0);
    std::vector<RandoItemId> starts;
    for (const auto& upgrade : InfiniteUpgrades::Catalog) starts.push_back(upgrade.item);
    for (int mask = 0; mask < 64; ++mask) {
        gSaveContext = {};
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        auto& save = gSaveContext.save.shipSaveInfo.rando;
        for (const auto& [id,row] : StaticData::Options) save.randoSaveOptions[id] = row.defaultValue;
        save.randoSaveOptions[RO_INFINITE_UPGRADES] = 1;
        std::vector<RandoItemId> selected;
        for (int i = 0; i < 6; ++i) if (mask & (1 << i)) selected.push_back(starts[i]);
        SetStartingItemsInSave(save,selected);
        json spoiler;
        SetStartingItemsInSpoiler(spoiler,selected);
        REQUIRE(GetStartingItemsFromSpoiler(spoiler) == selected);
        GrantStartingItems();
        for (int i = 0; i < 6; ++i) {
            const auto& upgrade = InfiniteUpgrades::Catalog[i];
            REQUIRE(bool(Flags_GetRandoInf(upgrade.flag)) == bool(mask & (1 << i)));
            REQUIRE(!NativeInfiniteObtainable(upgrade.item,true));
            REQUIRE(NativeInfiniteObtainable(upgrade.item,false) == !(mask & (1 << i)));
            REQUIRE(StaticData::GetIconTexturePath(upgrade.item) != nullptr);
            const auto& category = StaticData::StartingItemsMap.at(STARTING_ITEMS_INVENTORY);
            REQUIRE(std::find(category.begin(),category.end(),upgrade.item) != category.end());
        }
        json persisted = save;
        RandoSaveInfo restored = persisted.get<RandoSaveInfo>();
        REQUIRE(restored.randoSaveOptions[RO_INFINITE_UPGRADES] == 1);
        REQUIRE(GetStartingItemsFromSave(restored) == selected);
        for (const auto& upgrade : InfiniteUpgrades::Catalog)
            REQUIRE(restored.randoInf[upgrade.flag/16] == save.randoInf[upgrade.flag/16]);
        auto legacy = persisted;
        auto options = legacy["randoSaveOptions"].get<std::vector<u32>>();
        options.resize(RO_INFINITE_UPGRADES);
        legacy["randoSaveOptions"] = options;
        std::memset(&restored,0xFF,sizeof(restored));
        legacy.get_to(restored);
        REQUIRE(restored.randoSaveOptions[RO_INFINITE_UPGRADES] == 0);
    }
    // Re-running the native kit with endpoints already owned must repair newly
    // usable bases even when duplicate conversion skips their grant helpers.
    gSaveContext = {};
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    SetStartingItemsInSave(gSaveContext.save.shipSaveInfo.rando,starts);
    GrantStartingItems();
    const int ammoItems[] = {ITEM_BOW,ITEM_BOMB,ITEM_BOMBCHU,ITEM_DEKU_STICK,ITEM_DEKU_NUT};
    const int upgrades[] = {UPG_QUIVER,UPG_BOMB_BAG,UPG_BOMB_BAG,UPG_DEKU_STICKS,UPG_DEKU_NUTS};
    for (int i = 0; i < 5; ++i) {
        Inventory_ChangeUpgrade(upgrades[i],1);
        INV_CONTENT(ammoItems[i]) = ammoItems[i];
        AMMO(ammoItems[i]) = 0;
    }
    gSaveContext.magicCapacity = 12;
    gSaveContext.save.saveInfo.playerData.isMagicAcquired = 1;
    gSaveContext.save.saveInfo.playerData.magic = 0;
    gSaveContext.save.saveInfo.playerData.rupees = 0;
    GrantStartingItems();
    for (int i = 0; i < 5; ++i) {
        REQUIRE(AMMO(ammoItems[i]) == CUR_CAPACITY(upgrades[i]));
        REQUIRE(CUR_UPG_VALUE(upgrades[i]) == 1);
    }
    REQUIRE(gSaveContext.save.saveInfo.playerData.magic == 12);
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 500);
    REQUIRE(!CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI));
#ifdef COMBO_BUILD
    gSaveContext = {};
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    for (const auto& upgrade : InfiniteUpgrades::Catalog) {
        RandoItemId received = RI_UNKNOWN;
        REQUIRE(GrantMmItemByName(StaticData::Items.at(upgrade.item).name,&received));
        REQUIRE(received == upgrade.item && Flags_GetRandoInf(upgrade.flag));
        REQUIRE(GrantMmItemByName(StaticData::Items.at(upgrade.item).spoilerName,&received));
        REQUIRE(received == RI_RUPEE_RED && receiveDepth == 0);
        REQUIRE(InfiniteUpgrades::Remove(upgrade.item));
        REQUIRE(!Flags_GetRandoInf(upgrade.flag));
    }
    REQUIRE(dormantGrants == 12);
#endif
    std::puts("PASS infinite all 64 starts, native JSON/spoiler/save, default-off legacy, category/icons and bounded foreign receipts");
}
