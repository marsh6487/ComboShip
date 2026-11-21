#include "Rando/Logic/Logic.h"
#include "Rando/Wallets.h"
#include "tests/test_require.h"
#include <cstdio>

extern "C" {
#include "variables.h"
#include "functions.h"
SaveContext gSaveContext{};
PlayState* gPlayState;
int NativeWalletFourDigits(void);
}
#include "wallet_flush.inc"
namespace Rando::Logic { uint64_t gCurrentRegionTime; }

int main() {
    using namespace Rando::Logic;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET] = 1;
    RANDO_SAVE_CHECKS[RC_UNKNOWN].price = 10;
    REQUIRE(!CAN_AFFORD(RC_UNKNOWN));
    REQUIRE(CUR_CAPACITY(UPG_WALLET) == 0);
    RANDO_SAVE_CHECKS[RC_UNKNOWN].price = 0;
    REQUIRE(CAN_AFFORD(RC_UNKNOWN));
    Rando::Logic::RandoRegion feeChecks{};
    feeChecks.checks = {
        CHECK(RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_HIGH_SCORE, true),
        CHECK(RC_CLOCK_TOWN_EAST_POSTMAN_HAT, true),
    };
    REQUIRE(!feeChecks.checks.at(RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_HIGH_SCORE).first());
    REQUIRE(feeChecks.checks.at(RC_CLOCK_TOWN_EAST_POSTMAN_HAT).first());
#define CanKillEnemy(actor) true
#include "wallet_fee_rules.inc"
#undef CanKillEnemy
    gSaveContext.save.saveInfo.playerData.healthCapacity = 4 * 16;
    gSaveContext.save.playerForm = PLAYER_FORM_DEKU;
    gCurrentRegionTime = uint64_t(1) << TIME_DAY1_AM_06_00;
    REQUIRE(!feeChecks.checks.at(RC_ENEMY_DROP_POE_SISTER).first());
    REQUIRE(!playgroundEvents.at(RE_DEKU_PLAYGROUND_1)());
    Rupees_ChangeBy(50);
    REQUIRE(gSaveContext.rupeeAccumulator == 0);
    gSaveContext.save.saveInfo.playerData.rupees = 99;
    gSaveContext.rupeeAccumulator = 200;
    Combo_MM_FlushAccumulators();
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 0);
    REQUIRE(gSaveContext.rupeeAccumulator == 0);
    REQUIRE(Rando::Wallets::Give(RI_WALLET_CHILD));
    REQUIRE(Flags_GetRandoInf(RANDO_INF_WALLET_CHILD));
    Combo_MM_FlushAccumulators();
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 99);
    gSaveContext.save.saveInfo.playerData.rupees = 1;
    REQUIRE(Rando::Wallets::Give(RI_WALLET_CHILD));
    REQUIRE(gSaveContext.rupeeAccumulator == 0);
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 1);
    REQUIRE(feeChecks.checks.at(RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_HIGH_SCORE).first());
    REQUIRE(feeChecks.checks.at(RC_ENEMY_DROP_POE_SISTER).first());
    REQUIRE(playgroundEvents.at(RE_DEKU_PLAYGROUND_1)());
    REQUIRE(!playgroundEvents.at(RE_DEKU_PLAYGROUND_2)());
    RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE] = 1;
    REQUIRE(!playgroundEvents.at(RE_DEKU_PLAYGROUND_1)());
    Flags_SetRandoInf(RANDO_INF_OBTAINED_CLOCK_DAY_1);
    REQUIRE(playgroundEvents.at(RE_DEKU_PLAYGROUND_1)());
    RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE] = 0;
    gCurrentRegionTime = uint64_t(1) << TIME_DAY2_AM_06_00;
    REQUIRE(!playgroundEvents.at(RE_DEKU_PLAYGROUND_1)());
    REQUIRE(playgroundEvents.at(RE_DEKU_PLAYGROUND_2)());
    REQUIRE(CUR_CAPACITY(UPG_WALLET) == 99);
    for (int price : {0,1,99,100,200,201,500,501}) {
        RANDO_SAVE_CHECKS[RC_UNKNOWN].price = price;
        REQUIRE(bool(CAN_AFFORD(RC_UNKNOWN)) == (price <= 99));
    }
    Rupees_ChangeBy(250);
    Combo_MM_FlushAccumulators();
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 99);
    REQUIRE(Rando::Wallets::Remove(RI_WALLET_CHILD));
    REQUIRE(!Flags_GetRandoInf(RANDO_INF_WALLET_CHILD));
    REQUIRE(gSaveContext.save.saveInfo.playerData.rupees == 0);
    Inventory_ChangeUpgrade(UPG_WALLET, 1);
    REQUIRE(CUR_CAPACITY(UPG_WALLET) == 200);
    RANDO_SAVE_CHECKS[RC_UNKNOWN].price = 200;
    REQUIRE(CAN_AFFORD(RC_UNKNOWN));
    RANDO_SAVE_CHECKS[RC_UNKNOWN].price = 201;
    REQUIRE(!CAN_AFFORD(RC_UNKNOWN));
    Inventory_ChangeUpgrade(UPG_WALLET, 0);
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET] = 0;
    REQUIRE(CUR_CAPACITY(UPG_WALLET) == 99);
    std::puts("PASS native missing/child/adult wallet affordability, rupee grants and dormant save flush");

    for (bool tycoon : {false, true}) {
        gSaveContext = {};
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET] = 1;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] = tycoon;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_ZFG_WALLET] = 1;
        auto& balance = gSaveContext.save.saveInfo.playerData.rupees;
        auto& upgrades = gSaveContext.save.saveInfo.inventory.upgrades;
        upgrades = 1u << gUpgradeShifts[UPG_BULLET_BAG];
        REQUIRE(Rando::Wallets::Give(RI_WALLET_ZFG));
        REQUIRE(RandoCapacity_HasZfgWallet());
        REQUIRE(CUR_UPG_VALUE(UPG_BULLET_BAG) == 1);
        REQUIRE(CUR_UPG_VALUE(UPG_WALLET) == (tycoon ? 3 : 2));
        REQUIRE(CUR_CAPACITY(UPG_WALLET) == 9999);
        REQUIRE(NativeWalletFourDigits());
        Combo_MM_FlushAccumulators();
        REQUIRE(balance == 9999);
        Rupees_ChangeBy(-125);
        Combo_MM_FlushAccumulators();
        REQUIRE(balance == 9874);
        REQUIRE(Rando::Wallets::Give(RI_WALLET_ZFG));
        REQUIRE(gSaveContext.rupeeAccumulator == 0);
        REQUIRE(Rando::Wallets::NextProgressiveItem() == RI_JUNK);
        REQUIRE(Rando::Wallets::Remove(RI_WALLET_ZFG));
        REQUIRE(!RandoCapacity_HasZfgWallet());
        REQUIRE(balance == CUR_CAPACITY(UPG_WALLET));
        REQUIRE(Rando::Wallets::NextProgressiveItem() == RI_WALLET_ZFG);
    }
    std::puts("PASS ZFG wallet direct grant, native-tier floor, four-digit HUD, spend, deduplication and removal");
}
