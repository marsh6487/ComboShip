#include "hud/PickupCounter.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/BenPort.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
}
#ifdef COMBO_BUILD
bool Combo_MmIsForeground(void);
#endif

namespace PickupCounterHost {
using namespace PickupCounter;
constexpr const char* kWindowCvar = "gPickupCounterMmWindow";
constexpr const char* kWindowName = "MM Pickup Counter";
ImFont* Font() {
    return OTRGlobals::Instance ? OTRGlobals::Instance->fontStandardLargest : nullptr;
}

bool IsForeground() {
#ifdef COMBO_BUILD
    return Combo_MmIsForeground();
#else
    return true;
#endif
}
bool IsMirrored() {
    return CVarGetInteger("gModes.MirroredWorld.State", 0) != 0;
}
Snapshot ReadSnapshot() {
    Snapshot result{};
    result[Index(Kind::Rupees)] = { gSaveContext.save.saveInfo.playerData.rupees + gSaveContext.rupeeAccumulator,
                                    CUR_CAPACITY(UPG_WALLET), RandoCapacity_InfiniteWallet() != 0 };
    const auto put = [&](Kind kind, int item, int capacity) {
        result[Index(kind)] = { AMMO(item), capacity, RandoCapacity_InfiniteAmmo(item) != 0 };
    };
    put(Kind::Arrows, ITEM_BOW, CUR_CAPACITY(UPG_QUIVER));
    put(Kind::Bombs, ITEM_BOMB, CUR_CAPACITY(UPG_BOMB_BAG));
    put(Kind::Bombchus, ITEM_BOMBCHU, RandoCapacity_BombchuCapacity());
    put(Kind::Sticks, ITEM_DEKU_STICK, CUR_CAPACITY(UPG_DEKU_STICKS));
    put(Kind::Nuts, ITEM_DEKU_NUT, CUR_CAPACITY(UPG_DEKU_NUTS));
    put(Kind::Beans, ITEM_MAGIC_BEANS, INV_CONTENT(ITEM_MAGIC_BEANS) == ITEM_MAGIC_BEANS ? 20 : 0);
    put(Kind::PowderKeg, ITEM_POWDER_KEG, INV_CONTENT(ITEM_POWDER_KEG) == ITEM_POWDER_KEG ? 1 : 0);
    return result;
}
Kind ReceiptKind(int item) {
    switch (item) {
        case ITEM_RUPEE_GREEN:
        case ITEM_RUPEE_BLUE:
        case ITEM_RUPEE_10:
        case ITEM_RUPEE_RED:
        case ITEM_RUPEE_PURPLE:
        case ITEM_RUPEE_SILVER:
        case ITEM_RUPEE_HUGE:
            return Kind::Rupees;
        case ITEM_BOW:
        case ITEM_ARROWS_10:
        case ITEM_ARROWS_30:
        case ITEM_ARROWS_40:
        case ITEM_ARROWS_50:
            return Kind::Arrows;
        case ITEM_BOMB:
        case ITEM_BOMBS_5:
        case ITEM_BOMBS_10:
        case ITEM_BOMBS_20:
        case ITEM_BOMBS_30:
            return Kind::Bombs;
        case ITEM_BOMBCHU:
        case ITEM_BOMBCHUS_1:
        case ITEM_BOMBCHUS_5:
        case ITEM_BOMBCHUS_10:
        case ITEM_BOMBCHUS_20:
            return Kind::Bombchus;
        case ITEM_DEKU_STICK:
        case ITEM_DEKU_STICKS_5:
        case ITEM_DEKU_STICKS_10:
            return Kind::Sticks;
        case ITEM_DEKU_NUT:
        case ITEM_DEKU_NUTS_5:
        case ITEM_DEKU_NUTS_10:
            return Kind::Nuts;
        case ITEM_MAGIC_BEANS:
            return Kind::Beans;
        case ITEM_POWDER_KEG:
            return Kind::PowderKeg;
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
        case Kind::Sticks:
            return ITEM_DEKU_STICK;
        case Kind::Nuts:
            return ITEM_DEKU_NUT;
        case Kind::Beans:
            return ITEM_MAGIC_BEANS;
        case Kind::PowderKeg:
            return ITEM_POWDER_KEG;
        default:
            return -1;
    }
}
} // namespace PickupCounterHost

#include "hud/PickupCounterWindow.inc"

void DrawGibdoRequests();

namespace {
void RegisterPickupCounter() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnInterfaceDrawStart>(
        [] { EnsurePickupCounterWindow(DrawGibdoRequests); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemGive>(
        [](u8 item) { NotePickup(PickupCounterHost::ReceiptKind(item)); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s8, s8) { ResetPickupCounters(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16) { ResetPickupCounters(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ResetPickupCounters);
}
static RegisterShipInitFunc pickupCounterInit(RegisterPickupCounter, {});
} // namespace
