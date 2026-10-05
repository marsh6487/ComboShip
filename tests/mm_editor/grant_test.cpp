#include "2s2h/Rando/Rando.h"
extern "C" {
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
}
#include "mods/extended_player.h"
#include "mods/items/logic/item_cane_of_somaria.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "2s2h/FleetShipCombo/FleetComboIds.h"
#include "tests/test_require.h"
#include <cstring>
#include <iostream>
#include <unordered_map>

#define SPDLOG_INFO(...) ((void)0)

SaveContext gSaveContext{};
PlayState *gPlayState = nullptr;
int gFcCombo_SuppressRecord = 0;
u32 gRandoPickupSerial = 0;
u8 gItemSlots[77]{};
const uint8_t gPage3MaskItems[24]{};
int wandRule = WAND_RANDO_MEDALLIONS;
int notifications = 0;
int sharedObtained = 0;
bool Rando::gComboDormantGive = false;
extern "C" void ItemGrantAudit_Begin(const char *, int, int, int) {}
extern "C" void ItemGrantAudit_End(void) {}
void ExtInv_SetOotSlotItem(int, u8) { REQUIRE(false); }
u8 ExtInv_GetOotSlotItem(int) {
    REQUIRE(false);
    return ITEM_NONE;
}
extern "C" s32 TradeAdult_OwnedCount(void) {
    REQUIRE(false);
    return 0;
}
extern "C" u8 TradeAdult_CellItem(void) {
    REQUIRE(false);
    return ITEM_NONE;
}

extern "C" void FleetShared_OnNativeObtained(int) { ++sharedObtained; }
int32_t CVarGetInteger(const char *name, int32_t defaultValue) {
    if (strcmp(name, "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE") == 0)
        return wandRule;
    return defaultValue;
}
namespace Notification {
struct Options {
    std::string prefix, message, suffix;
};
void Emit(Options) { ++notifications; }
} // namespace Notification
namespace Rando::MiscBehavior {
std::string BankRewardSourceSuffix(RandoCheckId) { return ""; }
} // namespace Rando::MiscBehavior
namespace Rando::StaticData {
std::string GetItemName(RandoItemId item, bool, RandoCheckId, bool) { return Items.at(item).name; }
} // namespace Rando::StaticData

#include "editor_production.inc"

static void reset() {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    Nei_InitNewSave();
    notifications = sharedObtained = 0;
    gRandoPickupSerial = 0;
    // Ordinary inventory, bottles, masks, currency and upgrades must survive grants.
    auto &native = gSaveContext.save.saveInfo;
    for (int i = 0; i < 48; ++i)
        native.inventory.items[i] = (u8)(i + 1);
    native.inventory.items[SLOT_BOTTLE_1] = ITEM_MUSHROOM;
    native.inventory.items[SLOT_BOTTLE_2] = ITEM_FISH;
    native.inventory.upgrades = 0x123456;
    native.playerData.rupees = 1667;
}

int main() {
    for (int rule = WAND_RANDO_MEDALLIONS; rule <= WAND_RANDO_ELEMENTAL; ++rule) {
        wandRule = rule;
        reset();
        const auto nativeBefore = gSaveContext.save.saveInfo;
        NeiEditor::GrantAll();
        auto *nei = Nei_Save();
        REQUIRE(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
        REQUIRE(Nei_GetOwnedItem(41) == ITEM_HYLIAS_GRACE);
        REQUIRE(NeiEditor::IsOwned(RI_OOT_NEI_HYLIAS_GRACE));
        REQUIRE(NeiEditor::IsOwned(RI_OOT_NEI_PHANTOM_HOURGLASS));
        REQUIRE(Nei_GetOwnedItem(44) == EXT_ITEM_SHADOW_CRYSTAL);
        REQUIRE(Nei_GetOwnedItem(47) == EXT_ITEM_ROD_OF_SEASONS);
        REQUIRE(nei->slateRunesOwned == 0x1F);
        REQUIRE(nei->seasonsOwned == 0x0F);
        REQUIRE(nei->caneSkills == 0x3F);
        REQUIRE(nei->wandRodsOwned == 0x3F);
        for (u8 mode = 0; mode < 6; ++mode)
            REQUIRE(Wand_ModeOwned(mode));
        REQUIRE(nei->shovelOwned && nei->dominionOwned && nei->bombArrowsOwned);
        REQUIRE(nei->pokeballOwned && nei->marioMaskOwned && nei->capeOwned);
        REQUIRE(Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE);
#ifndef MM_EDITOR_CONTROL
        for (const auto id : NeiEditor::Catalog())
            REQUIRE(NeiEditor::IsOwned(id));
#endif
        REQUIRE(memcmp(&nativeBefore, &gSaveContext.save.saveInfo, sizeof(nativeBefore)) == 0);

        // An already-complete grant must not recount FC items, send new notifications,
        // choose a different rune/season/tool, or change any unrelated NEI state.
        nei->slateMode = SLATE_RUNE_BOMB;
        nei->season = SEASON_SPRING;
        const auto neiBefore = *nei;
        const int countBefore = sharedObtained;
        const u32 serialBefore = gRandoPickupSerial;
        NeiEditor::GrantAll();
        REQUIRE(memcmp(&neiBefore, nei, sizeof(neiBefore)) == 0);
        REQUIRE(sharedObtained == countBefore && gRandoPickupSerial == serialBefore);
        REQUIRE(memcmp(&nativeBefore, &gSaveContext.save.saveInfo, sizeof(nativeBefore)) == 0);
    }
    puts("PASS grant-all: current NEI handlers, four canonical slots, all powers/seasons/cane/wand rules, ordinary "
         "inventory and repeat safety");
#ifndef MM_EDITOR_CONTROL
    const struct {
        RandoItemId id;
        u8 slot;
        u16 item;
    } individual[] = {
        {RI_OOT_NEI_SHEIKAH_SLATE, 39, EXT_ITEM_SHEIKAH_SLATE},
        {RI_OOT_NEI_PHANTOM_HOURGLASS, 41, EXT_ITEM_PHANTOM_HOURGLASS},
        {RI_OOT_NEI_HYLIAS_GRACE, 41, ITEM_HYLIAS_GRACE},
        {RI_OOT_NEI_SHADOW_CRYSTAL, 44, EXT_ITEM_SHADOW_CRYSTAL},
        {RI_OOT_NEI_ROD_OF_SEASONS, 47, EXT_ITEM_ROD_OF_SEASONS},
    };
    for (const auto &item : individual) {
        reset();
        const auto nativeBefore = gSaveContext.save.saveInfo;
        REQUIRE(NeiEditor::Grant(item.id));
        REQUIRE(Nei_GetOwnedItem(item.slot) == item.item);
        REQUIRE(gRandoPickupSerial == 1 && sharedObtained == 1);
        const auto neiBefore = *Nei_Save();
        REQUIRE(!NeiEditor::Grant(item.id));
        REQUIRE(memcmp(&neiBefore, Nei_Save(), sizeof(neiBefore)) == 0);
        REQUIRE(gRandoPickupSerial == 1 && sharedObtained == 1);
        REQUIRE(memcmp(&nativeBefore, &gSaveContext.save.saveInfo, sizeof(nativeBefore)) == 0);
    }
    reset();
    for (const auto first : {RI_OOT_NEI_HYLIAS_GRACE, RI_OOT_NEI_PHANTOM_HOURGLASS}) {
        reset();
        const auto second = first == RI_OOT_NEI_HYLIAS_GRACE ? RI_OOT_NEI_PHANTOM_HOURGLASS : RI_OOT_NEI_HYLIAS_GRACE;
        REQUIRE(NeiEditor::Grant(first));
        const auto selected = Nei_GetOwnedItem(41);
        REQUIRE(NeiEditor::Grant(second));
        REQUIRE(Nei_GetOwnedItem(41) == selected);
        REQUIRE(NeiEditor::IsOwned(first) && NeiEditor::IsOwned(second));
        const auto before = *Nei_Save();
        REQUIRE(!NeiEditor::Grant(first) && !NeiEditor::Grant(second));
        REQUIRE(memcmp(&before, Nei_Save(), sizeof(before)) == 0);
        REQUIRE(gRandoPickupSerial == 2 && sharedObtained == 2);
        Nei_SetOwnedItem(41, ITEM_NONE);
        REQUIRE(NeiEditor::Grant(first));
        REQUIRE(NeiEditor::IsOwned(first) && NeiEditor::IsOwned(second));
        REQUIRE(gRandoPickupSerial == 2 && sharedObtained == 2);
    }
    puts("PASS Grace/hourglass: both pickup orders retain selection and ownership; regrants do not recount");
    reset();
    REQUIRE(!NeiEditor::Grant(RI_BOW));
    REQUIRE(!NeiEditor::Grant(RI_UNKNOWN));
    REQUIRE(gRandoPickupSerial == 0 && sharedObtained == 0);
    REQUIRE(Nei_GetOwnedItem(41) == ITEM_NONE);
    REQUIRE(NeiEditor::Grant(RI_OOT_NEI_FIRE_ROD));
    REQUIRE(Nei_GetOwnedItem(SLOT_FIRE_ROD) == ITEM_ROD_FIRE);
    REQUIRE(Nei_GetOwnedItem(41) == ITEM_NONE);
    REQUIRE(RI_OOT_NEI_HYLIAS_GRACE == 194 && RI_OOT_NEI_FIRE_ROD == 192);
    puts("PASS individual canonical grants route through real randomizer recording; duplicates and ordinary "
         "identities are ignored");

    // Any power can be the first grant and must also provide its usable host item.
    const struct {
        RandoItemId id;
        u8 mask;
    } runes[] = {
        {RI_OOT_NEI_DESIRE_SENSOR, 16},    {RI_OOT_NEI_SLATE_RUNE_BOMB, 1},    {RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE, 8},
        {RI_OOT_NEI_SLATE_RUNE_STASIS, 2}, {RI_OOT_NEI_SLATE_RUNE_CRYONIS, 4},
    };
    for (const auto &rune : runes) {
        reset();
        REQUIRE(NeiEditor::Grant(rune.id));
        REQUIRE(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
        REQUIRE(Nei_Save()->slateRunesOwned == rune.mask);
    }
    const RandoItemId seasons[] = {RI_OOT_NEI_SEASON_SPRING, RI_OOT_NEI_SEASON_SUMMER, RI_OOT_NEI_SEASON_AUTUMN,
                                   RI_OOT_NEI_SEASON_WINTER};
    for (u8 season = 0; season < 4; ++season) {
        reset();
        REQUIRE(NeiEditor::Grant(seasons[season]));
        REQUIRE(Nei_GetOwnedItem(47) == EXT_ITEM_ROD_OF_SEASONS);
        REQUIRE(Nei_Save()->seasonsOwned == (1u << season));
    }
    for (u8 initialSkill = 0; initialSkill < 6; ++initialSkill) {
        reset();
        Cane_GiveSkill(initialSkill);
        NeiEditor::GrantAll();
        REQUIRE(Nei_Save()->caneSkills == 0x3F);
    }
    reset();
    Nei_SetOwnedItem(SLOT_ROCS, ITEM_ROCS_CAPE);
    REQUIRE(!NeiEditor::Grant(RI_OOT_NEI_ROCS_FEATHER));
    REQUIRE(Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE);
    puts("PASS first-power host ownership, partial cane completion and Roc cape preservation");

    // The existing Clear Custom Items control removes cells while retaining powers.
    // Give All must restore the hosts without recounting already-earned powers.
    reset();
    NeiEditor::GrantAll();
    Nei_Save()->slateMode = SLATE_RUNE_BOMB;
    Nei_Save()->season = SEASON_SPRING;
    const auto powered = *Nei_Save();
    for (u8 slot = 24; slot < 48; ++slot)
        Nei_SetOwnedItem(slot, ITEM_NONE);
    REQUIRE(!NeiEditor::IsOwned(RI_OOT_NEI_CANE_OF_SOMARIA));
    REQUIRE(!NeiEditor::IsOwned(RI_OOT_NEI_DOMINION_ROD));
    NeiEditor::GrantAll();
    REQUIRE(Nei_GetOwnedItem(SLOT_CANE_OF_SOMARIA) == ITEM_CANE_OF_SOMARIA);
    REQUIRE(Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_SHOVEL || Nei_GetOwnedItem(SLOT_SHOVEL) == ITEM_DOMINION_ROD);
    REQUIRE(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
    REQUIRE(Nei_GetOwnedItem(47) == EXT_ITEM_ROD_OF_SEASONS);
    REQUIRE(Nei_Save()->slateMode == SLATE_RUNE_BOMB && Nei_Save()->season == SEASON_SPRING);
    REQUIRE(Nei_Save()->caneSkills == powered.caneSkills);
    const int caneFc = FcCombo_ItemForNative(RI_OOT_NEI_CANE_OF_SOMARIA);
    REQUIRE(caneFc >= 0);
    REQUIRE(Nei_Save()->comboObtainedFc[caneFc] == powered.comboObtainedFc[caneFc]);
    REQUIRE(Nei_Save()->comboAppliedFc[caneFc] == powered.comboAppliedFc[caneFc]);
    puts("PASS clear/regrant: retained powers recover their host cells without recounting cane skills or changing "
         "rune/season selection");

    // Medallions make a wand mode usable, but do not constitute an earned wand.
    wandRule = WAND_RANDO_MEDALLIONS;
    for (const auto id : {RI_OOT_NEI_ELEMENTAL_WAND, RI_OOT_NEI_WAND_SAND_ROD}) {
        reset();
        Rando::GiveItem(RI_OOT_MEDALLION_SPIRIT);
        REQUIRE(Wand_ModeOwned(WAND_MODE_SAND));
        REQUIRE(Nei_GetOwnedItem(SLOT_ELEMENTAL_WAND) == ITEM_NONE);
        const int fc = FcCombo_ItemForNative(id);
        REQUIRE(fc >= 0);
        const auto obtained = Nei_Save()->comboObtainedFc[fc];
        const auto applied = Nei_Save()->comboAppliedFc[fc];
        REQUIRE(NeiEditor::Grant(id));
        REQUIRE(Nei_Save()->wandRodsOwned & (1u << WAND_MODE_SAND));
        REQUIRE(Nei_Save()->comboObtainedFc[fc] == obtained + 1);
        REQUIRE(Nei_Save()->comboAppliedFc[fc] == applied + 1);
    }
    reset();
    for (const auto id : NeiEditor::wandMedallions)
        Rando::GiveItem(id);
    NeiEditor::GrantAll();
    REQUIRE(Nei_Save()->wandRodsOwned == 0x3F);
    for (const auto id : NeiEditor::wandItems)
        REQUIRE(NeiEditor::IsOwned(id));

    // A bare earned wand needs only its missing prerequisite. Clearing its host
    // must not cause the previously earned wand to be awarded or counted again.
    for (bool clear : {false, true}) {
        reset();
        Rando::GiveItem(RI_OOT_NEI_ELEMENTAL_WAND);
        REQUIRE(!Wand_ModeOwned(WAND_MODE_SAND));
        REQUIRE(Nei_Save()->wandRodsOwned & (1u << WAND_MODE_SAND));
        const int fc = FcCombo_ItemForNative(RI_OOT_NEI_ELEMENTAL_WAND);
        REQUIRE(fc >= 0);
        const auto obtained = Nei_Save()->comboObtainedFc[fc];
        const auto applied = Nei_Save()->comboAppliedFc[fc];
        if (clear)
            Nei_SetOwnedItem(SLOT_ELEMENTAL_WAND, ITEM_NONE);
        REQUIRE(NeiEditor::Grant(RI_OOT_NEI_ELEMENTAL_WAND));
        REQUIRE(Wand_ModeOwned(WAND_MODE_SAND));
        REQUIRE(Nei_GetOwnedItem(SLOT_ELEMENTAL_WAND) == ITEM_ELEMENTAL_WAND);
        REQUIRE(Nei_Save()->comboObtainedFc[fc] == obtained);
        REQUIRE(Nei_Save()->comboAppliedFc[fc] == applied);
        const auto before = *Nei_Save();
        REQUIRE(!NeiEditor::Grant(RI_OOT_NEI_ELEMENTAL_WAND));
        REQUIRE(memcmp(&before, Nei_Save(), sizeof(before)) == 0);
    }
    puts("PASS wand earning versus usability: medallion-only canonical grants, complete siblings and bare/cleared "
         "owned wand prerequisite repair without recounting");
#endif
    return 0;
}
