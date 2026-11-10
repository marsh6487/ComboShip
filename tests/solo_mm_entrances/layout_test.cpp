#include "combo/rando/SoloEntranceLayout.h"
#include "combo/rando/CrossDungeonSeed.h"
#include <cassert>
#include <functional>
#include <iostream>

using namespace ComboRando::CrossDungeon;
namespace Solo = ComboRando::SoloEntrance;

static void Rejects(const std::function<void()>& action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    assert(rejected);
}

static Dungeon Portal(const char* id, int entrance, Pool pool) {
    Dungeon d{ id, {1, entrance}, {1, entrance + 1}, {1, entrance + 1} };
    d.pool = pool;
    return d;
}

static Dungeon Boss(const char* id, int entrance, int identity) {
    auto d = Portal(id, entrance, Pool::Boss);
    d.boss = identity;
    d.owner = "mm:temple";
    d.hasReverse = false;
    d.escapeExits = {d.exit};
    d.warpReturn = d.exit;
    d.reverseCheckpoint = d.warpCheckpoint = d.deathCheckpoint = Endpoint{1, 10};
    return d;
}

int main() {
    std::vector<Dungeon> catalogue = {
        Portal("mm:temple", 10, Pool::Dungeon), Boss("mm:boss:8", 20, 8),
        Portal("mm:house", 30, Pool::Interior), Portal("mm:grotto", 40, Pool::Grotto),
        Portal("mm:overworld", 50, Pool::Overworld), Portal("mm:shop", 60, Pool::Shop),
        Boss("mm:boss:9", 70, 9)
    };
    Solo::Policy mixed{false, true, true, true, true, true, true, true};
    const std::vector<size_t> forward = {2, 3, 6, 4, 0, 5, 1};
    const auto inverse = Solo::Inverse(forward);
    auto routes = Solo::BuildRoutes(catalogue, forward, inverse, mixed);
    assert((Resolve(routes, Phase::AfterNative, {1, 10}) == Endpoint{1, 30}));
    assert((Resolve(routes, Phase::BeforeNative, {1, 31}) == Endpoint{1, 11}));
    assert((Resolve(routes, Phase::AfterNative, {1, 30}) == Endpoint{1, 70}));
    assert((Resolve(routes, Phase::BeforeNative, {1, 71}) == Endpoint{1, 31}));
    assert(Solo::PhysicalBossClears(catalogue, mixed));
    assert(Solo::BossSource(catalogue, forward, mixed, 8) == 8);
    assert(Solo::BossSource(catalogue, forward, mixed, 9) == 9);

    mixed.decoupled = true;
    const std::vector<size_t> independent = {1, 2, 3, 4, 5, 6, 0};
    routes = Solo::BuildRoutes(catalogue, forward, independent, mixed);
    assert((Resolve(routes, Phase::AfterNative, {1, 30}) == Endpoint{1, 70}));
    assert((Resolve(routes, Phase::BeforeNative, {1, 71}) == Endpoint{1, 11}));
    assert((Resolve(routes, Phase::BeforeNative, {1, 31}) == Endpoint{1, 41}));
    assert(Solo::BossSource(catalogue, forward, mixed, 8) == 8);

    mixed.decoupled = false;
    Rejects([&] { Solo::BuildRoutes(catalogue, forward, independent, mixed); });
    auto separate = mixed; separate.mixed = false;
    Rejects([&] { Solo::BuildRoutes(catalogue, forward, inverse, separate); });
    auto duplicate = forward; duplicate[0] = duplicate[1];
    Rejects([&] { Solo::BuildRoutes(catalogue, duplicate, inverse, mixed); });
    auto foreign = catalogue; foreign[0].returnTo.game = 0;
    Rejects([&] { Solo::BuildRoutes(foreign, forward, inverse, mixed); });
    auto hiddenForeign = catalogue; hiddenForeign[1].deathCheckpoint = Endpoint{0, 10};
    Rejects([&] { Solo::BuildRoutes(hiddenForeign, forward, inverse, mixed); });

    // Enabling broad interiors does not mix shops unless its own mix flag is selected.
    auto selective = mixed; selective.shops = false;
    auto shopSwap = std::vector<size_t>{0, 1, 5, 3, 4, 2, 6};
    Rejects([&] { Solo::BuildRoutes(catalogue, shopSwap, Solo::Inverse(shopSwap), selective); });
    auto generated = Solo::Generate(catalogue, 137, selective);
    assert(generated.forward[5] == 5 && generated.reverse[5] == 5);
    assert(generated.forward == Solo::Generate(catalogue, 137, selective).forward);
    assert(generated.reverse == Solo::Inverse(generated.forward));
    selective.decoupled = true;
    generated = Solo::Generate(catalogue, 137, selective);
    assert(generated.reverse != Solo::Inverse(generated.forward));
    assert(generated.forward[5] == 5 && generated.reverse[5] == 5);
    Solo::BuildRoutes(catalogue, generated.forward, generated.reverse, selective);

    // The established combined codec still rejects cross-category permutations.
    Rejects([&] { ComboRando::CrossDungeon::FromPermutation(catalogue, forward); });
    std::vector<Dungeon> bosses{Boss("mm:boss:8", 20, 8), Boss("mm:boss:9", 70, 9)};
    auto bossOnly = std::vector<size_t>{1, 0};
    assert(!Solo::PhysicalBossClears(bosses, mixed));
    assert(Solo::BossSource(bosses, bossOnly, mixed, 8) == 9);
    assert(Solo::BossSource(bosses, bossOnly, mixed, 9) == 8);
    std::cout << "PASS Solo mixed/decoupled permutations, coupled returns, boss ownership, category isolation\n";
}
