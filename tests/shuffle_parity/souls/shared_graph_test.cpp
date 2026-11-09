#include "rando/CrossForeign.h"
#include <cassert>
using namespace ComboRando;
using json = nlohmann::json;
int main() {
  assert(SF_COUNT == 17);
  assert(SharedMaskFromKeys(SharedKeysFromMask(SS_ALL)) == SS_ALL);
  assert((SharedMaskAll() & SS_ALL) == SS_ALL);
  assert(SharedMaskFromKeys(json::array({nullptr, "unknown", 1})) == 0);
  auto all = BuildSharedItemGroups({}, SS_ALL);
  assert(all.size() == 66);
  json o = {{"soulOptions",
             {{"enemy", true},
              {"npc", true},
              {"animal", true},
              {"misc", true}}}},
       m = o;
  assert(EffectiveSharedSoulMask(SS_ALL, o, m) == SS_ALL);
  assert(EffectiveSharedSoulMask(SS_ALL, o, json::object()) == 0);
  for (const auto &c : SharedSoulCategories) {
    auto bad = m;
    bad["soulOptions"][c.option] = false;
    assert(EffectiveSharedSoulMask(SS_ALL, o, bad) == (SS_ALL & ~c.bit));
    bad["soulOptions"][c.option] = 1;
    assert(EffectiveSharedSoulMask(c.bit, o, bad) == 0);
    assert(SharedMaskFromKeys(json::array({c.key})) == c.bit);
  }
  for (const auto &p : SharedSoulPairs) {
    assert(IsSharedItem(GAME_OOT, p.ootName, p.bit, {}));
    assert(IsSharedItem(GAME_MM, p.mmName, p.bit, {}));
    assert(!IsSharedItem(GAME_OOT, p.ootName, 0, {}));
    auto groups = BuildSharedItemGroups({}, p.bit);
    std::vector<std::string> ov, mv;
    SharedOwnedViews(groups, {p.ootName}, {}, ov, mv);
    assert(std::count(mv.begin(), mv.end(), p.mmName) == 1);
    SharedOwnedViews(groups, {}, {p.mmName}, ov, mv);
    assert(std::count(ov.begin(), ov.end(), p.ootName) == 1);
  }
  auto groups = BuildSharedItemGroups(
      {{"Soul of Wallmasters", "Soul of Wallmasters"}}, SS_ENEMY);
  std::vector<std::string> ov, mv;
  SharedOwnedViews(groups,
                   {"Soul of Wallmasters", "Wallmaster Enemy Soul",
                    "Floormaster Enemy Soul"},
                   {}, ov, mv);
  assert(std::count(mv.begin(), mv.end(), "Soul of Wallmasters") == 1);
  assert(std::count(ov.begin(), ov.end(), "Wallmaster Enemy Soul") == 1);
  json startO = {{"startingSouls", json::array({"Wallmaster Enemy Soul"})}};
  groups = BuildSharedItemGroups({}, SS_ENEMY, startO.dump(), "{}");
  SharedOwnedViews(groups, {}, {}, ov, mv);
  assert(std::count(ov.begin(), ov.end(), "Floormaster Enemy Soul") == 1);
  assert(std::count(mv.begin(), mv.end(), "Soul of Wallmasters") == 1);
  groups = BuildSharedItemGroups({}, SS_ENEMY, "{}",
                                 R"({"startingSouls":["unknown",1]})");
  SharedOwnedViews(groups, {}, {}, ov, mv);
  assert(ov.empty() && mv.empty());
  groups = BuildSharedItemGroups({}, 0, startO.dump(), "{}");
  SharedOwnedViews(groups, {}, {}, ov, mv);
  assert(ov.empty() && mv.empty());
  puts("PASS shared category keys, effective native options, merged/legacy "
       "groups and absolute soul credit");
}
