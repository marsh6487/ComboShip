// Reuse native save, flag, cycle and pool services; do not duplicate the pool.
#define main KeyRingFoundationMain
#include "../key_rings/native_test.cpp"
#undef main

int main() {
    using namespace Rando;
    const char* names[] = {"Deku Stick Bag", "Deku Nut Bag"};
    const char* optionNames[] = {"RO_SHUFFLE_DEKU_STICK_BAG", "RO_SHUFFLE_DEKU_NUT_BAG"};
    RandoItemId bags[2] = {RI_UNKNOWN, RI_UNKNOWN};
    RandoOptionId options[2] = {RO_MAX, RO_MAX};
    for (int i = 0; i < 2; ++i) {
        for (const auto& [id, row] : StaticData::Items)
            if (!strcmp(row.name, names[i])) bags[i] = id;
        if (bags[i] == RI_UNKNOWN) {
            std::fprintf(stderr, "FAIL missing native bag item: %s\n", names[i]);
            return 1;
        }
        for (const auto& [id, row] : StaticData::Options)
            if (!strcmp(row.name, optionNames[i])) options[i] = id;
        REQUIRE(options[i] != RO_MAX && StaticData::Options.at(options[i]).defaultValue == 0);
        REQUIRE(StaticData::Items.at(bags[i]).randoItemType == RITYPE_MAJOR);
        REQUIRE(StaticData::Items.at(bags[i]).drawId != GID_NONE);
    }
    auto& region = Logic::Regions[static_cast<RandoRegionId>(0)];
    for (const auto& [id, row] : StaticData::Checks)
        if (id != RC_UNKNOWN) region.checks[id] = {[]{return true;}, "fixture region membership"};
    for (int mask = 0; mask < 4; ++mask) {
        for (bool plentiful : {false, true}) {
            for (int starts = 0; starts < 4; ++starts) {
                auto save = std::make_unique<RandoSaveInfo>();
                for (const auto& [id, row] : StaticData::Options) save->randoSaveOptions[id] = row.defaultValue;
                save->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
                std::vector<RandoItemId> starting;
                for (int i = 0; i < 2; ++i) {
                    save->randoSaveOptions[options[i]] = bool(mask & (1 << i));
                    if (starts & (1 << i)) starting.push_back(bags[i]);
                }
                SetStartingItemsInSave(*save, starting);
                std::vector<RandoCheckId> checks;
                std::vector<RandoItemId> items;
                Logic::GeneratePools(*save, checks, items);
                for (int i = 0; i < 2; ++i)
                    REQUIRE(std::count(items.begin(), items.end(), bags[i]) ==
                            ((mask & (1 << i)) && !(starts & (1 << i)) ? 1 : 0));
            }
        }
    }
    std::puts("PASS actual native bag catalogues, default-off pools, density and starting-inventory subtraction");
}
