#define CVarGetInteger FixtureDefaultCVarGetInteger
#define main KeyRingFoundationMain
#include "../key_rings/native_test.cpp"
#undef main
#undef CVarGetInteger
static int sharedWallets;
extern "C" int32_t CVarGetInteger(const char* key,int32_t fallback) {
    return !strcmp(key,"gCombo.Rando.Shared.Wallets") ? sharedWallets : fallback;
}

int main() {
    using namespace Rando;
    RandoItemId child = RI_UNKNOWN;
    RandoOptionId option = RO_MAX;
    for (const auto& [id,row] : StaticData::Items)
        if (!strcmp(row.name,"Child's Wallet")) child = id;
    if (child == RI_UNKNOWN) {
        std::fputs("FAIL missing native MM Child's Wallet\n",stderr);
        return 1;
    }
    for (const auto& [id,row] : StaticData::Options)
        if (!strcmp(row.name,"RO_SHUFFLE_CHILD_WALLET")) option = id;
    REQUIRE(option != RO_MAX && StaticData::Options.at(option).defaultValue == 0);
    const auto& zfgOption = StaticData::Options.at(RO_SHUFFLE_ZFG_WALLET);
    REQUIRE(zfgOption.defaultValue == 0 && !strcmp(zfgOption.cvar, "gRandoSettings.IncludeZfgWallet"));
    auto& region = Logic::Regions[static_cast<RandoRegionId>(0)];
    for (const auto& [id,row] : StaticData::Checks)
        if (id != RC_UNKNOWN) region.checks[id] = {[]{return true;}, "fixture region membership"};
    for (int mode : {COMBO_GAME_BOTH,COMBO_GAME_MM}) {
#ifdef COMBO_BUILD
        gComboGameMode = mode;
#endif
        for (sharedWallets = 0; sharedWallets < 2; ++sharedWallets) {
            for (bool tycoon : {false,true}) {
                for (int enabled : {0,1}) {
                    const bool effective = enabled && !(Combo_IsCombinedGame() && sharedWallets);
                    for (bool zfg : {false, true}) {
                        for (int starts : {0, 1, 2, 3}) {
                            auto save = std::make_unique<RandoSaveInfo>();
                            for (const auto& [id,row] : StaticData::Options) save->randoSaveOptions[id] = row.defaultValue;
                            save->randoSaveOptions[option] = enabled;
                            save->randoSaveOptions[RO_SHUFFLE_TYCOON_WALLET] = tycoon;
                            save->randoSaveOptions[RO_SHUFFLE_ZFG_WALLET] = zfg;
                            std::vector<RandoItemId> starting;
                            if (starts == 1) starting.push_back(RI_PROGRESSIVE_WALLET);
                            if (starts == 2 && effective) starting.push_back(child);
                            if (starts == 3 && zfg) starting.push_back(RI_WALLET_ZFG);
                            SetStartingItemsInSave(*save,starting);
                            std::vector<RandoCheckId> checks;
                            std::vector<RandoItemId> items;
                            Logic::GeneratePools(*save,checks,items);
                            REQUIRE(save->randoSaveOptions[option] == effective);
                            REQUIRE(std::count(items.begin(),items.end(),RI_PROGRESSIVE_WALLET) ==
                                    2 + int(tycoon) + int(effective) + int(zfg) - int(!starting.empty()));
                        }
                    }
                }
            }
        }
    }
    std::puts("PASS native Child, Tycoon and ZFG wallet supply, default-off options and concrete/progressive starts");
}
