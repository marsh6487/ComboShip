#include "2s2h/Rando/Rando.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
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
PlayState* gPlayState = nullptr;
int gFcCombo_SuppressRecord = 0;
u32 gRandoPickupSerial = 0;
u8 gItemSlots[77]{};
const uint8_t gPage3MaskItems[24]{};
int wandRule = WAND_RANDO_MEDALLIONS;
int notifications = 0;
int sharedObtained = 0;
bool Rando::gComboDormantGive = false;

extern "C" void FleetShared_OnNativeObtained(int) { ++sharedObtained; }
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    if (strcmp(name, "gRando.Options.RO_ELEMENTAL_WAND_SHUFFLE") == 0) return wandRule;
    return defaultValue;
}
namespace Notification {
struct Options { std::string prefix, message, suffix; };
void Emit(Options) { ++notifications; }
}
namespace Rando::MiscBehavior {
std::string BankRewardSourceSuffix(RandoCheckId) { return ""; }
}
namespace Rando::StaticData {
std::string GetItemName(RandoItemId item, bool, RandoCheckId, bool) { return Items.at(item).name; }
}

#include "editor_production.inc"

static void reset() {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    Nei_InitNewSave();
    notifications = sharedObtained = 0;
    gRandoPickupSerial = 0;
    // Ordinary inventory, bottles, masks, currency and upgrades must survive grants.
    auto& native = gSaveContext.save.saveInfo;
    for (int i = 0; i < 48; ++i) native.inventory.items[i] = (u8)(i + 1);
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
        auto* nei = Nei_Save();
        REQUIRE(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
        REQUIRE(Nei_GetOwnedItem(41) == EXT_ITEM_PHANTOM_HOURGLASS);
        REQUIRE(Nei_GetOwnedItem(44) == EXT_ITEM_SHADOW_CRYSTAL);
        REQUIRE(Nei_GetOwnedItem(47) == EXT_ITEM_ROD_OF_SEASONS);
        REQUIRE(nei->slateRunesOwned == 0x1F);
        REQUIRE(nei->seasonsOwned == 0x0F);
        REQUIRE(nei->caneSkills == 0x3F);
        REQUIRE(nei->wandRodsOwned == 0x3F);
        for (u8 mode = 0; mode < 6; ++mode) REQUIRE(Wand_ModeOwned(mode));
        REQUIRE(nei->shovelOwned && nei->dominionOwned && nei->bombArrowsOwned);
        REQUIRE(nei->pokeballOwned && nei->marioMaskOwned && nei->capeOwned);
        REQUIRE(Nei_GetOwnedItem(SLOT_ROCS) == ITEM_ROCS_CAPE);
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
    puts("PASS grant-all: current NEI handlers, four canonical slots, all powers/seasons/cane/wand rules, ordinary inventory and repeat safety");
#if __has_include("2s2h/DeveloperTools/NeiEditorItems.h")
    const struct { RandoItemId id; u8 slot; u16 item; } individual[] = {
        { RI_OOT_NEI_SHEIKAH_SLATE, 39, EXT_ITEM_SHEIKAH_SLATE },
        { RI_OOT_NEI_PHANTOM_HOURGLASS, 41, EXT_ITEM_PHANTOM_HOURGLASS },
        { RI_OOT_NEI_SHADOW_CRYSTAL, 44, EXT_ITEM_SHADOW_CRYSTAL },
        { RI_OOT_NEI_ROD_OF_SEASONS, 47, EXT_ITEM_ROD_OF_SEASONS },
    };
    for (const auto& item : individual) {
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
    REQUIRE(!NeiEditor::Grant(RI_OOT_NEI_HYLIAS_GRACE));
    REQUIRE(!NeiEditor::Grant(RI_BOW));
    REQUIRE(!NeiEditor::Grant(RI_UNKNOWN));
    REQUIRE(gRandoPickupSerial == 0 && sharedObtained == 0);
    REQUIRE(Nei_GetOwnedItem(41) == ITEM_NONE);
    REQUIRE(NeiEditor::Grant(RI_OOT_NEI_FIRE_ROD));
    REQUIRE(Nei_GetOwnedItem(SLOT_FIRE_ROD) == ITEM_ROD_FIRE);
    REQUIRE(Nei_GetOwnedItem(41) == ITEM_NONE);
    REQUIRE(RI_OOT_NEI_HYLIAS_GRACE == 194 && RI_OOT_NEI_FIRE_ROD == 192);
    puts("PASS individual canonical grants route through real randomizer recording; duplicates, ordinary/retired identities are ignored");

    // Any power can be the first grant and must also provide its usable host item.
    const RandoItemId runes[] = { RI_OOT_NEI_DESIRE_SENSOR, RI_OOT_NEI_SLATE_RUNE_BOMB,
        RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE, RI_OOT_NEI_SLATE_RUNE_STASIS, RI_OOT_NEI_SLATE_RUNE_CRYONIS };
    for (u8 rune = 0; rune < 5; ++rune) {
        reset();
        REQUIRE(NeiEditor::Grant(runes[rune]));
        REQUIRE(Nei_GetOwnedItem(39) == EXT_ITEM_SHEIKAH_SLATE);
        REQUIRE(Nei_Save()->slateRunesOwned == (1u << rune));
    }
    const RandoItemId seasons[] = { RI_OOT_NEI_SEASON_SPRING, RI_OOT_NEI_SEASON_SUMMER,
        RI_OOT_NEI_SEASON_AUTUMN, RI_OOT_NEI_SEASON_WINTER };
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
#endif
    return 0;
}
