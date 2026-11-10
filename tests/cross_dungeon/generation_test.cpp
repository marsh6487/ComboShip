#include "combo/rando/CrossDungeonGeneration.h"
#include <cassert>
#include <iostream>
#include <tuple>

using namespace ComboRando;
using namespace ComboRando::CrossDungeon;
using Json = nlohmann::json;
static std::vector<Dungeon> dungeons{{"oot:adult", {0, 1}, {0, 2}, {0, 3}},
                                     {"mm:temple", {1, 4}, {1, 5}, {1, 6}}};
static Json installed[2], accesses[2];
static std::vector<std::string> lastOwned[2];
static Json rejected;
static bool rejectEveryMap = false;
static int hostCalls = 0, queries = 0, runtimeCalls = 0;
static int lastQueryGame = 0;
static ComboGenProgress cancelledProgress;
static bool cancelOnQuery = false;
static bool pocketGraph = false;
static const char *Catalogue(int game) {
  static std::string raw[2];
  ++hostCalls;
  raw[game] = Json::array({DungeonJson(dungeons[game])}).dump();
  return raw[game].c_str();
}
static int Install(int game, const char *raw) {
  ++hostCalls;
  installed[game] = Json::parse(raw);
  return 1;
}
static void Access(int game, const char *raw) {
  accesses[game] = Json::parse(raw);
}
static const char *Exits() {
  return lastQueryGame == 0 ? "[{\"game\":1,\"index\":4}]"
                            : "[{\"game\":0,\"index\":1}]";
}
static void Reset() {
  if (cancelOnQuery)
    cancelledProgress.cancel = true;
}
static void Owned(int game, const char *raw) {
  lastOwned[game] = Json::parse(raw).get<std::vector<std::string>>();
}
static const char *Checks(int game) {
  lastQueryGame = game;
  static std::string raw[2];
  ++queries;
  Json checks = Json::array({"Native check", "Inventory gate"});
  const auto &owned = lastOwned[game];
  if (pocketGraph) {
    const auto has = [&](const char *item) {
      return std::count(owned.begin(), owned.end(), item) > 0;
    };
    const bool stones = has("Kokiri's Emerald") && has("Goron's Ruby") &&
                        has("Zora's Sapphire");
    if (game == 0 && stones) {
      checks.push_back("Song from Ocarina");
      if (has("Song of Time"))
        checks.push_back("Adult song");
      // The native path is reachable as a child after learning the warp song.
      // The swapped map moves its mouth to an adult-only foreign ingress.
      const bool childRoute =
          installed[0].is_null() || installed[0].at("permutation").at(0) == 0;
      if (has("Bolero of Fire") && childRoute) {
        checks.push_back("Well check");
        checks.push_back("Well key 1");
        checks.push_back("Well key 2");
        if (std::count(owned.begin(), owned.end(), "Small Key") >= 2)
          checks.push_back("Well locked check");
      }
    }
    raw[game] = checks.dump();
    return raw[game].c_str();
  }
  const bool song = std::count(owned.begin(), owned.end(), "Saria's Song") > 0;
  const bool fixedSongLock = game == 0 && !installed[game].is_null() &&
                             (rejectEveryMap || installed[game] == rejected);
  if (!fixedSongLock || song)
    checks.push_back("Confined song chest");
  if (song) {
    checks.push_back("Confined key chest 01");
    checks.push_back("Confined key chest 02");
  }
  if (song && std::count(owned.begin(), owned.end(), "Small Key") >= 2)
    checks.push_back("Adult dungeon check");
  raw[game] = checks.dump();
  return raw[game].c_str();
}
static uint8_t Portal() { return 1; }
static OracleFns Oracle(int game) {
  return game == 0 ? OracleFns{Reset, [](const char *raw) { Owned(0, raw); },
                               [] { return Checks(0); }, nullptr, Portal}
                   : OracleFns{Reset, [](const char *raw) { Owned(1, raw); },
                               [] { return Checks(1); }, nullptr};
}
static Json Dump() {
  return {{"checks", Json::array({{{"name", "Native check"}},
                                  {{"name", "Inventory gate"}},
                                  {{"name", "Adult dungeon check"}},
                                  {{"name", "Native unavailable check"}}})},
          {"pool", Json::array({{{"name", "Progressive Hookshot"}},
                                {{"name", "Progressive Hookshot"}}})},
          {"fixed",
           Json::array(
               {{{"check", "Confined song chest"}, {"item", "Saria's Song"}},
                {{"check", "Confined key chest 01"}, {"item", "Small Key"}},
                {{"check", "Confined key chest 02"}, {"item", "Small Key"}}})}};
}
static void CheckCancellation(Coordinator &coordinator, OracleFns &oot,
                              OracleFns &mm, const std::string &dump) {
  const auto previous = coordinator.LogicSeed();
  cancelledProgress.Reset();
  cancelOnQuery = true;
  bool cancelled = false;
  try {
    GenerateValidatedSeed(coordinator, true, 123, dump, dump, oot, mm, false, 4,
                          &cancelledProgress);
  } catch (const std::runtime_error &error) {
    cancelled =
        std::string(error.what()).find("cancelled") != std::string::npos;
  }
  cancelOnQuery = false;
  assert(cancelled && "Reset must cancel during a structural oracle query");
  assert(coordinator.LogicSeed() == previous && installed[0] == previous &&
         installed[1] == previous);
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty());
}

static void CheckForcedPocket(Coordinator &coordinator, OracleFns &oot,
                              OracleFns &mm) {
  pocketGraph = true;
  coordinator.InstallLogic(nullptr);
  coordinator.ConfigureOracles(oot, mm);
  const Json native = {
      {"checks", Json::array({{{"name", "Native check"}},
                              {{"name", "Well check"}},
                              {{"name", "Well locked check"}}})},
      {"pool", Json::array({{{"name", "Goron's Ruby"}}})},
      {"fixed",
       Json::array({{{"check", "Native boss"},
                     {"item", "Zora's Sapphire"},
                     {"allowedChecks", Json::array({"Native boss"})}},
                    {{"check", "Song from Ocarina"}, {"item", "Song of Time"}},
                    {{"check", "Adult song"}, {"item", "Bolero of Fire"}},
                    {{"check", "Well key 1"}, {"item", "Small Key"}},
                    {{"check", "Well key 2"}, {"item", "Small Key"}}})},
      {"items", Json::array({{{"name", "Kokiri's Emerald"}},
                             {{"name", "Goron's Ruby"}},
                             {{"name", "Zora's Sapphire"}},
                             {{"name", "Progressive Hookshot"}},
                             {{"name", "Song of Time"}},
                             {{"name", "Bolero of Fire"}},
                             {{"name", "Small Key"}}})}};
  const Json other = {{"checks", Json::array({{{"name", "Native check"}}})},
                      {"pool", Json::array()},
                      {"fixed", Json::array()}};
  const std::string forced =
      Json({{"Link's Pocket", {{"item", "Kokiri's Emerald"}}}}).dump();
  StructureProbe missingPocket(native.dump(), other.dump(), oot, mm, true);
  assert(!missingPocket.ootRequired.count("Well check"));
  StructureProbe withPocket(native.dump(), other.dump(), oot, mm, true, nullptr,
                            forced);
  assert(withPocket.ootRequired.count("Well check") &&
         withPocket.ootRequired.count("Well locked check") &&
         "the exact forced stone must open the native baseline before cross "
         "routing is judged");
  for (const char *physical : {"Song of Time", "Bolero of Fire", "Small Key"})
    assert(std::count(withPocket.ootOwned.begin(), withPocket.ootOwned.end(),
                      physical) == 0 &&
           "fixed keys and songs must still be collected at their physical "
           "checks");
  assert(std::count(withPocket.ootOwned.begin(), withPocket.ootOwned.end(),
                    "Kokiri's Emerald") == 1);
  const auto valid = Encode(dungeons, {0, 1});
  const auto ageLocked = Encode(dungeons, {1, 0});
  ValidateSavedStructure(coordinator, valid, native.dump(), other.dump(), oot,
                         mm, true, forced);
  bool rejectedAgeLock = false;
  try {
    ValidateSavedStructure(coordinator, ageLocked, native.dump(), other.dump(),
                           oot, mm, true, forced);
  } catch (const std::invalid_argument &error) {
    rejectedAgeLock =
        std::string(error.what()).find("Well") != std::string::npos;
  }
  assert(rejectedAgeLock && coordinator.LogicSeed() == valid);
  uint64_t badFirstSeed = 0;
  while (badFirstSeed < 100 &&
         GenerateSeed(dungeons, badFirstSeed) != ageLocked)
    ++badFirstSeed;
  assert(badFirstSeed < 100);
  const auto generated =
      GenerateValidatedSeed(coordinator, true, badFirstSeed, native.dump(),
                            other.dump(), oot, mm, true, 16, nullptr, forced);
  assert(generated == valid &&
         "generation must reject the adult-only Well route");

  // A forced item reserves an already-assumed source copy, including movable
  // fixed rewards; only genuinely missing native copies increase the count.
  for (const auto &[poolCopies, forcedCopies, expected] :
       {std::tuple{1, 2, 2}, std::tuple{2, 1, 2}, std::tuple{0, 2, 2}}) {
    auto counts = native;
    counts["pool"] = Json::array();
    for (int i = 0; i < poolCopies; ++i)
      counts["pool"].push_back({{"name", "Progressive Hookshot"}});
    Json starts = Json::object();
    for (int i = 0; i < forcedCopies; ++i)
      starts["Start " + std::to_string(i)] = {{"item", "Progressive Hookshot"}};
    coordinator.InstallLogic(nullptr);
    coordinator.ConfigureOracles(oot, mm);
    StructureProbe copies(counts.dump(), other.dump(), oot, mm, true, nullptr,
                          starts.dump());
    assert(std::count(copies.ootOwned.begin(), copies.ootOwned.end(),
                      "Progressive Hookshot") == expected);
  }
  const Json reserved = {{"Start", {{"item", "Zora's Sapphire"}}},
                         {"Category only", {{"category", "advancement"}}}};
  StructureProbe movable(native.dump(), other.dump(), oot, mm, true, nullptr,
                         reserved.dump());
  assert(std::count(movable.ootOwned.begin(), movable.ootOwned.end(),
                    "Zora's Sapphire") == 1);
  assert(movable.ootOwned.size() == 2 &&
         "category-only forces draw from the already-assumed pool");
  bool unknownRejected = false;
  try {
    StructureProbe unknown(
        native.dump(), other.dump(), oot, mm, true, nullptr,
        R"({"Link's Pocket":{"item":"Invented progression"}})");
  } catch (const std::invalid_argument &) {
    unknownRejected = true;
  }
  assert(unknownRejected &&
         "a forced name outside native metadata must not create progression");

  const auto rejects = [](const auto &operation) {
    try {
      operation();
    } catch (const std::invalid_argument &) {
      return true;
    }
    return false;
  };
  const Json starts =
      Json::array({{{"checkGame", "oot"}, {"checkName", "Link's Pocket"}},
                   {{"checkGame", "oot"}, {"checkName", "Link's Pocket"}}});
  const Json recorded = {{"oot",
                          {{"Link's Pocket", "Kokiri's Emerald (OOT)"},
                           {"Song from Ocarina", "Song of Time"}}},
                         {"startKnown", starts}};
  const auto recordedForce = RecordedForcedOot(recorded);
  assert(recordedForce == Json::parse(forced) &&
         "recorded starts resolve item values and duplicate check identities "
         "only once");
  auto legacy = Json{{"oot", {{"placements", recorded.at("oot")}}}};
  assert(RecordedForcedOot(legacy) == recordedForce);
  legacy["startKnown"] = Json::array();
  assert(RecordedForcedOot(legacy).empty() &&
         "explicitly empty recorded starts must not fall back to a fresh or "
         "legacy Pocket");
  auto metadata = recorded;
  metadata["oot"]["Link's Pocket"] = "Display-only item";
  metadata["foreign"] = Json::array({{{"checkGame", "oot"},
                                      {"checkName", "Link's Pocket"},
                                      {"itemGame", "oot"},
                                      {"itemName", "Kokiri's Emerald"}}});
  assert(RecordedForcedOot(metadata) == recordedForce);
  metadata["foreign"][0]["itemGame"] = "mm";
  assert(rejects([&] { RecordedForcedOot(metadata); }) &&
         "a recorded MM item must not become free OoT progression");
  auto missing = recorded;
  missing["oot"].erase("Link's Pocket");
  assert(rejects([&] { RecordedForcedOot(missing); }));
  ValidateRecordedForcedOot(forced, recordedForce, native.dump());
  assert(rejects([&] {
           ValidateRecordedForcedOot(
               R"({"Link's Pocket":{"item":"Goron's Ruby"}})", recordedForce,
               native.dump());
         }) &&
         "replay must not replace a recorded stone with a newly selected "
         "native reward");
  assert(rejects([&] {
    ValidateRecordedForcedOot(forced, Json::object(), native.dump());
  }));
  const std::string category =
      R"({"Link's Pocket":{"category":"advancement"}})";
  const Json poolStart = {{"Link's Pocket", {{"item", "Goron's Ruby"}}}};
  ValidateRecordedForcedOot(category, poolStart, native.dump());
  assert(rejects([&] {
           ValidateRecordedForcedOot(category, recordedForce, native.dump());
         }) &&
         "a category-only replay must reserve a current pool copy, not create "
         "a known but absent item");
  auto junk = native;
  junk["pool"][0]["advancement"] = false;
  assert(rejects(
      [&] { ValidateRecordedForcedOot(category, poolStart, junk.dump()); }));
  ValidateRecordedForcedOot(R"({"Link's Pocket":{"category":"any"}})",
                            poolStart, junk.dump());
  pocketGraph = false;
}

int main() {
  Coordinator coordinator;
  auto oot = Oracle(0), mm = Oracle(1);
  // This must return before reading malformed dumps, touching a host, querying,
  // or rolling maps.
  assert(GenerateValidatedSeed(coordinator, false, 123, "malformed",
                               "malformed", oot, mm, false)
             .is_null());
  assert(hostCalls == 0 && queries == 0);
  coordinator.hosts = {HostFns{[] { return Catalogue(0); },
                               [](const char *raw) { return Install(0, raw); },
                               [](const char *raw) { Access(0, raw); }, Exits,
                               [](const char *) {
                                 ++runtimeCalls;
                                 return 1;
                               }},
                       HostFns{[] { return Catalogue(1); },
                               [](const char *raw) { return Install(1, raw); },
                               [](const char *raw) { Access(1, raw); }, Exits,
                               [](const char *) {
                                 ++runtimeCalls;
                                 return 1;
                               }}};
  const auto dump = Dump().dump();
  {
    auto tagged = Dump();
    tagged["fixed"][0]["allowedChecks"] = Json::array({"Confined song chest", "Native check"});
    StructureProbe movable(tagged.dump(), tagged.dump(), oot, mm, false);
    assert(movable.ootFixed.size() == 2 && movable.mmFixed.size() == 2);
    assert(std::count(movable.ootOwned.begin(), movable.ootOwned.end(), "Saria's Song") == 1);
    assert(std::count(movable.mmOwned.begin(), movable.mmOwned.end(), "Saria's Song") == 1);
  }
  const uint64_t seed = 123;
  rejected = GenerateSeed(dungeons, seed);
  // A blanket grant of fixed items certifies this self-locking song map. The
  // real probe must earn the song from its confined check before using it as a
  // key.
  installed[0] = rejected;
  lastOwned[0] = {"Progressive Hookshot", "Progressive Hookshot",
                  "Saria's Song", "Small Key", "Small Key"};
  const auto pregranted = Json::parse(Checks(0));
  assert(std::find(pregranted.begin(), pregranted.end(),
                   "Adult dungeon check") != pregranted.end());
  installed[0] = nullptr;
  const auto accepted = GenerateValidatedSeed(coordinator, true, seed, dump,
                                              dump, oot, mm, false, 16);
  assert(accepted != rejected && coordinator.LogicSeed() == accepted);
  assert(installed[0] == accepted && installed[1] == accepted &&
         runtimeCalls == 0);
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty() &&
         accesses[0]["overworld"] == true && accesses[1]["overworld"] == true);
  for (const auto &owned : lastOwned) {
    assert(std::count(owned.begin(), owned.end(), "Progressive Hookshot") == 2);
    assert(std::count(owned.begin(), owned.end(), "Small Key") == 2);
    assert(std::count(owned.begin(), owned.end(), "Saria's Song") == 1);
  }
  // Baseline's unavailable named check must not make every candidate
  // impossible.
  const auto replay = GenerateValidatedSeed(coordinator, true, seed, dump, dump,
                                            oot, mm, false, 16);
  assert(replay == accepted); // Retry stream is deterministic and independent
                              // of prior probes.
  const auto callsBeforeSaved = hostCalls;
  bool rejectedSaved = false;
  try {
    ValidateSavedStructure(coordinator, rejected, dump, dump, oot, mm, false);
  } catch (const std::invalid_argument &e) {
    rejectedSaved =
        std::string(e.what()).find("structurally invalid") != std::string::npos;
  }
  assert(rejectedSaved && hostCalls > callsBeforeSaved &&
         coordinator.LogicSeed() == accepted);
  assert(installed[0] == accepted &&
         installed[1] == accepted); // Exact saved map was never rerolled.
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty());
  ValidateSavedStructure(coordinator, accepted, dump, dump, oot, mm, false);
  assert(coordinator.LogicSeed() == accepted);
  assert(accesses[0]["arrivals"].empty() && accesses[1]["arrivals"].empty());
  rejectEveryMap = true;
  bool exhausted = false;
  try {
    GenerateValidatedSeed(coordinator, true, seed, dump, dump, oot, mm, false,
                          4);
  } catch (const StructureExhausted &e) {
    const auto error = std::string(e.what());
    exhausted = error.find("after 4 attempts") != std::string::npos &&
                error.find("Adult dungeon check") != std::string::npos;
  }
  assert(exhausted && coordinator.LogicSeed() == accepted);
  assert(installed[0] == accepted && installed[1] == accepted &&
         runtimeCalls == 0);
  rejectEveryMap = false;
  CheckCancellation(coordinator, oot, mm, dump);
  CheckForcedPocket(coordinator, oot, mm);
  std::cout << "cross-dungeon structural generation, recorded forced starts, "
               "and cancellation: PASS\n";
}
