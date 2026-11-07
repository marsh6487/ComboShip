#include "rando/BlackMarket.h"
#include <cassert>
#include <iostream>
#include <set>

using namespace ComboRando::BlackMarket;

nlohmann::json Frozen(std::array<const char*, 8> keys, Games games = Games::Both) {
    auto market = Generate(912);
    market["stock"] = nlohmann::json::array();
    for (const char* key : keys) {
        const auto* item = Find(key);
        assert(item);
        market["stock"].push_back({ { "key", key }, { "identity", item->identity }, { "tier", item->tier },
                                     { "price", item->price }, { "clears", item->tier - 1 } });
    }
    if (games != Games::Both)
        market["games"] = games == Games::Oot ? "oot" : "mm";
    assert(Validate(market));
    return market;
}

void ReplacementTests() {
    const auto market = Frozen({ "nei.shovel", "nei.beetle", "nei.gust_jar", "nei.fire_rod", "nei.ice_rod",
                                 "nei.magic_cape", "wildcard.mask_truth", "wildcard.double_defense" });
    const std::array<const char*, 6> retired{
        "wildcard.command_melody", "wildcard.great_fairy_mask", "wildcard.mask_truth",
        "wildcard.gibdo_mask", "wildcard.garo_mask", "wildcard.captains_hat"
    };
    for (const char* key : retired) {
        const auto* item = Find(key);
        assert(item && !Selectable(*item));
        auto old = item->tier == 1
            ? Frozen({ key, "nei.shovel", "nei.gust_jar", "nei.fire_rod", "nei.ice_rod", "nei.bomb_arrows",
                       "nei.magic_cape", "wildcard.double_defense" })
            : Frozen({ "nei.shovel", "nei.beetle", "nei.gust_jar", "nei.fire_rod", "nei.ice_rod",
                       "nei.magic_cape", key, "wildcard.double_defense" });
        Session legacy;
        assert(legacy.Bind(old, 0));
        const int shelf = item->tier == 1 ? 0 : 6;
        CommitPurchase(legacy.state, shelf);
        const auto paid = legacy.Save(0);
        legacy.Load(paid, 0);
        assert(legacy.Save(0) == paid && legacy.Offer(shelf) == item);
        assert(CanPurchase(old, legacy.state, shelf, 999, 12, true, true) == PurchaseResult::SoldOut);
    }
    int activeWildcards = 0;
    for (const auto& item : Catalog()) activeWildcards += item.wildcard && Selectable(item);
    assert(activeWildcards == 14);
    std::set<std::string> owned{ "nei.beetle" };
    const auto isOwned = [&](const Item& item) { return owned.contains(item.key); };
    const auto usable = [](const Item&) { return true; };
    Session first, second;
    assert(first.Bind(market, 0) && second.Bind(market, 0));
    // A paid retired item stays paid, while an unpurchased owned tool changes.
    CommitPurchase(first.state, 6);
    CommitPurchase(second.state, 6);
    const auto frozen = first.market;
    assert(first.RefreshOwned(isOwned, usable));
    assert(second.RefreshOwned(isOwned, usable));
    assert(first.Save(0) == second.Save(0));
    assert(first.market == frozen && first.state.purchased == 64);
    assert(std::string_view(first.Offer(1)->key) != "nei.beetle");
    assert(first.Offer(1)->tier == 1 && !first.Offer(1)->wildcard);
    assert(std::string_view(first.Offer(6)->key) == "wildcard.mask_truth");
    assert(!first.RefreshOwned(isOwned, usable));
    assert(CanPurchase(first.market, first.state, 1, market["stock"][1]["price"], 0, false, true) ==
           PurchaseResult::Ready);
    const auto fingerprint = first.OfferFingerprint();
    CommitPurchase(first.state, 0);
    assert(first.OfferFingerprint() == fingerprint);
    first.state.purchased = 64;
    auto saved = first.Save(0);
    Session restored;
    assert(restored.Bind(market, 0));
    restored.Load(saved, 0);
    assert(restored.Save(0) == saved && !restored.RefreshOwned(isOwned, usable));
    // Acquiring a saved replacement elsewhere gives one new stable replacement.
    owned.insert(restored.Offer(1)->key);
    assert(restored.RefreshOwned(isOwned, usable));
    assert(restored.state.replacements[1] != first.state.replacements[1]);
    assert(restored.OfferFingerprint() != fingerprint);
    assert(restored.market["stock"][1] == market["stock"][1]);
    assert(!restored.RefreshOwned(isOwned, usable));
    std::set<std::string> unique;
    for (int i = 0; i < 8; ++i) assert(unique.insert(restored.Offer(i)->key).second);
    // Override validation cannot create foreign, duplicate, retired, renewable,
    // or different-tier stock. Paid bits survive bad optional override data.
    for (const char* bad : { "no.such.item", "nei.shovel", "wildcard.command_melody", "wildcard.chateau",
                             "wildcard.fierce_deity", "nei.fire_rod" }) {
        auto invalid = saved;
        invalid["state"]["replacements"][1] = bad;
        restored.Load(invalid, 0);
        assert(restored.state.purchased == 64);
        assert((restored.state.replacements == std::array<std::string, 8>{}));
    }
    auto invalid = saved;
    invalid["state"]["replacements"] = { "nei.beetle", 1 };
    restored.Load(invalid, 0);
    assert(restored.state.purchased == 64 && restored.state.replacements[1].empty());
    restored.Load({ { "market", market }, { "state", { { "purchased", 64 } } } }, 0);
    assert(restored.state.purchased == 64 && restored.state.replacements[1].empty());
    // Exhausted or unusable candidates leave an honest blocked offer unchanged.
    assert(restored.RefreshOwned([](const Item&) { return true; }, usable) == false);
    assert(restored.RefreshOwned(isOwned, [](const Item&) { return false; }) == false);
    assert(restored.market == market && restored.state.purchased == 64);
    // A legacy OoT Command Melody slot has no remaining tier-one wildcard.
    // It falls back to an OoT-capable tier-one tool without changing its price.
    const auto legacyOot = Frozen({ "nei.shovel", "wildcard.command_melody", "nei.gust_jar", "nei.fire_rod",
                                    "nei.ice_rod", "nei.magic_cape", "nei.bomb_arrows", "wildcard.double_defense" },
                                  Games::Oot);
    Session solo;
    assert(solo.Bind(legacyOot, 0));
    assert(solo.RefreshOwned([](const Item&) { return false; }, usable));
    assert(solo.Offer(1)->tier == 1 && !solo.Offer(1)->wildcard && Eligible(*solo.Offer(1), Games::Oot));
    assert(solo.market == legacyOot && solo.market["stock"][1]["price"] == 80);
    auto soloSaved = solo.Save(0);
    solo.Load(soloSaved, 0);
    assert(solo.Save(0) == soloSaved);
    auto foreign = soloSaved;
    foreign["state"]["replacements"][7] = "wildcard.fierce_deity";
    solo.Load(foreign, 0);
    assert(solo.state.replacements[7].empty());
    // Renewable refills never change or become sold out even after purchase.
    Session refill;
    const auto refillMarket = Frozen({ "nei.shovel", "nei.beetle", "nei.gust_jar", "nei.fire_rod", "nei.ice_rod",
                                      "nei.magic_cape", "wildcard.bunny_hood", "wildcard.chateau" });
    assert(refill.Bind(refillMarket, 0));
    CommitPurchase(refill.state, 7);
    assert(!refill.RefreshOwned([](const Item& item) { return item.renewable; }, usable));
    assert(refill.Offer(7)->renewable);
    assert(CanPurchase(refill.market, refill.state, 7, 999, 12, true, true) == PurchaseResult::Ready);
    // Honor an explicit generation eligibility set during future replacements.
    auto restricted = market;
    restricted["eligible"] = nlohmann::json::array();
    for (const auto& offer : market["stock"]) restricted["eligible"].push_back(offer["key"]);
    Session filtered;
    assert(filtered.Bind(restricted, 0));
    assert(!filtered.RefreshOwned(isOwned, usable));
    assert(filtered.Offer(1) == Find("nei.beetle"));
    assert(first.Bind(Generate(913), 0));
    assert(first.state.purchased == 0 && first.state.replacements[1].empty());
}

int main() {
    ReplacementTests();
    {
        using namespace ComboRando::BlackMarket;
        Session session;
        const auto seed = Generate(77);
        assert(!session.Bind(nlohmann::json(), 0));
        assert(session.Bind(seed, 0));
        CommitPurchase(session.state, 3);
        const auto saved = session.Save(0);
        assert(session.Save(1).is_null());
        assert(session.Bind(seed, 0));
        assert(session.state.purchased == 8);
        session.Load(nlohmann::json(), 0);
        assert(session.state.purchased == 0);
        session.Load(saved, 0);
        assert(session.state.purchased == 8);
        session.Load(saved, 1);
        assert(session.state.purchased == 0);
        assert(session.Bind(Generate(78), 0));
        session.Load(saved, 0);
        assert(session.state.purchased == 0);
        assert(!session.Bind(nlohmann::json::object(), 0));
        assert(!session.Active(0));
    }
    const auto* melody = Find("wildcard.command_melody");
    assert(melody && melody->wildcard && !melody->renewable);
    assert(std::string_view(melody->identity) == "RI_OOT_SONG_COMMAND_MELODY");
    for (auto game : {Games::Oot, Games::Mm, Games::Both}) assert(Eligible(*melody, game));
    Options options;
    const auto first = Generate(123456u, options);
    assert(Validate(first));
    assert(first == Generate(123456u, options));
    assert(first != Generate(123457u, options));
    std::set<std::string> seen;
    for (uint32_t seed = 0; seed < 4096; ++seed) {
        const auto market = Generate(seed, options);
        assert(Validate(market));
        assert(market.at("stock").size() == 8);
        std::set<std::string> unique;
        int wildcards = 0;
        int tiers[3]{};
        for (const auto& offer : market.at("stock")) {
            const auto* item = Find(offer.at("key").get<std::string>());
            assert(item);
            assert(unique.insert(item->key).second);
            seen.insert(item->key);
            wildcards += item->wildcard;
            ++tiers[item->tier - 1];
            assert(offer.at("price") == item->price);
            assert(offer.at("clears") == item->tier - 1);
            assert(item->price >= (item->tier == 1 ? 30 : item->tier == 2 ? 100 : 160));
            assert(item->price <= (item->tier == 1 ? 90 : item->tier == 2 ? 150 : 200));
        }
        assert(wildcards == 2);
        assert(tiers[0] == 2 && tiers[1] == 4 && tiers[2] == 2);
    }
    for (const auto& item : Catalog())
        assert(seen.contains(item.key) == Selectable(item));
    assert(Catalog().size() >= 75);
    // A solo OoT file must never roll native MM masks/refills that need MM
    // assets, while retaining the fixed 6 NEI / 2 wildcard and 2/4/2 quotas.
    auto ootOptions = OptionsFromJson({ { "games", "oot" } });
    auto mmOptions = OptionsFromJson({ { "games", "mm" } });
    for (uint32_t seed = 0; seed < 512; ++seed) {
        const auto oot = Generate(seed, ootOptions);
        const auto mm = Generate(seed, mmOptions);
        assert(Validate(oot) && Validate(mm));
        assert(oot.value("games", "both") == "oot");
        assert(mm.value("games", "both") == "mm");
        for (const auto& offer : oot["stock"]) {
            const auto key = offer["key"].get<std::string>();
            assert(!key.starts_with("wildcard.") || key == "wildcard.mario_mask" ||
                   key == "wildcard.poke_ball" || key == "wildcard.magic_upgrade" ||
                   key == "wildcard.double_defense");
        }
    }
    assert(!first.contains("games")); // combined seeds retain their legacy games default
    auto foreignOffer = Generate(7, mmOptions);
    foreignOffer["games"] = "oot";
    foreignOffer["games"] = "invalid";
    assert(!Validate(foreignOffer));
    assert(Find("wildcard.fierce_deity"));
    assert(Find("wildcard.poke_ball"));
    assert(Find("wildcard.mario_mask"));
    assert(Find("nei.cane.byrna"));
    assert(Find("nei.season.winter"));
    assert(Find("nei.rune.sensor"));
    assert(Find("nei.cane.ultrahand"));

    options.dungeonUnlocks = false;
    const auto ungated = Generate(123456u, options);
    for (size_t i = 0; i < 8; ++i) {
        assert(ungated["stock"][i]["key"] == first["stock"][i]["key"]);
        assert(ungated["stock"][i]["clears"] == 0);
    }
    options.pricing = Pricing::FollowSoH;
    options.soh.mode = PriceMode::Fixed;
    options.soh.fixedPrice = 375;
    auto fixed = Generate(123456u, options);
    assert(Validate(fixed));
    for (size_t i = 0; i < 8; ++i) {
        assert(fixed["stock"][i]["key"] == first["stock"][i]["key"]);
        assert(fixed["stock"][i]["price"] == 375);
    }
    options.soh.mode = PriceMode::Wallet;
    options.soh.walletWeights = { 0, 1, 0, 0, 0 };
    for (uint32_t seed = 0; seed < 100; ++seed) {
        const auto walletMarket = Generate(seed, options);
        for (const auto& offer : walletMarket.at("stock"))
            assert(offer.at("price") >= 1 && offer.at("price") <= 99);
    }
    options.soh.walletWeights = { 0, 0, 0, 0, 1 };
    options.soh.tycoon = false;
    const auto noTycoon = Generate(9, options);
    for (const auto& offer : noTycoon.at("stock"))
        assert(offer.at("price") <= 500);

    assert(!Validate(nlohmann::json::object()));
    auto invalid = first;
    invalid["version"] = 999;
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][1] = invalid["stock"][0];
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][0]["key"] = "unknown.item";
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][0]["price"] = -1;
    assert(!Validate(invalid));
    invalid = first;
    invalid["stock"][0]["identity"] = "RI_WALLET_GIANT";
    assert(!Validate(invalid));

    State state;
    assert(StateFromJson(StateToJson(state)).purchased == 0);
    int tier3slot = -1;
    for (int i = 0; i < 8; ++i)
        if (first["stock"][i]["tier"] == 3)
            tier3slot = i;
    assert(tier3slot >= 0);
    assert(CanPurchase(first, state, tier3slot, 999, 0, false, true) == PurchaseResult::Locked);
    assert(CanPurchase(first, state, tier3slot, 0, 2, false, true) == PurchaseResult::NeedRupees);
    assert(CanPurchase(first, state, tier3slot, 999, 2, true, true) == PurchaseResult::Owned);
    assert(CanPurchase(first, state, tier3slot, 999, 2, false, false) == PurchaseResult::Unavailable);
    assert(CanPurchase(first, state, tier3slot, 999, 2, false, true) == PurchaseResult::Ready);
    CommitPurchase(state, tier3slot);
    assert(CanPurchase(first, state, tier3slot, 999, 2, false, true) == PurchaseResult::SoldOut);
    const auto reloaded = StateFromJson(StateToJson(state));
    assert(reloaded.purchased == state.purchased);
    assert(CanPurchase(first, reloaded, tier3slot, 999, 2, false, true) == PurchaseResult::SoldOut);
    assert(CanPurchase(first, state, -1, 999, 2, false, true) == PurchaseResult::Unavailable);
    assert(CanPurchase(first, state, 8, 999, 2, false, true) == PurchaseResult::Unavailable);
    assert(StateFromJson(nlohmann::json{ { "purchased", -1 } }).purchased == 0);
    assert(ClearCount(0xFFF) == 12);
    assert(ClearCount(0xFFFF) == 12);
    std::cout << "PASS: 4096 filtered inventories; persistent deterministic replacements, legacy saves, quotas, pricing and purchases\n";
}
