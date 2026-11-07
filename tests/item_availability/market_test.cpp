#include "rando/BlackMarket.h"
#include <cstdlib>
#include <iostream>

using namespace ComboRando::BlackMarket;
using nlohmann::json;

static void Check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

// Hand-authored old-format payload, independent of the new generator. Both
// wildcards are valid in all three modes; slots retain their original prices.
static json FrozenMarket(Games games, bool trirod = false) {
  json stock = {
      {{"key", "nei.shovel"},
       {"identity", "RI_OOT_NEI_SHOVEL"},
       {"tier", 1},
       {"price", 50},
       {"clears", 0}},
      {{"key", "nei.lantern"},
       {"identity", "RI_OOT_NEI_LANTERN"},
       {"tier", 1},
       {"price", 60},
       {"clears", 0}},
      {{"key", "wildcard.magic_upgrade"},
       {"identity", "RI_DOUBLE_MAGIC"},
       {"tier", 2},
       {"price", 150},
       {"clears", 1}},
      {{"key", "nei.wand.sand"},
       {"identity", "RI_OOT_NEI_WAND_SAND_ROD"},
       {"tier", 2},
       {"price", 130},
       {"clears", 1}},
      {{"key", "nei.cane.blocks"},
       {"identity", "RI_OOT_NEI_CANE_SOMARIA_BLOCK"},
       {"tier", 2},
       {"price", 130},
       {"clears", 1}},
      {{"key", "nei.cane.flip"},
       {"identity", "RI_OOT_NEI_CANE_PACCI_FLIP"},
       {"tier", 2},
       {"price", 100},
       {"clears", 1}},
      {{"key", "wildcard.double_defense"},
       {"identity", "RI_DOUBLE_DEFENSE"},
       {"tier", 3},
       {"price", 180},
       {"clears", 2}},
      {{"key", "nei.trident"},
       {"identity", "RI_OOT_EXT_TRIDENT"},
       {"tier", 3},
       {"price", 200},
       {"clears", 2}},
  };
  if (trirod)
    stock[5] = {{"key", "nei.cane.trirod"},
                {"identity", "RI_OOT_NEI_CANE_SOMARIA_PLATFORM"},
                {"tier", 2},
                {"price", 150},
                {"clears", 1}};
  json market = {
      {"version", 1},        {"catalogVersion", 1},    {"seed", 123},
      {"pricing", "tiered"}, {"dungeonUnlocks", true}, {"stock", stock}};
  if (games != Games::Both)
    market["games"] = games == Games::Oot ? "oot" : "mm";
  return market;
}

static bool IsTrirod(const Item &item) {
  return std::string_view(item.key) == "nei.cane.trirod";
}

static void CheckInventory(const json &market, bool excludesTrirod) {
  Check(Validate(market),
        "generated schema, prices and clear gates remain valid");
  Check(market.at("stock").size() == 8, "new inventory retains eight offers");
  std::array<int, 3> tiers{};
  int wildcards = 0;
  std::set<std::string> keys;
  for (const auto &offer : market.at("stock")) {
    const auto key = offer.at("key").get<std::string>();
    Check(!excludesTrirod || key != "nei.cane.trirod",
          "MM-only generated stock must not roll unfinished Trirod");
    Check(keys.insert(key).second, "new inventory offers remain unique");
    ++tiers.at(offer.at("tier").get<int>() - 1);
    wildcards += key.starts_with("wildcard.");
  }
  Check(wildcards == 2, "new inventory retains two wildcards");
  Check(tiers == std::array<int, 3>{2, 4, 2},
        "new inventory retains tier quotas");
}

static void Generation() {
  for (Games games : {Games::Mm, Games::Oot, Games::Both}) {
    Options options;
    options.games = games;
    bool seenTrirod = false;
    for (uint32_t seed = 0; seed < 2048; ++seed) {
      const auto market = Generate(seed, options);
      CheckInventory(market, games == Games::Mm);
      Check(Generate(seed, options) == market,
            "stock generation remains deterministic");
      for (const auto &offer : market.at("stock"))
        seenTrirod |= offer.at("key") == "nei.cane.trirod";
    }
    Check(seenTrirod == (games != Games::Mm),
          "Trirod still rolls for OoT and paired Combo");
  }
  // Follow-SoH pricing uses its independent stream and keeps its stored gates.
  Options prices;
  prices.games = Games::Mm;
  prices.pricing = Pricing::FollowSoH;
  prices.soh.mode = PriceMode::Fixed;
  prices.soh.fixedPrice = 77;
  prices.dungeonUnlocks = false;
  const auto market = Generate(13, prices);
  CheckInventory(market, true);
  for (const auto &offer : market.at("stock"))
    Check(offer.at("price") == 77 && offer.at("clears") == 0,
          "new admission preserves pricing and unlock options");
  std::cout << "PASS generation: 6144 inventories, deterministic stock, mode "
               "eligibility, quotas and prices\n";
}

static Session Open(const json &market) {
  Session session;
  Check(session.Bind(market, 1), "fixture binds as an existing market");
  return session;
}

static void Replacement() {
  auto market = FrozenMarket(Games::Mm);
  auto owned = [](const Item &item) {
    return std::string_view(item.key) == "nei.cane.flip";
  };
  auto usable = [](const Item &) { return true; };
  for (uint32_t seed = 0; seed < 2048; ++seed) {
    market["seed"] = seed;
    auto first = Open(market), second = Open(market);
    first.state.purchased = second.state.purchased = 1; // unrelated paid shelf
    Check(first.RefreshOwned(owned, usable),
          "owned offer obtains an available replacement");
    Check(second.RefreshOwned(owned, usable),
          "same owned offer replaces deterministically");
    Check(!IsTrirod(*first.Offer(5)),
          "MM-only replacements must not roll unfinished Trirod");
    Check(StateToJson(first.state) == StateToJson(second.state),
          "replacement selection remains deterministic");
    Check(first.market == market && first.state.purchased == 1,
          "replacement preserves frozen stock and purchase bits");
    Check(ValidateReplacements(market, first.state),
          "new replacement remains loadable");
  }
  // Force the former candidate to be the only usable item. Exhaustion must
  // leave a blocked offer, rather than introducing an unavailable item.
  auto exhausted = Open(market);
  Check(!exhausted.RefreshOwned(owned, IsTrirod),
        "MM Trirod-only replacement candidates exhaust safely");
  Check(exhausted.state.replacements[5].empty(),
        "exhaustion does not mutate replacement state");
  for (Games games : {Games::Oot, Games::Both}) {
    auto other = Open(FrozenMarket(games));
    Check(other.RefreshOwned(owned, IsTrirod) && IsTrirod(*other.Offer(5)),
          "Trirod remains a real replacement in OoT and Combo");
  }
  std::cout << "PASS replacement: 2048 deterministic MM rolls, exhaustion, "
               "OoT/Combo Trirod and preserved slots\n";
}

static void Filter() {
  const std::set<std::string> choices = {"nei.shovel",
                                         "nei.lantern",
                                         "wildcard.magic_upgrade",
                                         "nei.wand.sand",
                                         "nei.cane.blocks",
                                         "nei.cane.flip",
                                         "wildcard.double_defense",
                                         "nei.trident",
                                         "nei.cane.trirod"};
  Options options;
  options.games = Games::Mm;
  options.eligible = choices;
  for (uint32_t seed = 0; seed < 128; ++seed) {
    const auto market = Generate(seed, options);
    CheckInventory(market, true);
    Check(market.at("eligible") == json(choices),
          "explicit eligibility metadata keeps its serialized identities");
  }
  options.eligible = {"nei.cane.trirod"};
  bool rejected = false;
  try {
    (void)Generate(1, options);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  Check(rejected, "insufficient unavailable-only explicit filter rejects with "
                  "the existing quota error");
  options = OptionsFromJson({{"games", "mm"}});
  for (uint32_t seed = 0; seed < 128; ++seed)
    CheckInventory(Generate(seed, options), true);
  CheckInventory(Generate(5, OptionsFromJson(nullptr)), false);
  std::cout << "PASS filters: explicit unavailable choices, safe exhaustion "
               "and parsed/default mode options\n";
}

static void Legacy() {
  const auto *trirod = Find("nei.cane.trirod");
  Check(trirod && std::string_view(trirod->identity) ==
                      "RI_OOT_NEI_CANE_SOMARIA_PLATFORM",
        "legacy item identity remains available");
  for (bool base : {false, true}) {
    auto market = FrozenMarket(Games::Mm, base);
    State state;
    state.purchased = 0x81;
    if (!base)
      state.replacements[5] = "nei.cane.trirod";
    Check(Validate(market) && ValidateReplacements(market, state),
          "frozen MM Trirod stock and replacements remain valid");
    const json saved = {{"market", market}, {"state", StateToJson(state)}};
    auto session = Open(market);
    session.Load(json::parse(saved.dump()), 1);
    Check(session.Save(1) == saved && IsTrirod(*session.Offer(5)),
          "legacy JSON round-trip keeps exact Trirod and paid bits");
    Check(!session.RefreshOwned([](const Item &) { return false; },
                                [](const Item &) { return true; }),
          "unowned frozen Trirod is never rerolled");
    Check(session.Save(1) == saved,
          "load and idle refresh do not rewrite existing seed data");
    Check(CanPurchase(market, session.state, 5, 999, 0, false, true) ==
              PurchaseResult::Locked,
          "legacy unlock gate remains active");
    Check(CanPurchase(market, session.state, 5, 0, 12, false, true) ==
              PurchaseResult::NeedRupees,
          "legacy price remains active");
    Check(CanPurchase(market, session.state, 5, 999, 12, false, true) ==
              PurchaseResult::Ready,
          "historical Trirod still follows purchase policy");
    CommitPurchase(session.state, 5);
    Check(CanPurchase(market, session.state, 5, 999, 12, false, true) ==
              PurchaseResult::SoldOut,
          "legacy purchase bit is retained");
  }
  std::cout << "PASS legacy: frozen Trirod stock/replacements deserialize "
               "unchanged; prices, gates and paid slots preserved\n";
}

int main(int argc, char **argv) {
  Check(argc == 2, "runner must select a case");
  const std::string_view name(argv[1]);
  if (name == "generation")
    Generation();
  else if (name == "replacement")
    Replacement();
  else if (name == "filter")
    Filter();
  else if (name == "legacy")
    Legacy();
  else
    Check(false, "unknown test case");
}
