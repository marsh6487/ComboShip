#!/usr/bin/env python3
"""Exercise the production region walker with a Solo MM entrance map."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'mm/2s2h/Rando/Logic/Logic.cpp').read_text()
start = source.index('void FindReachableRegions(')
end = source.index('// clang-format off', start)
walker = source[start:end]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
using s32 = int32_t;
using RandoRegionId = int;
enum { COMBO_GAME_MM = 1, COMBO_GAME_BOTH = 2, RR_MAX = 99 };
int gComboGameMode = COMBO_GAME_MM;
bool Combo_IsCombinedGame() { return gComboGameMode == COMBO_GAME_BOTH; }
int soloCalls = 0, combinedCalls = 0;
namespace Rando::SoloEntrances {
int ResolveEntrance(int index, int region) {
    ++soloCalls;
    return gComboGameMode == COMBO_GAME_MM && region == 0 && index == 101 ? 201 : index;
}
}
int Combo_MM_CrossDungeonEntranceFromRegion(int index, int) {
    ++combinedCalls;
    return index;
}
namespace Rando::Logic {
struct RegionTimeState { uint64_t timeSlices = 1; bool canStayOverTime = false; };
struct RegionExit { std::function<bool()> condition; };
struct Region {
    bool canStayOverTime = false;
    std::map<int, std::pair<std::function<bool()>, int>> connections;
    std::map<int, RegionExit> exits;
};
std::map<int, Region> Regions;
uint64_t gCurrentRegionTime = 1;
namespace TimeLogic {
uint64_t ExpandTimeForward(uint64_t time, Region&) { return time; }
}
void EnsureRegionTimeState(std::unordered_map<int, RegionTimeState>& states, int region) {
    states.try_emplace(region);
}
bool TimeStateCovers(const RegionTimeState& a, const RegionTimeState& b) {
    return (a.timeSlices | b.timeSlices) == a.timeSlices;
}
RegionTimeState MergeTimeStates(const RegionTimeState& a, const RegionTimeState& b) {
    return {a.timeSlices | b.timeSlices, a.canStayOverTime || b.canStayOverTime};
}
int GetRegionIdFromEntrance(int index) { return index == 101 ? 1 : index == 201 ? 2 : RR_MAX; }
'''
cases = r'''
}
int main() {
    using namespace Rando::Logic;
    bool doorOpen = true;
    Regions[0].exits[101] = {[&] { return doorOpen; }};
    Regions[1] = {}; Regions[2] = {};
    std::set<int> reached{0};
    std::unordered_map<int, RegionTimeState> times{{0, {1, false}}};
    FindReachableRegions(0, reached, times);
    assert(reached.contains(2) && !reached.contains(1) && "Solo MM must follow its saved entrance map");
    assert(combinedCalls == 0 && "Solo routing must not invoke the combined adapter");
    doorOpen = false; reached = {0}; times = {{0, {1, false}}};
    FindReachableRegions(0, reached, times);
    assert(reached.size() == 1 && "Shuffle must preserve the native source-door condition");
    doorOpen = true; gComboGameMode = COMBO_GAME_BOTH; reached = {0}; times = {{0, {1, false}}};
    FindReachableRegions(0, reached, times);
    assert(reached.contains(1) && !reached.contains(2) && combinedCalls == 1);
}
'''
with tempfile.TemporaryDirectory(prefix='solo-mm-logic-') as temp:
    cpp, binary = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture + walker + cases)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DCOMBO_BUILD', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS production native walker: Solo map, native source conditions, combined isolation')
