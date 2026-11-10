#include "combo/rando/CrossDungeonRuntime.h"
#include "mm/tests/test_require.h"
#include <cstdio>

using namespace ComboRando::CrossDungeon;

int main() {
    Dungeon house{ "house", { 1, 100 }, { 1, 101 }, { 1, 101 } };
    house.pool = Pool::Interior;
    Dungeon boss{ "boss", { 1, 200 }, { 1, 201 }, { 1, 202 } };
    boss.pool = Pool::Boss;
    boss.boss = 8;
    boss.owner = "woodfall";
    boss.hasReverse = false;
    boss.escapeExits = { { 1, 201 } };
    boss.warpReturn = Endpoint{ 1, 203 };
    boss.deathCheckpoint = Endpoint{ 1, 202 };
    Dungeon grotto{ "grotto", { 1, 300 }, { 1, 301 }, { 1, 301 } };
    grotto.pool = Pool::Grotto;
    const nlohmann::json seed{ { "soloVersion", 2 }, { "enabled", true } };
    std::vector<Route> routes{
        { Phase::AfterNative, house.entrance, boss.entrance, boss.id, boss.entrance },
        { Phase::AfterNative, boss.entrance, grotto.entrance, grotto.id, grotto.entrance },
        { Phase::AfterNative, grotto.entrance, house.entrance, house.id, house.entrance },
        // Deliberately decoupled: the boss warp exits at the grotto, although
        // the physical boss was entered through the house door.
        { Phase::BeforeNative, boss.exit, grotto.returnTo, grotto.id },
        { Phase::BeforeNative, grotto.exit, house.returnTo, house.id },
        { Phase::BeforeNative, house.exit, boss.returnTo, boss.id, boss.deathCheckpoint },
    };
    RuntimeState runtime(1);
    runtime.InstallValidated(seed, routes, { house, boss, grotto });
    const auto entered = runtime.Lookup(Phase::AfterNative, 100);
    REQUIRE(entered && entered->to == (Endpoint{ 1, 200 }));
    REQUIRE(entered->checkpoint == (Endpoint{ 1, 101 }));
    const auto escaped = runtime.Lookup(Phase::BeforeNative, 201);
    REQUIRE(escaped && escaped->to == (Endpoint{ 1, 301 }));
    const auto recovery = runtime.RecoveryForBoss({ 1, 200 });
    REQUIRE(recovery && recovery->source == (Endpoint{ 1, 100 }));
    REQUIRE(recovery->parent == (Endpoint{ 1, 101 }));
    REQUIRE(runtime.IsRecoveryEntrance(101));
    REQUIRE(!runtime.QueueBossRecovery({ 1, 200 }));

    // The local install boundary rejects foreign endpoints transactionally,
    // preserving the last valid map rather than arming a handoff.
    auto corrupt = routes;
    corrupt[0].to.game = 0;
    bool rejected = false;
    try { runtime.InstallValidated(seed, corrupt, { house, boss, grotto }); }
    catch (const std::invalid_argument&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(runtime.Lookup(Phase::AfterNative, 100)->to == (Endpoint{ 1, 200 }));
    REQUIRE(runtime.RecoveryForBoss({ 1, 200 })->parent == (Endpoint{ 1, 101 }));

    // Combined decoding continues to reject cross-category permutations.
    rejected = false;
    try { Encode({ house, boss, grotto }, { 1, 2, 0 }); }
    catch (const std::invalid_argument&) { rejected = true; }
    REQUIRE(rejected);
    runtime.Install(nullptr);
    REQUIRE(!runtime.Enabled());
    REQUIRE(!runtime.RecoveryForBoss({ 1, 200 }));
    std::puts("PASS explicit Solo runtime: mixed ingress, independent return, source death recovery and foreign rejection");
}
