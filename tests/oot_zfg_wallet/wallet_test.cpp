#include "combo/hud/RupeeCounterDigits.h"
#include "global.h"
#include "soh/Enhancements/randomizer/WalletProgression.h"
#include <algorithm>
#include <cassert>

SaveContext gSaveContext{};
PlayState *gPlayState{};

static bool tycoon;
static bool fullWallets;

struct WalletSettings {
  bool GetRandoSettingValue(RandomizerSettingKey setting) const {
    return setting == RSK_INCLUDE_TYCOON_WALLET ? tycoon : fullWallets;
  }
};
struct OTRGlobals {
  static OTRGlobals *Instance;
  WalletSettings *gRandomizer;
};
WalletSettings settings;
OTRGlobals globals{&settings};
OTRGlobals *OTRGlobals::Instance = &globals;

extern "C" void Flags_SetRandomizerInf(RandomizerInf flag) {
  gSaveContext.ship.randomizerInf[flag >> 4] |= 1u << (flag & 0xF);
}

extern "C" {
#include "wallet_production.inc"
}

static void GiveZfgWallet() {
  switch (RG_ZFG_WALLET) {
#include "zfg_grant.inc"
  default:
    assert(false);
  }
}

static_assert(OotWallet::NextUpgrade(0, false, false, true, false, false,
                                     false) == RG_CHILD_WALLET);
static_assert(OotWallet::NextUpgrade(0, true, false, true, false, false,
                                     false) == RG_ADULT_WALLET);
static_assert(OotWallet::NextUpgrade(1, true, true, true, false, false,
                                     false) == RG_GIANT_WALLET);
static_assert(OotWallet::NextUpgrade(2, true, true, true, false, true, false) ==
              RG_TYCOON_WALLET);
static_assert(OotWallet::NextUpgrade(3, true, true, true, false, true, false) ==
              RG_ZFG_WALLET);
static_assert(OotWallet::NextUpgrade(2, true, false, true, false, true,
                                     false) == RG_ZFG_WALLET);
static_assert(OotWallet::NextUpgrade(2, true, false, true, true, true, false) ==
              RG_WALLET_INF);
static_assert(OotWallet::NextUpgrade(3, true, true, false, false, true,
                                     false) == RG_WALLET_INF);
static_assert(OotWallet::NextUpgrade(3, true, true, true, true, true, true) ==
              RG_BLUE_RUPEE);
static_assert(OotWallet::NextUpgrade(2, true, false, false, false, false,
                                     false) == RG_BLUE_RUPEE);

int main() {
  for (bool includeTycoon : {false, true}) {
    for (bool fill : {false, true}) {
      gSaveContext = {};
      tycoon = includeTycoon;
      fullWallets = fill;
      Inventory_ChangeUpgrade(UPG_BULLET_BAG, 2);
      gSaveContext.rupees = 123;
      assert(CUR_CAPACITY(UPG_WALLET) == 99);
      GiveZfgWallet();
      assert(OotWallet_HasZfg());
      assert(CUR_UPG_VALUE(UPG_WALLET) == (tycoon ? 3 : 2));
      assert(CUR_UPG_VALUE(UPG_BULLET_BAG) == 2);
      assert(CUR_CAPACITY(UPG_WALLET) == 9999);
      assert(gSaveContext.rupees == (fullWallets ? 9999 : 123));
      Rupees_ChangeBy(-100);
      const auto afterPurchase = gSaveContext.rupees;
      GiveZfgWallet();
      assert(gSaveContext.rupees == afterPurchase);
    }
  }
  int16_t digits[RUPEE_COUNTER_MAX_DIGITS];
  assert(RupeeCounter_BuildDigits(9999, 4, digits) == 4);
  for (int digit = 0; digit < 4; ++digit)
    assert(digits[digit] == 9);
  assert(RupeeCounter_BuildDigits(1000, 4, digits) == 4);
  assert(digits[0] == 1 && digits[1] == 0 && digits[2] == 0 && digits[3] == 0);
}
