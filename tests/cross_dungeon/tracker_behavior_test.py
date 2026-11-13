#!/usr/bin/env python3
"""Compile the real read-only host bridge and tracker metadata/sort adapters, without ROM/UI runtime."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--source-root', type=Path, default=ROOT)
p.add_argument('--sanitize', action='store_true')
a = p.parse_args()
ui = (a.source_root/'soh/soh/Enhancements/randomizer/randomizer_entrance_tracker.cpp').read_text()
header = (ROOT/'soh/soh/Enhancements/randomizer/randomizer_entrance_tracker.h').read_text()
assert ui.count('IsEntranceDiscovered(entrance.index)') == 2
assert 'IsEntranceDiscovered(entrance.override)' not in ui
if 'trackerSeedRevision != Combo_SOH_TrackerSeedRevision()' in ui:
    draw = ui[ui.index('void EntranceTrackerWindow::DrawElement()'): ]
    assert draw.index('InitEntranceTrackingData();') < draw.index('GroupEntranceCounts')
host = (ROOT/'combo/rando/CrossDungeonOotRuntime.inc.h').read_text()
types = header[header.index('typedef enum {'):header.index('static std::map<int32_t, const char*> windowType')]
bridge = host[host.index('// Read-only tracker bridge.'):host.index('extern "C" int Combo_SOH_CrossBossEnabled')]
if 'static const EntranceData* GetDestinationData' in ui:
    metadata = ui[ui.index('// Every destination consumer'):ui.index('void LoadFromPreset(')]
    sorting = ui[ui.index('static void SortNativeEntranceListByType('):ui.index('s16 GetLastEntranceOverride()')]
    initialization = ui[ui.index('void InitEntranceTrackingData()'):ui.index('void EntranceTrackerSettingsWindow::DrawElement()')]
else:
    # Execute the old raw-metadata behavior against the exact failing scenario.
    metadata = 'const EntranceData* GetDestinationData(const EntranceOverride& e) { return GetEntranceData(e.override); }'
    sorting = ''
    initialization = ''
source = r'''
#include "combo/rando/CrossDungeonTracker.h"
#include <array>
#include <cassert>
#include <cctype>
#include <iostream>
#include <cstring>
using namespace ComboRando::CrossDungeon;
using s16 = int16_t; using u8 = uint8_t; using u16 = uint16_t;
constexpr int ENTRANCE_OVERRIDES_MAX_COUNT = 1024;
struct EntranceOverride { int16_t type, index, destination, override, overrideDestination; };
static bool combined = true;
bool Combo_IsCombinedGame() { return combined; }
namespace ComboCrossOotRuntime {
RuntimeState state(0);
std::vector<Dungeon> trackerCatalogue;
uint64_t trackerRevision = 0;
std::optional<RuntimeState::Transition> pending;
CheckpointState checkpoint;
}
''' + bridge + types + r'''
static const EntranceData data[] = {
 {0x88,-1,{},"Zoras Fountain","Ice Cavern",ENTRANCE_GROUP_ZORAS_FOUNTAIN,ENTRANCE_GROUP_ZORAS_FOUNTAIN,ENTRANCE_TYPE_DUNGEON,"ice",1},
 {0x8,-1,{},"Gerudo Fortress","Gerudo Training Ground",ENTRANCE_GROUP_GERUDO_FORTRESS,ENTRANCE_GROUP_GERUDO_FORTRESS,ENTRANCE_TYPE_DUNGEON,"gtg",1},
 {0x165,0x24a,{},"Death Mountain Crater","Fire Temple",ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER,ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER,ENTRANCE_TYPE_DUNGEON,"fire",1},
 {0x3d4,0x88,{},"Ice Cavern","Zoras Fountain",ENTRANCE_GROUP_ZORAS_FOUNTAIN,ENTRANCE_GROUP_ZORAS_FOUNTAIN,ENTRANCE_TYPE_DUNGEON,"ice exit",0}};
const EntranceData* GetEntranceData(s16 index) { for(const auto& d:data) if(d.index==index) return &d; return nullptr; }
bool Entrance_EntranceIsNull(const EntranceOverride* e) {return e->index==0 && e->override==0 && e->destination==0;}
''' + metadata + r'''
constexpr int RSK_SHUFFLE_ENTRANCES=0;
struct FixtureShuffler { std::array<EntranceOverride,ENTRANCE_OVERRIDES_MAX_COUNT> entranceOverrides{}; };
FixtureShuffler fixture;
namespace Rando { struct Context { static Context* GetInstance() {static Context c;return &c;} FixtureShuffler* GetEntranceShuffler() {return &fixture;} }; }
struct Randomizer { bool enabled=true; int GetRandoSettingValue(int) {return enabled;} };
Randomizer randomizer;
struct Globals { Randomizer* gRandomizer=&randomizer; };
namespace OTRGlobals { Globals globals; Globals* Instance=&globals; }
EntranceTrackingData gEntranceTrackingData{};
EntranceOverride srcListSortedByArea[ENTRANCE_OVERRIDES_MAX_COUNT]{}, destListSortedByArea[ENTRANCE_OVERRIDES_MAX_COUNT]{}, srcListSortedByType[ENTRANCE_OVERRIDES_MAX_COUNT]{}, destListSortedByType[ENTRANCE_OVERRIDES_MAX_COUNT]{};
#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))
const auto& entranceData=data;
static const EntranceOverride emptyOverride{};
''' + sorting + initialization + r'''
int main() {
 using namespace ComboCrossOotRuntime;
 trackerCatalogue = {{"oot:gtg",{0,8},{0,0x3a8},{0,0x3d4}},
 {"oot:fire",{0,0x165},{0,0x24a},{0,0x3d4}},
 {"mm:pirates-fortress",{1,0x165},{1,0x200},{1,0x201}}};
 state.Install(Encode(trackerCatalogue,{1,2,0}));
 EntranceOverride ice{0,0x88,0,8,0}, fire{0,0x165,0,0x165,0}, exit{0,0x24a,0,8,0};
 const auto iceNative = ice;
 checkpoint.Load(0x88);
 pending = state.Lookup(Phase::AfterNative,0x165);
 const auto pendingTo = pending->to;
 assert(state.QueueArrival(8));
 auto* shown = GetDestinationData(ice);
 assert(shown && shown->destination == "Fire Temple");
 assert(shown->dstGroup == ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER);
 assert(shown->metaTag.find("fire") != std::string::npos);
 assert(GetDestinationData(exit)->destination == "Zoras Fountain");
 assert(GetDestinationData(fire)->destination == "MM Pirates Fortress");
 assert(GetDestinationData(fire)->reverseIndex == -1);
 assert(GetDestinationData(fire)->dstGroup == ENTRANCE_GROUP_MM);
 for (int i=0;i<100;++i) { GetDestinationData(ice); GetDestinationData(fire); }
 assert(checkpoint.Saved()==0x88 && pending->to==pendingTo);
 assert(state.ConsumeArrival()==8);
 assert(std::memcmp(&ice,&iceNative,sizeof(ice))==0);
 combined=false;
 assert(GetDestinationData(ice)->destination=="Gerudo Training Ground");
 combined=true;
 state.Install(nullptr);trackerCatalogue.clear();++trackerRevision;
 assert(GetDestinationData(ice)->destination=="Gerudo Training Ground");
 state.Install(Encode({{"oot:gtg",{0,8},{0,0x3a8},{0,0x3d4}},
 {"mm:pirates-fortress",{1,0x165},{1,0x200},{1,0x201}}},{1,0}));
 trackerCatalogue={{"oot:gtg",{0,8},{0,0x3a8},{0,0x3d4}},
 {"mm:pirates-fortress",{1,0x165},{1,0x200},{1,0x201}}};++trackerRevision;
 assert(GetDestinationData(ice)->destination=="MM Pirates Fortress");
 EntranceOverride unknown{0,0x555,0,0x701,0};
 assert(GetDestinationData(unknown));
 assert(GetDestinationData(unknown)->destination.find("unknown")!=std::string::npos);
 std::cout << "tracker host/UI metadata tests passed\n";
}
'''
# Sorting coverage uses the exact production adapter; baseline has no adapter and
# the failing destination assertion above is the red regression.
if sorting:
    source = source.replace(' assert(GetDestinationData(ice)->destination=="Gerudo Training Ground");\n state.Install', ' InitEntranceTrackingData(); assert(gEntranceTrackingData.EntranceCount==1);\n assert(GetDestinationData(ice)->destination=="Gerudo Training Ground");\n state.Install')
    source = source.replace(' combined=false;', '''

 fixture.entranceOverrides[0]=ice;
 const auto nativeRows=fixture.entranceOverrides;
 InitEntranceTrackingData();
 assert(gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_AREA][ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER]>=1);
 assert(gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_AREA][ENTRANCE_GROUP_MM]>=1);
 assert(gEntranceTrackingData.EntranceCount>1); // Cross-only identity rows.
 assert(std::memcmp(fixture.entranceOverrides.data(),nativeRows.data(),sizeof(nativeRows))==0);
 assert(trackerSeedRevision==Combo_SOH_TrackerSeedRevision());
 for(int i=0;i<ENTRANCE_OVERRIDES_MAX_COUNT;++i)
     fixture.entranceOverrides[i]={0,static_cast<int16_t>(2000+i),-1,static_cast<int16_t>(2000+i),-1};
 InitEntranceTrackingData();
 assert(gEntranceTrackingData.EntranceCount==ENTRANCE_OVERRIDES_MAX_COUNT);
 size_t total=0;
 for(auto count:gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_AREA]) total+=count;
 assert(total==ENTRANCE_OVERRIDES_MAX_COUNT);
 fixture.entranceOverrides=nativeRows;
 EntranceOverride rows[ENTRANCE_OVERRIDES_MAX_COUNT]{};
 rows[0]=fire;rows[1]=ice;
 SortEntranceListByArea(rows,1);
 assert(rows[0].index==ice.index && rows[1].index==fire.index);
 SortEntranceListByType(rows,1);
 assert(rows[0].index==ice.index && rows[1].index==fire.index);
 assert(checkpoint.Saved()==0x88 && pending->to==pendingTo);
 assert(std::memcmp(&ice,&iceNative,sizeof(ice))==0);
 combined=false;''')
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp)/'tracker.cpp'; exe = Path(tmp)/'tracker'
    cpp.write_text(source)
    includes = [ROOT, ROOT.parent/'build-deps/usr/include']
    flags = ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'] if a.sanitize else []
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD',*flags,*[f'-I{x}' for x in includes],str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
