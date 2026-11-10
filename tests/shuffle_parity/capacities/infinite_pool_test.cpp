#define main KeyRingFoundationMain
#include "../key_rings/native_test.cpp"
#undef main

int main() {
  using namespace Rando;
  const char *names[] = {"Infinite Quiver",      "Infinite Bomb Bag",
                         "Infinite Deku Sticks", "Infinite Deku Nuts",
                         "Infinite Magic",       "Infinite Wallet"};
  RandoItemId upgrades[6];
  for (int i = 0; i < 6; ++i) {
    upgrades[i] = RI_UNKNOWN;
    for (const auto &[id, row] : StaticData::Items)
      if (!strcmp(row.name, names[i]))
        upgrades[i] = id;
    if (upgrades[i] == RI_UNKNOWN) {
      std::fprintf(stderr, "FAIL missing native infinite upgrade: %s\n",
                   names[i]);
      return 1;
    }
    REQUIRE(StaticData::Items.at(upgrades[i]).randoItemType == RITYPE_MAJOR);
    REQUIRE(StaticData::Items.at(upgrades[i]).drawId != GID_NONE);
  }
  RandoOptionId option = RO_MAX;
  for (const auto &[id, row] : StaticData::Options)
    if (!strcmp(row.name, "RO_INFINITE_UPGRADES"))
      option = id;
  REQUIRE(option != RO_MAX && StaticData::Options.at(option).defaultValue == 0);
  auto &region = Logic::Regions[static_cast<RandoRegionId>(0)];
  for (const auto &[id, row] : StaticData::Checks)
    if (id != RC_UNKNOWN)
      region.checks[id] = {[] { return true; }, "fixture region membership"};
  for (int enabled : {0, 1}) {
    for (bool plentiful : {false, true}) {
      for (int starts = 0; starts < 64; ++starts) {
        auto save = std::make_unique<RandoSaveInfo>();
        for (const auto &[id, row] : StaticData::Options)
          save->randoSaveOptions[id] = row.defaultValue;
        save->randoSaveOptions[option] = enabled;
        save->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
        std::vector<RandoItemId> starting;
        for (int i = 0; i < 6; ++i)
          if (starts & (1 << i))
            starting.push_back(upgrades[i]);
        SetStartingItemsInSave(*save, starting);
        std::vector<RandoCheckId> checks;
        std::vector<RandoItemId> items;
        Logic::GeneratePools(*save, checks, items);
        for (int i = 0; i < 6; ++i)
          REQUIRE(std::count(items.begin(), items.end(), upgrades[i]) ==
                  (enabled && !(starts & (1 << i)) ? 1 : 0));
      }
    }
  }
  std::puts("PASS native six-family infinite pools, defaults, uniqueness and "
            "all starting combinations");
}
