// Native data and verbatim production serializers are supplied by run_save_tests.py.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#if HOST_MM
extern "C" {
#include "z64save.h"
#include "z64item.h"
#include "macros.h"
}
#else
#include "z64.h"
#include "macros.h"
#endif
#include "mods/extended_inventory.h"
#include "combo/rando/RpgStatsJson.h"
#if HOST_MM
#include "2s2h/FleetShipCombo/FleetComboOptions.h"
#else
#include "soh/FleetShipCombo/FleetComboOptions.h"
// Only the current JSON section needs fixture access. The real SaveData and
// LoadData templates remain unchanged; standard headers are already loaded.
#define private public
#include "soh/SaveManager.h"
#undef private
SaveManager* SaveManager::Instance = nullptr;
SaveManager::SaveManager() {}

// Unrelated sidecar I/O and session wheel tracking are not part of this section
// persistence test. The real NEI save/load bodies still call these observers.
static void Bottle_WheelResetTracking() {}
static void Picto_SyncRead() {}
static void TradeItems_SyncWrite() {}
static void TradeItems_SyncRead() {}
#endif

using json = nlohmann::json;
extern "C" {
SaveContext gSaveContext{};
// Observe no engine audit service in this synchronous storage fixture. Real
// slot writers and serializer scopes retain their production audit calls.
void ItemGrantAudit_Begin(const char*, int, int, int) {}
void ItemGrantAudit_End(void) {}
}

#include "nei_save_production.inc"

static_assert(sizeof(NeiSaveData::ownedItems[0]) == sizeof(uint16_t));
#if HOST_MM
static_assert(offsetof(NeiSaveData, hyliasGraceOwned) >=
              offsetof(NeiSaveData, comboRpg) + sizeof(NeiSaveData::comboRpg));
#else
static_assert(offsetof(NeiSaveData, hyliasGraceOwned) > offsetof(NeiSaveData, seasonsOwned));
#endif
static_assert(offsetof(NeiSaveData, phantomHourglassOwned) == offsetof(NeiSaveData, hyliasGraceOwned) + 1);

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void Reset() {
    gSaveContext = {};
#if HOST_MM
    Nei_InitNewSave();
#else
    NeiSave_Init(false);
#endif
}

static json SaveBlob() {
    json section = json::object();
#if HOST_MM
    to_json(section, *Nei_Save());
#else
    auto* manager = SaveManager::Instance;
    manager->currentJsonContext = &section;
    NeiSave_Save(&gSaveContext, 0, true);
    Check(manager->currentJsonContext == &section, "SaveManager restored the NEI section after array traversal");
    manager->currentJsonContext = nullptr;
#endif
    // Force real textual serialization/parsing rather than reuse a JSON node.
    return json::parse(section.dump());
}

static void LoadBlob(json section) {
    std::memset(Nei_Save(), 0xA5, sizeof(*Nei_Save()));
#if HOST_MM
    from_json(section, *Nei_Save());
#else
    auto* manager = SaveManager::Instance;
    manager->currentJsonContext = &section;
    NeiSave_Load();
    Check(manager->currentJsonContext == &section, "SaveManager restored the NEI section after loading arrays");
    manager->currentJsonContext = nullptr;
#endif
}

static uint16_t Selected() {
    return ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS);
}

static void CheckFlags(uint8_t grace, uint8_t hourglass, const char* message) {
    Check(Nei_Save()->hyliasGraceOwned == grace && Nei_Save()->phantomHourglassOwned == hourglass, message);
}

static void CheckBothOwnedRoundtrips() {
    for (uint16_t first : {uint16_t(ITEM_HYLIAS_GRACE), uint16_t(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        Reset();
        GraceHourglass_Grant(first);
        GraceHourglass_Grant(first == ITEM_HYLIAS_GRACE ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_HYLIAS_GRACE);
        CheckFlags(1, 1, "both items are independently owned before save");
        Check(Selected() == first, "granting a sibling retained the selected shared item");
        ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS - 1, 0x0234);
        ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS + 1, 0xF123);
        LoadBlob(SaveBlob());
        CheckFlags(1, 1, "both appended ownership flags survived the JSON roundtrip");
        Check(Selected() == first, "the selected full u16 Grace/hourglass cell survived the JSON roundtrip");
        Check(ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS - 1) == 0x0234 &&
                  ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS + 1) == 0xF123,
              "adjacent custom cells retained their full u16 values");
        GraceHourglass_Heal();
        Check(Selected() == first, "healing retained the saved selection when both flags are owned");
        CheckFlags(1, 1, "healing did not drop a persisted sibling");
    }
    std::puts("PASS native NEI save: both acquisition orders, both flags and selected/adjacent u16 cells");
}

static void CheckLegacySelectedItems() {
    for (uint16_t selected : {uint16_t(ITEM_HYLIAS_GRACE), uint16_t(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        std::array<uint16_t, 48> owned;
        owned.fill(ITEM_NONE);
        owned[SLOT_PHANTOM_HOURGLASS - 24] = selected;
        const json legacy = {{"ownedItems", owned}};
        LoadBlob(json::parse(legacy.dump()));
        CheckFlags(0, 0, "old blobs absent the new keys default both ownership flags to zero");
        Check(Selected() == selected, "legacy load retained its selected full u16 item");
        GraceHourglass_Heal();
        CheckFlags(selected == ITEM_HYLIAS_GRACE, selected == EXT_ITEM_PHANTOM_HOURGLASS,
                   "healing inferred only the selected legacy item's ownership");
        Check(Selected() == selected, "legacy healing retained its original selection");
        LoadBlob(SaveBlob());
        CheckFlags(selected == ITEM_HYLIAS_GRACE, selected == EXT_ITEM_PHANTOM_HOURGLASS,
                   "the recovered legacy flag persisted on the next save");
        Check(Selected() == selected, "the recovered legacy selection persisted on the next save");
    }
    std::puts("PASS native NEI load: absent ownership keys reset stale flags, then heal and resave legacy selection");
}

static void CheckFlagsOnlyRepair() {
    for (uint8_t grace : {uint8_t(0), uint8_t(1)}) {
        for (uint8_t hourglass : {uint8_t(0), uint8_t(1)}) {
            LoadBlob({{"hyliasGraceOwned", grace}, {"phantomHourglassOwned", hourglass}});
            CheckFlags(grace, hourglass, "flags-only JSON retained each ownership combination");
            for (uint16_t item : Nei_Save()->ownedItems) {
                Check(item == ITEM_NONE, "absent ownedItems default to u16 ITEM_NONE, not zero or 0xFFFF");
            }
            GraceHourglass_Heal();
            const uint16_t expected = grace ? ITEM_HYLIAS_GRACE
                                           : hourglass ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_NONE;
            Check(Selected() == expected, "flags-only repair selected the owned item with Grace first when both owned");
            GraceHourglass_Heal();
            Check(Selected() == expected, "flags-only repair is idempotent");
            CheckFlags(grace, hourglass, "flags-only repair preserved both ownership facts");
            LoadBlob(SaveBlob());
            Check(Selected() == expected, "flags-only repaired selection persisted");
            CheckFlags(grace, hourglass, "flags-only repaired ownership persisted");
        }
    }
    LoadBlob(json::object());
    CheckFlags(0, 0, "an empty old NEI blob defaulted both flags to zero");
    Check(Nei_Save()->slateMode == 0 && Nei_Save()->slateRunesOwned == 0,
          "an empty old NEI blob cleared stale Slate mode and ownership");
    std::puts("PASS native NEI repair: four flags-only combinations, empty old blob, idempotence and resave");
}

static void CheckSlateRoundtrip(uint8_t mask, uint8_t mode) {
    Reset();
    ExtInv_SetSlotItem(SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE);
    Nei_Save()->slateRunesOwned = mask;
    Slate_SetRune(mode);
    Check(Nei_Save()->slateMode == mode, "an owned Slate rune was selectable before save");
    LoadBlob(SaveBlob());
    Check(ExtInv_GetSlotItem(SLOT_SHEIKAH_SLATE) == EXT_ITEM_SHEIKAH_SLATE,
          "the full u16 Slate cell survived the JSON roundtrip");
    Check(Nei_Save()->slateRunesOwned == mask && Nei_Save()->slateMode == mode,
          "Slate owned runes and selected mode survived the JSON roundtrip");
    Check(Slate_GetRune() == mode, "the loaded owned Slate mode remained selected");
    Check(Nei_Save()->slateRunesOwned == mask, "resolving the selected mode preserved its ownership mask");
}

static void CheckSlateModes() {
    const uint8_t all = (1u << SLATE_RUNE_COUNT) - 1u;
    for (uint8_t mode = 0; mode < SLATE_RUNE_COUNT; ++mode) {
        CheckSlateRoundtrip(1u << mode, mode);
        CheckSlateRoundtrip(all, mode);
        if (mode % 2 == 0) {
            CheckSlateRoundtrip(0x15, mode);
        }
    }
    Reset();
    ExtInv_SetSlotItem(SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE);
    Nei_Save()->slateRunesOwned = 1u << SLATE_RUNE_CRYONIS;
    Nei_Save()->slateMode = 255;
    LoadBlob(SaveBlob());
    Check(Nei_Save()->slateMode == 255, "the serializer retained the raw invalid legacy mode for runtime repair");
    Check(Slate_GetRune() == SLATE_RUNE_CRYONIS, "the real Slate resolver repaired to the only owned rune");
    LoadBlob(SaveBlob());
    Check(Nei_Save()->slateMode == SLATE_RUNE_CRYONIS &&
              Nei_Save()->slateRunesOwned == (1u << SLATE_RUNE_CRYONIS),
          "the repaired Slate mode and ownership survived the next save");
    std::puts("PASS native Slate save: all five modes with single/all/sparse ownership, u16 cell and invalid-mode repair");
}

static void CheckFleetOwnershipMerge() {
    Reset();
    Fixture_FCO_APPLY(json{{"hyliasGraceOwned", 1}, {"phantomHourglassOwned", 1}});
    CheckFlags(1, 1, "the real Fleet option table imported both appended ownership flags");
    Fixture_FCO_APPLY(json{{"hyliasGraceOwned", 0}, {"phantomHourglassOwned", 0}});
    CheckFlags(1, 1, "MAX merge rejected incoming ownership downgrades");
    Fixture_FCO_APPLY(json::object());
    CheckFlags(1, 1, "an old shared blob without the flags preserved local ownership");
    json exported = json::object();
    Fixture_FCO_EXTRACT(exported);
    Reset();
    Fixture_FCO_EXTRACT(exported);
    Fixture_FCO_APPLY(json::parse(exported.dump()));
    CheckFlags(1, 1, "MAX export retained previous shared ownership when this host had zero flags");
    GraceHourglass_Heal();
    Check(Selected() == ITEM_HYLIAS_GRACE, "imported flags repaired the shared cell through the real helper");
    LoadBlob(SaveBlob());
    CheckFlags(1, 1, "Fleet-imported ownership persisted through the native serializer");
    std::puts("PASS native Fleet option table: both flags import/export with MAX, missing-key retention and persistence");
}

int main() {
#if !HOST_MM
    SaveManager manager;
    SaveManager::Instance = &manager;
#endif
    CheckBothOwnedRoundtrips();
    CheckLegacySelectedItems();
    CheckFlagsOnlyRepair();
    CheckSlateModes();
    CheckFleetOwnershipMerge();
    std::puts(HOST_MM ? "PASS host: MM native save" : "PASS host: OoT native save");
    return 0;
}
