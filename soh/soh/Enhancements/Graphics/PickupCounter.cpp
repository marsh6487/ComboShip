#include "hud/PickupCounter.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
extern "C" {
#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
}
#ifdef COMBO_BUILD
bool Combo_OotIsForeground(void);
#endif

namespace PickupCounterHost {
using namespace PickupCounter;
constexpr const char* kWindowCvar = "gPickupCounterOotWindow";
constexpr const char* kWindowName = "OoT Pickup Counter";
ImFont* Font() {
    return OTRGlobals::Instance ? OTRGlobals::Instance->fontStandardLargest : nullptr;
}

bool IsForeground() {
#ifdef COMBO_BUILD
    return Combo_OotIsForeground();
#else
    return true;
#endif
}
bool IsMirrored() {
    return CVarGetInteger("gEnhancements.MirroredWorld", 0) != 0;
}

Snapshot ReadSnapshot() {
    Snapshot result{};
    const auto put = [&](Kind kind, int current, int capacity, RandomizerInf infinite) {
        result[Index(kind)] = { current, capacity, IS_RANDO && Flags_GetRandomizerInf(infinite) != 0 };
    };
    const int wallet = !IS_RANDO || Flags_GetRandomizerInf(RAND_INF_HAS_WALLET) ? CUR_CAPACITY(UPG_WALLET) : 0;
    // Include the pending accumulator exactly once, instead of interpreting
    // every native count-up tick as a fresh rupee pickup.
    put(Kind::Rupees, gSaveContext.rupees + gSaveContext.rupeeAccumulator, wallet, RAND_INF_HAS_INFINITE_MONEY);
    put(Kind::Arrows, AMMO(ITEM_BOW), CUR_CAPACITY(UPG_QUIVER), RAND_INF_HAS_INFINITE_QUIVER);
    put(Kind::Bombs, AMMO(ITEM_BOMB), CUR_CAPACITY(UPG_BOMB_BAG), RAND_INF_HAS_INFINITE_BOMB_BAG);
    put(Kind::Seeds, AMMO(ITEM_SLINGSHOT), CUR_CAPACITY(UPG_BULLET_BAG), RAND_INF_HAS_INFINITE_BULLET_BAG);
    put(Kind::Sticks, AMMO(ITEM_STICK), CUR_CAPACITY(UPG_STICKS), RAND_INF_HAS_INFINITE_STICK_UPGRADE);
    put(Kind::Nuts, AMMO(ITEM_NUT), CUR_CAPACITY(UPG_NUTS), RAND_INF_HAS_INFINITE_NUT_UPGRADE);
    int chuCapacity = INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU ? 50 : 0;
    if (IS_RANDO && Randomizer_GetSettingValue(RSK_BOMBCHU_BAG) == RO_BOMBCHU_BAG_PROGRESSIVE) {
        constexpr int capacities[] = { 0, 20, 30, 50 };
        chuCapacity = capacities[std::min<int>(3, gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel)];
    }
    put(Kind::Bombchus, AMMO(ITEM_BOMBCHU), chuCapacity, RAND_INF_HAS_INFINITE_BOMBCHUS);
    result[Index(Kind::Beans)] = { AMMO(ITEM_BEAN), INV_CONTENT(ITEM_BEAN) == ITEM_BEAN ? 10 : 0, false };
    return result;
}

Kind ReceiptKind(int item) {
    switch (item) {
        case ITEM_RUPEE_GREEN:
        case ITEM_RUPEE_BLUE:
        case ITEM_RUPEE_RED:
        case ITEM_RUPEE_PURPLE:
        case ITEM_RUPEE_GOLD:
            return Kind::Rupees;
        case ITEM_BOW:
        case ITEM_ARROWS_SMALL:
        case ITEM_ARROWS_MEDIUM:
        case ITEM_ARROWS_LARGE:
            return Kind::Arrows;
        case ITEM_BOMB:
        case ITEM_BOMBS_5:
        case ITEM_BOMBS_10:
        case ITEM_BOMBS_20:
        case ITEM_BOMBS_30:
            return Kind::Bombs;
        case ITEM_BOMBCHU:
        case ITEM_BOMBCHUS_5:
        case ITEM_BOMBCHUS_20:
            return Kind::Bombchus;
        case ITEM_SLINGSHOT:
        case ITEM_SEEDS:
        case ITEM_SEEDS_30:
            return Kind::Seeds;
        case ITEM_STICK:
        case ITEM_STICKS_5:
        case ITEM_STICKS_10:
            return Kind::Sticks;
        case ITEM_NUT:
        case ITEM_NUTS_5:
        case ITEM_NUTS_10:
            return Kind::Nuts;
        case ITEM_BEAN:
            return Kind::Beans;
        default:
            return Kind::Count;
    }
}
int IconItem(Kind kind) {
    switch (kind) {
        case Kind::Bombs:
            return ITEM_BOMB;
        case Kind::Bombchus:
            return ITEM_BOMBCHU;
        case Kind::Seeds:
            return ITEM_SEEDS;
        case Kind::Sticks:
            return ITEM_STICK;
        case Kind::Nuts:
            return ITEM_NUT;
        case Kind::Beans:
            return ITEM_BEAN;
        default:
            return -1; // Authored gem and arrow glyphs.
    }
}
} // namespace PickupCounterHost

#include "hud/PickupCounterWindow.inc"

namespace {
void RegisterPickupCounter() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>([] { EnsurePickupCounterWindow(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemReceive>(
        [](GetItemEntry entry) { NotePickup(PickupCounterHost::ReceiptKind(entry.itemId)); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { ResetPickupCounters(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) { ResetPickupCounters(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ResetPickupCounters);
}
static RegisterShipInitFunc pickupCounterInit(RegisterPickupCounter, {});
} // namespace
