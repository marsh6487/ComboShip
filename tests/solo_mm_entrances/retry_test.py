#!/usr/bin/env python3
"""Native retry RNG must not depend on work consumed by a failed fill attempt.

Execute the production retry block and the real native PRNG. The slow graph
search is the controlled boundary: it consumes different amounts of random
work before failing, then exposes the next attempt's placement-choice stream.
The linked native regression separately reproduces the actual timed-out DFS.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-rev", help="production retry revision for the negative control")
args = parser.parse_args()


def block(source, marker):
    start = source.index(marker)
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


relative = "mm/2s2h/Rando/Logic/SeedGeneration.cpp"
generation = (subprocess.check_output(["git", "show", args.source_rev + ":" + relative],
                                      cwd=ROOT, text=True) if args.source_rev else
              (ROOT / relative).read_text())
start = generation.index("        const int maxAttempts")
loop = generation.index("        for (int attempt", start)
retry = generation[start:loop] + block(generation, "        for (int attempt")
random_source = (ROOT / "mm/2s2h/ShipUtils.cpp").read_text()
random_source = random_source[random_source.index("static bool seeded = false;"):
                              random_source.index("extern uint32_t Ship_Hash(")]

program = r'''
#include <cassert>
#include <bit>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
using u64 = uint64_t;
using s32 = int32_t;
using RandoCheckId = int;
using RandoItemId = int;
#define SPDLOG_WARN(...) ((void)0)
''' + random_source + r'''
struct RandoSaveInfo {};
struct { struct { struct { RandoSaveInfo rando; } shipSaveInfo; } save; } gSaveContext;
static std::vector<size_t> rejectedWork;
static std::vector<int> placementChoices;
static size_t attempts;
namespace Rando::SoloEntrances {
// Entrance shuffle is off in this isolated native retry case.
void Generate(RandoSaveInfo&, uint32_t, uint32_t) {}
}
namespace Rando::Logic {
void ApplyGlitchlessLogicToSaveContext(std::vector<RandoCheckId>& checks,
                                     std::vector<RandoItemId>& items) {
    assert((checks == std::vector<int>{10, 20, 30}));
    assert((items == std::vector<int>{101, 102, 103}));
    ++attempts;
    if (attempts <= rejectedWork.size()) {
        for (size_t i = 0; i < rejectedWork[attempts - 1]; ++i)
            Ship_Random(0, 1000000);
        checks.pop_back();
        items.pop_back();
        throw std::runtime_error("controlled failed native search");
    }
    for (int i = 0; i < 8; ++i)
        placementChoices.push_back(Ship_Random(0, 1000000));
    checks.clear();
    items.clear();
}
}
namespace Rando {
std::vector<int> RunAttempts(uint32_t finalSeed, const std::vector<size_t>& failedWork) {
    rejectedWork = failedWork;
    placementChoices.clear();
    attempts = 0;
    Ship_Random_Seed(finalSeed);
    // Represent fixed pre-fill pool/price rolls. Attempt one must retain the
    // accepted stream position after that native preprocessing has completed.
    for (int i = 0; i < 37; ++i)
        Ship_Random(0, 1000000);
    std::vector<RandoCheckId> checkPool{10, 20, 30};
    std::vector<RandoItemId> itemPool{101, 102, 103};
''' + retry + r'''
    return placementChoices;
}
}
int main() {
    using Rando::RunAttempts;
    const auto first = RunAttempts(0x12345678, {});
    assert((first == std::vector<int>{421132, 849538, 538243, 661347,
                                     332786, 780209, 913616, 535842}) &&
           "successful first attempts must preserve the accepted native stream");
    const auto second = RunAttempts(0x12345678, {1});
    assert(attempts == 2);
    assert(second == RunAttempts(0x12345678, {7000}) &&
           "retry placement choices must not depend on failed search work");
    assert(second != first && "a retry needs its own search stream");
    const auto third = RunAttempts(0x12345678, {7000, 1});
    assert(attempts == 3);
    assert(third == RunAttempts(0x12345678, {17, 19000}));
    assert(third != second && "attempt identity must select a distinct stream");
    assert(second != RunAttempts(0x12345679, {7000}) &&
           "retry placement choices must retain native seed identity");
    const auto tenth = RunAttempts(0x12345678, std::vector<size_t>(9, 1));
    assert(attempts == 10 && tenth.size() == 8);
    bool exhausted = false;
    try { RunAttempts(0x12345678, std::vector<size_t>(10, 1)); }
    catch (const std::runtime_error&) { exhausted = true; }
    assert(exhausted && attempts == 10 && placementChoices.empty());
    std::cout << "PASS native retry block and PRNG: failed-work isolation, first-attempt preservation, "
                 "seed/attempt streams, fresh pools and unchanged ten-attempt limit\n";
}
'''

with tempfile.TemporaryDirectory(prefix="solo-mm-retry-") as temporary:
    source, binary = Path(temporary) / "retry.cpp", Path(temporary) / "retry"
    source.write_text(program)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
