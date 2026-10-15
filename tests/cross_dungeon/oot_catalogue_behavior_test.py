#!/usr/bin/env python3
"""Compile actual OoT pool composition against native entrance descriptors and enum values."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
if args.sanitize: os.environ['ASAN_OPTIONS']='detect_leaks=0'
adapter=(ROOT/'combo/rando/CrossDungeonOot.inc.h').read_text()
assert 'SOH_DumpCrossDungeonCatalogueForOptions' in adapter, 'OoT opt-in pool catalogue export is missing'
def body(source,name):
    m=re.search(r'^(?:extern "C" )?(?:static )?[\w: *&<>,]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source,re.M)
    assert m, name+' production function is missing'
    return block_from(source,m.start())
native=(ROOT/'soh/soh/Enhancements/randomizer/entrance.cpp').read_text()
boss=(ROOT/'combo/rando/CrossEntranceOotBoss.inc.h').read_text()
constants=re.search(r'static const int32_t bossEntrances\[8\] = \{.*?\};',boss,re.S).group()
types=(ROOT/'soh/soh/Enhancements/randomizer/entrance.h').read_text()
entrance_enum=re.search(r'enum class EntranceType \{.*?\};',types,re.S).group()
link_info=re.search(r'typedef struct \{\n    EntranceType type;.*?\} EntranceLinkInfo;',types,re.S).group()
source='''
#include "global.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "rando/CrossDungeonCatalogue.h"
#include "rando/CrossDungeonState.h"
#include "rando/CrossDungeonRuntime.h"
#include <cassert>
#include <iostream>
namespace Rando {
'''+entrance_enum+'\n'+link_info+'''
using EntranceInfoPair=std::pair<EntranceLinkInfo,EntranceLinkInfo>;
EntranceLinkInfo NO_RETURN_ENTRANCE = { EntranceType::None, RR_NONE, RR_NONE, -1 };
'''+body(native,'GetEntranceShuffleTable')+'\n}\nnamespace ComboCrossOot {\nusing namespace ComboRando::CrossDungeon;\nstatic HostState state(0);\n'+constants+'\n'+body(boss,'OriginalEntranceScene')+'\n'+body(boss,'AppendBossCatalogue')+'\n'+body(adapter,'BaseCatalogue')+'\n'+body(adapter,'BlueWarpExits')+'\n'+body(adapter,'CatalogueJson')+'''
}
#define COMBO_EXPORT
static std::map<std::string,int> liveSettings;
int CVarGetInteger(const char* key,int fallback) { auto it=liveSettings.find(key);return it==liveSettings.end()?fallback:it->second; }
'''+body(adapter,'SOH_ReadCrossEntranceOptions')+'\n#define SPDLOG_ERROR(...) ((void)0)\n'+body(adapter,'SOH_SetCrossDungeonSeed')+r'''
using namespace ComboCrossOot;
using namespace ComboRando::CrossDungeon;
int main() {
    PoolOptions options;
    liveSettings["gCombo.Rando.CrossBossEntrances"]=1;
    liveSettings["gCombo.Rando.CrossInteriorEntrances"]=1;
    liveSettings["gCombo.Rando.CrossGrottoEntrances"]=1;
    liveSettings["gCombo.Rando.ExpandedDungeonPool"]=1;
    auto live=ParseOptions(nlohmann::json::parse(SOH_ReadCrossEntranceOptions()));
    assert(!live.dungeons && live.bosses && live.interiors && live.grottos && !live.expanded);
    liveSettings["gCombo.Rando.CrossDungeonEntrances"]=1;
    live=ParseOptions(nlohmann::json::parse(SOH_ReadCrossEntranceOptions()));
    assert(live.dungeons && live.expanded);
    auto legacy=nlohmann::json::parse(CatalogueJson(false,{},options));
    assert(legacy.size()==11);
    for(const auto& d:legacy) assert(!d.contains("pool") && !d.contains("expanded"));
    assert(nlohmann::json::parse(CatalogueJson(true,{},options)).size()==12);
    options.dungeons=false;options.interiors=true;options.grottos=true;
    auto all=nlohmann::json::parse(CatalogueJson(false,{},options));
    size_t expected=0;
    for(const auto& pair:Rando::GetEntranceShuffleTable()) {
        const auto type=pair.first.type;
        const bool selected=type==Rando::EntranceType::Interior || type==Rando::EntranceType::SpecialInterior ||
                            type==Rando::EntranceType::GrottoGrave;
        if(!selected||pair.first.index==ENTR_HAPPY_MASK_SHOP_0) continue;
        ++expected;
        auto it=std::find_if(all.begin(),all.end(),[&](const auto& d){return d.at("entrance").at("index")==pair.first.index;});
        assert(it!=all.end());
        assert(it->at("exit").at("index")==pair.second.index);
        assert(it->at("returnTo").at("index")==pair.second.index);
        assert(it->at("pool")==static_cast<int>(type==Rando::EntranceType::GrottoGrave?Pool::Grotto:Pool::Interior));
    }
    assert(all.size()==expected);
    for(const auto& d:all)assert(d.at("entrance").at("index")!=ENTR_HAPPY_MASK_SHOP_0);
    // Recorded native exit composition must precede pool append, including boss checkpoint metadata.
    const auto first=ParseDungeon(all.at(0));
    std::map<int32_t,int32_t> overrides{{first.exit.index,ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE}};
    options.bosses=true;
    auto mixed=nlohmann::json::parse(CatalogueJson(false,overrides,options));
    assert(mixed.at(0).at("returnTo").at("index")==ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE);
    assert(mixed.size()==expected+8);
    for(const auto& raw:mixed)if(raw.value("pool",0)==static_cast<int>(Pool::Boss)) {
        const auto d=ParseDungeon(raw);
        assert(d.reverseCheckpoint && !d.warpCheckpoint && d.warpReturn && d.escapeExits.size()==1);
    }
    // Native decoupled ingress owns death independently of the shuffled reverse.
    PoolOptions bossOptions;bossOptions.dungeons=false;bossOptions.bosses=true;
    const auto checkRecovery = [&](const std::map<int32_t,int32_t>& native, int source, int target,
                                   int parent, int reverse) {
        const auto raw=nlohmann::json::parse(CatalogueJson(false,native,bossOptions));
        std::vector<Dungeon> bosses;std::vector<size_t> permutation;
        for(const auto& entry:raw) { permutation.push_back(bosses.size());bosses.push_back(ParseDungeon(entry)); }
        std::swap(permutation[source],permutation[target]);
        const auto seed=Encode(bosses,permutation);
        RuntimeState runtime(0);runtime.Install(seed);
        const auto recovery=runtime.RecoveryForBoss(bosses[target].entrance);
        assert(recovery && recovery->parent==Endpoint({0,parent}) &&
               "decoupled death ownership must follow the incoming native override");
        assert(runtime.Lookup(Phase::BeforeNative,bosses[target].exit.index)->to==Endpoint({0,reverse}));
        assert(runtime.Lookup(Phase::BeforeNative,bosses[target].escapeExits[0].index)->to==*bosses[source].warpReturn);
        runtime.Install(seed); // Stable across slot rebinds / repeated recovery.
        assert(runtime.RecoveryForBoss(bosses[target].entrance)->parent==Endpoint({0,parent}));
    };
    checkRecovery({{ENTR_FIRE_TEMPLE_BOSS_ENTRANCE,ENTR_DEKU_TREE_BOSS_ENTRANCE},
                   {ENTR_DEKU_TREE_BOSS_DOOR,ENTR_WATER_TEMPLE_BOSS_DOOR}},
                  0,4,ENTR_FIRE_TEMPLE_ENTRANCE,ENTR_WATER_TEMPLE_BOSS_DOOR);
    // A reverse door may independently become the incoming edge to another boss arena.
    checkRecovery({{ENTR_DEKU_TREE_BOSS_DOOR,ENTR_SHADOW_TEMPLE_BOSS_ENTRANCE},
                   {ENTR_SHADOW_TEMPLE_BOSS_DOOR,ENTR_WATER_TEMPLE_BOSS_DOOR}},
                  7,4,ENTR_DEKU_TREE_ENTRANCE,ENTR_WATER_TEMPLE_BOSS_DOOR);
    // Native mixed/decoupled exits may resolve to the existing portal even though it is not a pool member.
    options={};options.dungeons=false;options.interiors=true;
    auto portalReturn=nlohmann::json::parse(CatalogueJson(false,{{first.exit.index,ENTR_HAPPY_MASK_SHOP_0}},options));
    std::vector<Dungeon> entries;std::vector<size_t> identity;
    bool resolvedPortal=false;
    for(const auto& raw:portalReturn) {
        auto entry=ParseDungeon(raw);
        assert(entry.entrance.index!=ENTR_HAPPY_MASK_SHOP_0);
        resolvedPortal|=entry.returnTo.index==ENTR_HAPPY_MASK_SHOP_0;
        identity.push_back(entries.size());entries.push_back(entry);
    }
    assert(resolvedPortal);
    auto recorded=Encode(entries,identity);
    assert(SOH_SetCrossDungeonSeed(recorded.dump().c_str())==1);
    options={};options.dungeons=false;options.bosses=true;
    assert(nlohmann::json::parse(CatalogueJson(false,{},options)).size()==8);
    std::cout<<"OoT catalogue: legacy11, native Ganon, interiors/grottos with portal exclusion, composed returns, boss metadata pass\n";
}
'''
with tempfile.TemporaryDirectory(prefix='oot-catalogue-') as tmp:
    path=Path(tmp);(path/'test.cpp').write_text(source)
    cmd=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
    for p in ('combo','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include'):
        cmd+=['-I'+str(ROOT/p)]
    cmd+=['-I'+str(ROOT.parent/'build-deps/usr/include')]
    if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
