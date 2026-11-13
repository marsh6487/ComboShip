#include "z64save.h"
#include "z64item.h"
#ifdef CORE_MM
#include "z64dma.h"
#endif
#include "z64interface.h"
#ifdef CORE_MM
#include "mods/nei_oot_compat.h"
#endif
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "../../combo/ComboGameMode.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>

// CORE_ITEM_DEFINES

SaveContext gSaveContext{};
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save(void) { return &nei; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return nei.ownedItems[slot - 24]; }
extern "C" uint8_t RitoMask_IsOwned(void) { return 0; }
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
#ifdef CORE_MM
unsigned char OotAssets_Available(void) { return 1; }
uint8_t ExtInv_GetOotSlotItem(int) { assert(false && "trade test reached an unrelated virtual item"); return ITEM_NONE; }
#else
unsigned char Picto_IsOwned(void) { return 0; }
unsigned char PowerKeg_IsOwned(void) { return 0; }
#endif

// PRODUCTION_CORE

static void Reset(void) {
    nei = {};
    gSaveContext = {};
#ifdef CORE_MM
    memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
           sizeof(gSaveContext.save.saveInfo.inventory.items));
    memset(gSaveContext.save.saveInfo.equips.buttonItems, ITEM_NONE,
           sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
    memset(gSaveContext.save.saveInfo.equips.cButtonSlots, SLOT_NONE,
           sizeof(gSaveContext.save.saveInfo.equips.cButtonSlots));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems, ITEM_NONE,
           sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots, SLOT_NONE,
           sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots));
#else
    memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
    for (auto* equip : {&gSaveContext.equips, &gSaveContext.childEquips, &gSaveContext.adultEquips}) {
        memset(equip->buttonItems, ITEM_NONE, sizeof(equip->buttonItems));
        memset(equip->cButtonSlots, SLOT_NONE, sizeof(equip->cButtonSlots));
    }
#endif
}

static void TestConsumedHistoryUnavailable(void) {
    // Catches readers that use earned history without honoring consumed parity.
    for (int index = 0; index < 23; ++index) {
        Reset();
        nei.tradeAdultOwned = 0x7FFFFF;
        nei.tradeAdultRevision[index] = 1;
        assert(!TradeAdult_IsOwnedIndex(index));
        assert(TradeAdult_OwnedCount() == 22);
        for (int ordinal = 0; ordinal < 22; ++ordinal)
            assert(TradeAdult_OwnedAt(ordinal) != index);
        assert(TradeAdult_OwnedAt(22) == -1);
    }
}

#ifndef CORE_RED_PROBE
static void TestRevisionLifecycle(void) {
    // Catches non-idempotent duplicate events, history replay resurrection, and
    // legitimate reacquisition that fails to make a new copy selectable.
    for (int index = 0; index < 23; ++index) {
        Reset();
        const uint32_t bit = 1u << index;
        TradeAdult_GiveIndex(index);
        assert(nei.tradeAdultOwned == bit && nei.tradeAdultRevision[index] == 0);
        assert(TradeAdult_IsOwnedIndex(index) && !TradeAdult_IsConsumedIndex(index));
        TradeAdult_GiveIndex(index);
        assert(nei.tradeAdultRevision[index] == 0);
        TradeAdult_ConsumeIndex(index);
        assert(nei.tradeAdultOwned == bit && nei.tradeAdultRevision[index] == 1);
        assert(!TradeAdult_IsOwnedIndex(index) && TradeAdult_IsConsumedIndex(index));
        TradeAdult_ConsumeIndex(index);
        TradeAdult_FoldCurrent(TradeAdult_ItemId(index));
        assert(nei.tradeAdultRevision[index] == 1 && !TradeAdult_IsOwnedIndex(index));
        TradeAdult_GiveIndex(index);
        assert(nei.tradeAdultRevision[index] == 2 && TradeAdult_IsOwnedIndex(index));
        TradeAdult_ConsumeIndex(index);
        assert(nei.tradeAdultRevision[index] == 3);
        TradeAdult_GiveIndex(index);
        assert(nei.tradeAdultRevision[index] == 4);
        TradeAdult_SetOwnedIndex(index, 0);
        assert(nei.tradeAdultOwned == 0 && nei.tradeAdultRevision[index] == 5);
        nei.tradeAdultOwned |= bit; // stale earned-history merge
        assert(!TradeAdult_IsOwnedIndex(index));
        TradeAdult_SetOwnedIndex(index, 1);
        assert(nei.tradeAdultRevision[index] == 6 && TradeAdult_IsOwnedIndex(index));
    }
    Reset();
    const NeiSaveData initial = nei;
    for (int index : {-1, 23, 99}) {
        TradeAdult_ConsumeIndex(index);
        TradeAdult_GiveIndex(index);
        TradeAdult_SetOwnedIndex(index, 0);
        assert(!TradeAdult_IsOwnedIndex(index) && !TradeAdult_IsConsumedIndex(index));
    }
    TradeAdult_ConsumeItem(ITEM_NONE);
    TradeAdult_ConsumeItem(ITEM_BOW);
    TradeAdult_GiveItem(ITEM_NONE);
    TradeAdult_FoldCurrent(ITEM_NONE);
    assert(memcmp(&initial, &nei, sizeof(nei)) == 0);
    assert(TradeAdult_IndexOfItem(ITEM_NONE) == -1);
}

static void TestSaturatingRevision(void) {
    // Catches unsigned wrap, which would make stale peers win after exhaustion.
    Reset();
    nei.tradeAdultOwned = 1;
    nei.tradeAdultRevision[0] = UINT32_MAX - 1;
    TradeAdult_ConsumeIndex(0);
    assert(nei.tradeAdultRevision[0] == UINT32_MAX && !TradeAdult_IsOwnedIndex(0));
    TradeAdult_ConsumeIndex(0);
    TradeAdult_GiveIndex(0);
    assert(nei.tradeAdultRevision[0] == UINT32_MAX && !TradeAdult_IsOwnedIndex(0));
}

static void TestPendantPermanentPerk(void) {
    // Catches loss of the earned combat perk or resetting its independent toggle.
    for (int off : {0, 1}) {
        Reset();
        nei.pendantEffectOff = off;
        TradeAdult_GiveIndex(19);
        assert(nei.pendantOwned == 1 && nei.pendantEffectOff == off);
        TradeAdult_ConsumeIndex(19);
        assert(!TradeAdult_IsOwnedIndex(19) && nei.pendantOwned == 1);
        assert(nei.pendantEffectOff == off);
        Reset();
        nei.tradeAdultOwned = 1u << 19; // legacy earned pendant, perk not latched yet
        nei.pendantEffectOff = off;
        TradeAdult_ConsumeIndex(19);
        assert(nei.pendantOwned == 1 && nei.pendantEffectOff == off);
        Reset();
        TradeAdult_SetOwnedIndex(19, 0); // removing a never-earned entry grants nothing
        assert(nei.pendantOwned == 0);
    }
}

static void TestExactNativeCleanup(void) {
    // Each real item must be removed from every native trade home and all seven
    // equip bindings; unrelated contents and B/form data must remain byte-identical.
    for (int index = 0; index < 23; ++index) {
        const u8 item = TradeAdult_ItemId(index);
        if (item == ITEM_NONE) continue; // MM's fourteen foreign entries have no byte ID
        for (int mixed : {0, 1}) {
            Reset();
            nei.tradeAdultOwned = 1u << index;
#ifdef CORE_MM
            auto& inventory = gSaveContext.save.saveInfo.inventory;
            for (int slot : {SLOT_TRADE_DEED, SLOT_TRADE_KEY_MAMA, SLOT_TRADE_COUPLE})
                inventory.items[slot] = item;
            inventory.items[SLOT_BOW] = ITEM_BOW;
            for (int form = 0; form < 4; ++form) {
                BUTTON_ITEM_EQUIP(form, EQUIP_SLOT_B) = item;
                for (int button = 1; button <= 3; ++button) {
                    BUTTON_ITEM_EQUIP(form, button) = mixed && button % 2 ? ITEM_BOW : item;
                    C_SLOT_EQUIP(form, button) = SLOT_BOW; // cleanup follows identity, not slot
                }
                for (int button = 0; button < 4; ++button) {
                    DPAD_BUTTON_ITEM_EQUIP(form, button) = mixed && button % 2 ? ITEM_BOW : item;
                    DPAD_SLOT_EQUIP(form, button) = SLOT_BOW;
                }
            }
            SaveContext expected = gSaveContext;
            for (int slot : {SLOT_TRADE_DEED, SLOT_TRADE_KEY_MAMA, SLOT_TRADE_COUPLE})
                expected.save.saveInfo.inventory.items[slot] = ITEM_NONE;
            for (int button = 1; button <= 3; ++button) {
                if (expected.save.saveInfo.equips.buttonItems[0][button] == item) {
                    expected.save.saveInfo.equips.buttonItems[0][button] = ITEM_NONE;
                    expected.save.saveInfo.equips.cButtonSlots[0][button] = SLOT_NONE;
                }
            }
            for (int button = 0; button < 4; ++button) {
                if (expected.save.shipSaveInfo.dpadEquips.dpadItems[0][button] == item) {
                    expected.save.shipSaveInfo.dpadEquips.dpadItems[0][button] = ITEM_NONE;
                    expected.save.shipSaveInfo.dpadEquips.dpadSlots[0][button] = SLOT_NONE;
                }
            }
#else
            for (int slot : {SLOT_TRADE_ADULT, SLOT_TRADE_CHILD}) gSaveContext.inventory.items[slot] = item;
            gSaveContext.inventory.items[SLOT_BOW] = ITEM_BOW;
            for (auto* equip : {&gSaveContext.equips, &gSaveContext.childEquips, &gSaveContext.adultEquips}) {
                equip->buttonItems[0] = item;
                for (int button = 1; button < 8; ++button) {
                    equip->buttonItems[button] = mixed && button % 2 ? ITEM_BOW : item;
                    equip->cButtonSlots[button - 1] = SLOT_BOW;
                }
            }
            SaveContext expected = gSaveContext;
            expected.inventory.items[SLOT_TRADE_ADULT] = ITEM_NONE;
            expected.inventory.items[SLOT_TRADE_CHILD] = ITEM_NONE;
            for (auto* equip : {&expected.equips, &expected.childEquips, &expected.adultEquips}) {
                for (int button = 1; button < 8; ++button) {
                    if (equip->buttonItems[button] == item) {
                        equip->buttonItems[button] = ITEM_NONE;
                        equip->cButtonSlots[button - 1] = SLOT_NONE;
                    }
                }
            }
#endif
            TradeAdult_ConsumeItem(item);
            assert(memcmp(&gSaveContext, &expected, sizeof(expected)) == 0);
            assert(nei.tradeAdultRevision[index] == 1 && !TradeAdult_IsOwnedItem(item));
            TradeAdult_ReconcileConsumed();
            assert(memcmp(&gSaveContext, &expected, sizeof(expected)) == 0);
        }
        Reset();
        TradeAdult_ConsumeItem(item); // native legacy hand-in before the wheel ever folded it
        assert(nei.tradeAdultOwned == (1u << index));
        assert(nei.tradeAdultRevision[index] == 1 && !TradeAdult_IsOwnedItem(item));
    }
}

static void TestIncomingRetirement(void) {
    // Catches synchronization that changes only the ledger, leaving a stale slot usable.
    Reset();
    nei.tradeAdultOwned = (1u << 11) | (1u << 16);
    nei.tradeAdultRevision[11] = 7;
    const u8 consumed = TradeAdult_ItemId(11), kept = TradeAdult_ItemId(16);
#ifdef CORE_MM
    auto& items = gSaveContext.save.saveInfo.inventory.items;
    items[SLOT_TRADE_DEED] = consumed;
    items[SLOT_TRADE_KEY_MAMA] = kept;
    items[SLOT_TRADE_COUPLE] = ITEM_BOW;
    BUTTON_ITEM_EQUIP(0, 1) = consumed;
    C_SLOT_EQUIP(0, 1) = SLOT_TRADE_DEED;
    BUTTON_ITEM_EQUIP(0, 2) = kept;
    C_SLOT_EQUIP(0, 2) = SLOT_TRADE_DEED;
    TradeAdult_ReconcileConsumed();
    assert(items[SLOT_TRADE_DEED] == ITEM_NONE && items[SLOT_TRADE_KEY_MAMA] == kept);
    assert(items[SLOT_TRADE_COUPLE] == ITEM_BOW);
    assert(BUTTON_ITEM_EQUIP(0, 1) == ITEM_NONE && C_SLOT_EQUIP(0, 1) == SLOT_NONE);
    assert(BUTTON_ITEM_EQUIP(0, 2) == kept && C_SLOT_EQUIP(0, 2) == SLOT_TRADE_DEED);
#else
    auto& items = gSaveContext.inventory.items;
    items[SLOT_TRADE_ADULT] = consumed;
    items[SLOT_TRADE_CHILD] = ITEM_MASK_KEATON;
    gSaveContext.equips.buttonItems[1] = consumed;
    gSaveContext.equips.cButtonSlots[0] = SLOT_TRADE_ADULT;
    gSaveContext.equips.buttonItems[2] = kept;
    gSaveContext.equips.cButtonSlots[1] = SLOT_TRADE_ADULT;
    TradeAdult_ReconcileConsumed();
    assert(items[SLOT_TRADE_ADULT] == ITEM_NONE && items[SLOT_TRADE_CHILD] == ITEM_MASK_KEATON);
    assert(gSaveContext.equips.buttonItems[1] == ITEM_NONE && gSaveContext.equips.cButtonSlots[0] == SLOT_NONE);
    assert(gSaveContext.equips.buttonItems[2] == kept && gSaveContext.equips.cButtonSlots[1] == SLOT_TRADE_ADULT);
#endif
    assert(TradeAdult_IsOwnedIndex(16) && !TradeAdult_IsOwnedIndex(11));
    TradeAdult_FoldCurrent(consumed);
    assert(nei.tradeAdultRevision[11] == 7 && !TradeAdult_IsOwnedIndex(11));
}

#ifdef CORE_MM
static void TestForeignSelectionCleanup(void) {
    // Catches ITEM_NONE matching a foreign trade item, and cursor repair that
    // silently reassigns consumed placeholder bindings to a different item.
    for (int index : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21, 22}) {
        for (int survivor : {-1, 16, 20}) {
            if (survivor == index) continue;
            Reset();
            nei.tradeAdultOwned = (1u << index) | (survivor < 0 ? 0 : 1u << survivor);
            nei.tradeAdultCursor = index;
            for (int slot : {SLOT_TRADE_DEED, SLOT_TRADE_KEY_MAMA, SLOT_TRADE_COUPLE})
                gSaveContext.save.saveInfo.inventory.items[slot] = ITEM_TRADE_PLACEHOLDER;
            for (int button = 1; button <= 3; ++button) {
                BUTTON_ITEM_EQUIP(0, button) = ITEM_TRADE_PLACEHOLDER;
                C_SLOT_EQUIP(0, button) = SLOT_TRADE_DEED;
            }
            for (int button = 0; button < 4; ++button) {
                DPAD_BUTTON_ITEM_EQUIP(0, button) = ITEM_TRADE_PLACEHOLDER;
                DPAD_SLOT_EQUIP(0, button) = SLOT_TRADE_DEED;
            }
            nei.tradeAdultRevision[index] = 1; // incoming consumption
            // Read the cursor FIRST: this must not lose the stale binding identity.
            assert(TradeAdult_CursorIndex() == survivor);
            for (int slot : {SLOT_TRADE_DEED, SLOT_TRADE_KEY_MAMA, SLOT_TRADE_COUPLE})
                assert(gSaveContext.save.saveInfo.inventory.items[slot] == ITEM_NONE);
            for (int button = 1; button <= 3; ++button)
                assert(BUTTON_ITEM_EQUIP(0, button) == ITEM_NONE && C_SLOT_EQUIP(0, button) == SLOT_NONE);
            for (int button = 0; button < 4; ++button)
                assert(DPAD_BUTTON_ITEM_EQUIP(0, button) == ITEM_NONE && DPAD_SLOT_EQUIP(0, button) == SLOT_NONE);
            const u8 expected = survivor < 0 ? ITEM_NONE : survivor == 16 ? ITEM_ROOM_KEY : ITEM_TRADE_PLACEHOLDER;
            assert(TradeAdult_CellItem() == expected);
            assert(ExtInv_GetSlotItem(SLOT_TRADE_DEED) == expected);
            assert(TradeAdult_IndexOfItem(ITEM_NONE) == -1);
        }
    }
    Reset();
    nei.tradeAdultOwned = 1u << 16;
    nei.tradeAdultCursor = 16;
    gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] = ITEM_TRADE_PLACEHOLDER;
    assert(ExtInv_GetSlotItem(SLOT_TRADE_DEED) == ITEM_ROOM_KEY);
}
#endif
#endif

int main(void) {
    TestConsumedHistoryUnavailable();
#ifndef CORE_RED_PROBE
    TestRevisionLifecycle();
    TestSaturatingRevision();
    TestPendantPermanentPerk();
    TestExactNativeCleanup();
    TestIncomingRetirement();
#ifdef CORE_MM
    TestForeignSelectionCleanup();
#endif
#endif
    puts("PASS trade core: native save structures, all 23 revisions, exact native cleanup and permanent Pendant perk");
}
