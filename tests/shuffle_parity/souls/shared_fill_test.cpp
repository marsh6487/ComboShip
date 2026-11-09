// Reuse the existing native fill fixture's boundary oracles and dump
// construction.
#define main ExistingSharedRegressionMain
#include "../../../combo/tests/combo_shared_test.cpp"
#undef main
int main() {
  for (const auto &category : SharedSoulCategories)
    for (GameId start : {GAME_OOT, GAME_MM}) {
      json o = emptyDump(), m = emptyDump();
      o["soulOptions"] = {{category.option, true}};
      m["soulOptions"] = o["soulOptions"];
      std::set<std::string> names[2];
      for (const auto &p : SharedSoulPairs)
        if (p.bit == category.bit) {
          names[0].insert(p.ootName);
          names[1].insert(p.mmName);
        }
      for (const auto &n : names[0])
        add(o, n, 1);
      for (const auto &n : names[1])
        add(m, n, 1);
      add(o, "Junk", 1, false);
      add(m, "Junk", 1, false);
      configure(o, m);
      auto result = CrossWorldCombinedFill(
          o.dump(), m.dump(), 20261108, oot, mm, nullptr, "",
          OotAccess::ALL_REACHABLE, {}, start, "", category.bit);
      check(result.success,
            "native combined fill succeeds for shared soul category");
      if (result.success) {
        size_t count = 0;
        for (const auto &p : ParseSpoilerPlacements(result.spoilerJson))
          count += names[p.itemGame].contains(p.item);
        check(count == BuildSharedItemGroups({}, category.bit).size(),
              "one native advancement per connected soul group");
      }
      m["soulOptions"][category.option] = false;
      configure(o, m);
      result = CrossWorldCombinedFill(o.dump(), m.dump(), 20261108, oot, mm,
                                      nullptr, "", OotAccess::ALL_REACHABLE, {},
                                      start, "", category.bit);
      check(result.success, "disabled peer family fill succeeds");
      if (result.success) {
        size_t count = 0;
        for (const auto &p : ParseSpoilerPlacements(result.spoilerJson))
          count += names[p.itemGame].contains(p.item);
        check(count == names[0].size() + names[1].size(),
              "disabled peer does not dedupe native soul sources");
      }
    }
  // Native configured starting souls are removed from that game's pool, but
  // must still credit the shared peer and remove the remaining duplicate.
  for (const auto &category : SharedSoulCategories)
    for (GameId starter : {GAME_OOT, GAME_MM}) {
      const auto *pair =
          std::find_if(std::begin(SharedSoulPairs), std::end(SharedSoulPairs),
                       [&](const auto &p) { return p.bit == category.bit; });
      json o = emptyDump(), m = emptyDump();
      o["soulOptions"] = {{category.option, true}};
      m["soulOptions"] = o["soulOptions"];
      add(o, pair->ootName, 1);
      add(m, pair->mmName, 1);
      add(o, "Junk", 1, false);
      add(m, "Junk", 1, false);
      auto &starting = starter == GAME_OOT ? o : m;
      starting["startingSouls"] =
          json::array({starter == GAME_OOT ? pair->ootName : pair->mmName});
      starting["pool"].erase(starting["pool"].begin());
      configure(o, m, pair->ootName, 1, pair->mmName, 1);
      auto result = CrossWorldCombinedFill(
          o.dump(), m.dump(), 20261109, oot, mm, nullptr, "",
          OotAccess::ALL_REACHABLE, {}, GAME_OOT, "", category.bit);
      check(result.success,
            "native starting soul satisfies shared oracle gates");
      if (result.success) {
        check(countPlaced(result, GAME_OOT, pair->ootName) +
                      countPlaced(result, GAME_MM, pair->mmName) ==
                  0,
              "starting shared component has no redundant findable copy");
        configure(o, m, pair->ootName, 1, pair->mmName, 1);
        auto replay = RunPlaythrough(
            result.spoilerJson, oot, mm, "starting_souls", nullptr, nullptr,
            o.dump(), m.dump(), true, false, {}, false, category.bit);
        check(replay.beatable && replay.beatableSphere == 0,
              "shared starting soul wins replay at sphere0");
        o["pool"].push_back({{"name", "Unrelated advancement"},
                             {"advancement", true},
                             {"category", "major"}});
        json candidate = {{"oot", {{"Check 0", "Unrelated advancement"}}},
                          {"mm", json::object()},
                          {"foreign", json::array()},
                          {"sharedItems", json::array({category.key})}};
        configure(o, m, pair->ootName, 1, pair->mmName, 1);
        auto requiredness = PareDownPlaythrough(
            candidate.dump(), oot, mm, nullptr, o.dump(), m.dump(), {}, {},
            DefaultGanonMajoraGoal, true, false, nullptr, category.bit);
        check(requiredness.requiredByCheck.contains("oot:Check 0") &&
                  !requiredness.requiredByCheck.at("oot:Check 0"),
              "requiredness classifies unrelated advancement using shared "
              "starting baseline");
      }
    }
  return failures ? 1 : 0;
}
