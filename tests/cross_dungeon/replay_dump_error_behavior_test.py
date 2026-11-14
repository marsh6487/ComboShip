#!/usr/bin/env python3
"""Execute the actual CLI replay dump guard; native exporters are service boundaries.

This checks setup-error propagation before traversal, not a native graph/fill.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'combo/ComboRandoHeadless.cpp').read_text()
start = source.index('            std::string sohDump = SOH_Dump(), mmDump = MM_Dump();')
end = source.index('            const std::string sharedPairs', start)
guard = source[start:end]
start = source.index('        auto r2 = runPass(true, &pt2);')
end = source.index('        if (r2.beatable)', start)
second_pass_guard = source[start:end]
program = r'''
#include "combo/rando/ComboPlaythrough.h"
#include "combo/rando/CrossDungeonGeneration.h"
#include <cassert>
#include <cstdint>
std::string oot, mm, preparedForce;
nlohmann::json crossSeed, recordedForcedOot;
constexpr uint32_t masterSeed = 20261109;
uint32_t observedForceSeed = 0;
bool entranceRestoreFailed = false;
int traversals = 0, forceCalls = 0;
const char* SOH_Dump() { return oot.c_str(); }
const char* MM_Dump() { return mm.c_str(); }
const char* PreparedForce(uint32_t seed) {
    ++forceCalls;
    observedForceSeed = seed;
    return preparedForce.c_str();
}
const char* (*SOH_GetForced)(uint32_t) = PreparedForce;
void ResetBoundary() {
    entranceRestoreFailed = false;
    traversals = forceCalls = 0;
    observedForceSeed = 0;
}
ComboRando::PlaythroughResult ReplayBoundary() {
''' + guard + r'''
    ++traversals;
    ComboRando::PlaythroughResult result;
    result.beatable = true;
    return result;
}
int SecondPassExitBoundary(bool failed) {
    bool entranceRestoreFailed = failed;
    nlohmann::json pt2;
    auto runPass = [](bool, nlohmann::json*) { return ComboRando::PlaythroughResult{}; };
''' + second_pass_guard + r'''
    (void)r2;
    return 3;
}
int main() {
    using namespace ComboRando::CrossDungeon;
    const std::vector<Dungeon> dungeons{{"oot:test", {0, 1}, {0, 2}, {0, 3}},
                                         {"mm:test", {1, 4}, {1, 5}, {1, 6}}};
    crossSeed = Encode(dungeons, {1, 0});
    recordedForcedOot = RecordedForcedOot(nlohmann::json::parse(
        R"json({"oot":{"placements":{"Link's Pocket":"Kokiri's Emerald (OOT)"}}})json"));
    preparedForce = R"({"Link's Pocket":{"item":"Kokiri's Emerald"}})";
    const std::string validOot = R"({"pool":[{"name":"Goron's Ruby","advancement":true}],"fixed":[]})";
    for (const auto& bad : {std::string(R"({"error":"native budget failed","fixed":[]})"),
                            std::string("{bad"), std::string("[]")}) {
        for (bool badOot : {false, true}) {
            oot = badOot ? bad : validOot;
            mm = badOot ? "{}" : bad;
            ResetBoundary();
            assert(!ReplayBoundary().beatable && entranceRestoreFailed && traversals == 0 && forceCalls == 0);
        }
    }
    oot = validOot;
    mm = "{}";
    ResetBoundary();
    assert(ReplayBoundary().beatable && !entranceRestoreFailed && traversals == 1 && forceCalls == 1);
    assert(observedForceSeed == masterSeed);
    // The real guard must reject a regenerated pool that omitted another native reward.
    oot = R"({"pool":[{"name":"Kokiri's Emerald","advancement":true}],"fixed":[]})";
    preparedForce = R"({"Link's Pocket":{"item":"Goron's Ruby"}})";
    ResetBoundary();
    assert(!ReplayBoundary().beatable && entranceRestoreFailed && traversals == 0 && forceCalls == 1);
    assert(observedForceSeed == masterSeed);
    // Legacy metadata without a cross map still bypasses the forced-placement service.
    crossSeed = nullptr;
    oot = mm = "{}";
    ResetBoundary();
    assert(ReplayBoundary().beatable && !entranceRestoreFailed && traversals == 1 && forceCalls == 0);
    assert(SecondPassExitBoundary(true) == 2); // A setup failure is not a circular-seed verdict.
    assert(SecondPassExitBoundary(false) == 3);
}
'''
with tempfile.TemporaryDirectory(prefix='replay-dump-error-') as temporary:
    folder = Path(temporary)
    cpp = folder / 'test.cpp'
    cpp.write_text(program)
    binary = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT), '-I' + str(ROOT / 'combo'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS actual CLI replay guard: native dump errors precede force reads/traversal, recorded Pocket is exact, '
      'legacy metadata and second-pass setup exits preserved')
