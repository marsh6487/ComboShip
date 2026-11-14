#include "combo/rando/CrossDungeonTracker.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
int main() {
    std::vector<Dungeon> catalogue = {
        {"oot:gtg", {0,0x8}, {0,0x3a8}, {0,0x3d4}},
        {"oot:fire", {0,0x165}, {0,0x24a}, {0,0x3d4}},
        {"mm:pirates-fortress", {1,0x165}, {1,0x200}, {1,0x201}}};
    RuntimeState state(0);
    state.Install(Encode(catalogue, {1,2,0}));
    const RuntimeState& view = state;
    assert(ComposeTrackerDestination(view,true,{0,0x88},0x8)==(Endpoint{0,0x165}));
    assert(ComposeTrackerDestination(view,true,{0,0x165},0x165)==(Endpoint{1,0x165}));
    assert(ComposeTrackerDestination(view,true,{0,0x24a},0x8)==(Endpoint{0,0x3d4}));
    assert(ComposeTrackerDestination(view,false,{0,0x88},0x8)==(Endpoint{0,0x8}));
    assert(ComposeTrackerDestination(view,true,{1,0x88},0x8)==(Endpoint{1,0x8}));
    assert(FindTrackerPortal(catalogue,{1,0x165})->id=="mm:pirates-fortress");
    assert(FindTrackerPortal(catalogue,{0,0x165})->id=="oot:fire");
    assert(FindTrackerPortal(catalogue,{0,0x24a})->id=="oot:fire");
    assert(!FindTrackerPortal(catalogue,{1,0x700}));
    assert(state.QueueArrival(0x8));
    for(int i=0;i<100;++i) ComposeTrackerDestination(view,true,{0,0x88},0x8);
    assert(state.ConsumeArrival()==0x8);
    state.Install(nullptr);
    assert(ComposeTrackerDestination(view,true,{0,0x88},0x8)==(Endpoint{0,0x8}));
    assert(ComposeTrackerDestination(view,true,{0,0x700},0x701)==(Endpoint{0,0x701}));
    state.Install(Encode(catalogue,{2,0,1}));
    assert(ComposeTrackerDestination(view,true,{0,0x88},0x8)==(Endpoint{1,0x165}));
    std::cout << "tracker destination tests passed\n";
}
