#!/usr/bin/env python3
"""Compile the two production seasonal selectors; exercise ownership and a full cycle."""
from pathlib import Path
import re, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[2]
def function(source, name):
    found = re.search(r'^(?:extern "C" )?(?:static )?(?:inline )?(?:s32|void|bool|const char\*|Gfx\*) '+name+r'\([^;{}]*\)\s*\{',source,re.M)
    assert found, name
    end=found.end();depth=1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}');end+=1
    return source[found.start():end]
for game in ('soh','mm'):
    source=(ROOT/game/'mods/pak_loader/pak_loader.cpp').read_text()
    native='\n'.join(function(source,n) for n in ('sBodySelectionEnabled','sGetActiveIndex','PakLoader_UpdateSeasonalSelection','PakLoader_GetSeasonalSkeletonPath','PakLoader_BeginDrawFrame'))
    frame=function(source,'PakLoader_FrameBegin')
    native += '\n' + frame[:frame.index('    CleanupRuntimeDLs();') + len('    CleanupRuntimeDLs();')] + '\n}'

    fixture=r'''
#include "combo/actors/SeasonalOutfits.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
using s32=int; using u8=unsigned char;
struct SkeletonHeader { void** segment; int limbCount; };
struct FlexSkeletonHeader { SkeletonHeader sh; int dListCount; };
struct PakModel { int source=1; bool isEquipmentOnly=false; char o2rAdultSkelOtr[160]{},o2rChildSkelOtr[160]{}; bool o2rArchiveMounted=false; FlexSkeletonHeader* o2rAdultSkel=nullptr; FlexSkeletonHeader* o2rChildSkel=nullptr; };
constexpr int PAK_SOURCE_O2R=1,YEARS_ADULT=17;
int age=17,season=0,owned=1; bool enabled=false,manualEnabled=false,remote=false; std::string set="din";
#define LINK_AGE_IN_YEARS age
int AdultLink_IsActive(){return age==YEARS_ADULT;}
struct PlayState { struct { uint32_t frames=0; } state; };
PlayState play; PlayState* gPlayState=&play;
SeasonalOutfits::DrawFrameGate sDrawFrameGate;
int CVarGetInteger(const char* key,int){return !std::strcmp(key,SEASONAL_OUTFITS_CVAR)?enabled:manualEnabled;}
const char* CVarGetString(const char*,const char*){return set.c_str();}
int Seasons_GetSeason(){return season;} int Seasons_SeasonCount(){return owned;}
bool PakLoader_IsRemoteRenderActive(){return remote;}
std::vector<PakModel> sModels(12);
int sSeasonalModelIndex=-1,sForcedModelIndex=-1,sSelectedAdultIndex=9,sSelectedChildIndex=10,mounts=0;
bool sInitialized=true; int cleanups=0;
void PakLoader_Init(){sInitialized=true;}
void CleanupRuntimeDLs(){++cleanups;}
bool MountO2rArchive(PakModel&m){if(!m.o2rArchiveMounted){m.o2rArchiveMounted=true;++mounts;}return true;}
void LazyResolveO2rSkel(PakModel&){}
bool IsValidLinkSkel(SkeletonHeader*h){return h&&h->segment&&h->limbCount>0&&h->limbCount<=32;}
/* NATIVE */
int main(){
    assert(PakLoader_BeginDrawFrame());
    assert(!PakLoader_BeginDrawFrame() && !PakLoader_BeginDrawFrame());
    ++play.state.frames; assert(PakLoader_BeginDrawFrame());
    PlayState otherPlay; otherPlay.state.frames=play.state.frames; gPlayState=&otherPlay;
    assert(PakLoader_BeginDrawFrame()); gPlayState=&play; assert(PakLoader_BeginDrawFrame());
    sDrawFrameGate={}; cleanups=0;
    PakLoader_FrameBegin(); PakLoader_FrameBegin(); PakLoader_FrameBegin(); assert(cleanups==1);
    ++play.state.frames; PakLoader_FrameBegin(); assert(cleanups==2);
    assert(SeasonalOutfits::PrivateArchive("model.private.o2r"));
    assert(!SeasonalOutfits::PrivateArchive("normal.o2r"));
    assert(!SeasonalOutfits::PrivateArchive("model.private.o2r.bad"));
    for(const char* name:{"din","vanilla-link","my_outfits9","PublicSet"})assert(SeasonalOutfits::ValidSet(name));
    for(const char* name:{"","../din","a/b","foo:bar","foo bar","a.b"})assert(!SeasonalOutfits::ValidSet(name));
    assert(!SeasonalOutfits::ValidSet(std::string(64,'a').c_str()));
    int dummy=0; void* segments[]={&dummy}; FlexSkeletonHeader skeleton{{segments,21},18};
    for(int i=0;i<8;++i){auto path=SeasonalOutfits::Skeleton("din",i<4,i%4);std::strcpy(i<4?sModels[i].o2rAdultSkelOtr:sModels[i].o2rChildSkelOtr,path.c_str());if(i<4)sModels[i].o2rAdultSkel=&skeleton;else sModels[i].o2rChildSkel=&skeleton;std::string family;assert(SeasonalOutfits::Season(path.c_str(),i<4,&family)==i%4&&family=="din");}
    auto other=SeasonalOutfits::Skeleton("public-pack",true,3);std::strcpy(sModels[8].o2rAdultSkelOtr,other.c_str());sModels[8].o2rAdultSkel=&skeleton;
    assert(SeasonalOutfits::Season("__OTR__objects/object_link_boy/gLinkAdultSkel",true)==-1);
    assert(SeasonalOutfits::Season("__OTR__objects/seasonal_outfits/din/adult/spring/gLinkAdultSkel/extra",true)==-1);
    PakLoader_UpdateSeasonalSelection();assert(sSeasonalModelIndex==-1&&sGetActiveIndex()==9&&!sBodySelectionEnabled());
    enabled=true;
    for(int i:{0,1,2,3,0}){season=i;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==i&&sBodySelectionEnabled());assert(std::string(PakLoader_GetSeasonalSkeletonPath())==SeasonalOutfits::Skeleton("din",true,i));}
    assert(mounts==4); // Returning to Spring never remounts or unloads resources.
    age=9;season=2;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==6);
    sModels[6].o2rChildSkel=nullptr;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==10&&!sBodySelectionEnabled());
    age=17;set="public-pack";season=3;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==8);
    season=0;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==9&&PakLoader_GetSeasonalSkeletonPath()==nullptr);
    set="../din";PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==9);
    set="din";season=4;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==9);
    season=0;owned=0;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==9);
    owned=1;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==0);
    sForcedModelIndex=11;assert(sGetActiveIndex()==11&&PakLoader_GetSeasonalSkeletonPath()==nullptr);sForcedModelIndex=-1;
    remote=true;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==9&&!sBodySelectionEnabled()&&PakLoader_GetSeasonalSkeletonPath()==nullptr);remote=false;
    enabled=false;PakLoader_UpdateSeasonalSelection();assert(sGetActiveIndex()==9&&!sBodySelectionEnabled());
    manualEnabled=true;assert(sBodySelectionEnabled());
    std::puts("PASS production seasonal router: cycles, sets, age, Off/missing, forced/remote priority, private mounts");
}
'''.replace('/* NATIVE */',native)
    with tempfile.TemporaryDirectory(prefix='seasonal-outfits-') as directory:
        p=Path(directory);(p/'test.cpp').write_text(fixture)
        subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie','-I',str(ROOT),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
        subprocess.run([str(p/'test')],check=True)
    assert 'SeasonWeather_' not in function(source,'PakLoader_UpdateSeasonalSelection')
    assert 'MMWeather_' not in function(source,'PakLoader_UpdateSeasonalSelection')
    assert 'if (!privateBodyFile && archiveManager)' in source
print('PASS source private discovery and weather-independent gameplay-season contract')

with tempfile.TemporaryDirectory(prefix='seasonal-equipment-oot-') as directory:
    path = Path(directory)
    loader = (ROOT / 'soh/mods/pak_loader/pak_loader.cpp').read_text()
    player = (ROOT / 'soh/src/code/z_player_lib.c').read_text()
    native = function(loader, 'PakLoader_GetSeasonalDLOverride')
    native += '\n' + function(player, 'Player_ResolveLimbDLForDummyOrLocal')
    (path / 'test.cpp').write_text(r'''
#include <cassert>
#include <cstring>
#include <string>
struct Gfx { int marker; };
Gfx stock{1}, outfit{2}, form{3}, remote{4};
bool selected=true, available=true, customForm=false, peer=false;
std::string requested;
const char* PakLoader_GetSeasonalSkeletonPath() {
    return selected && !peer ? "__OTR__objects/seasonal_outfits/din/adult/winter/gLinkAdultSkel" : nullptr;
}
bool ResourceMgr_FileExists(const char* path) {
    requested=path;
    return available;
}
Gfx* ResourceMgr_LoadGfxByName(const void* path) {
    if (path == nullptr)
        return nullptr;
    return !std::strncmp(static_cast<const char*>(path), "__OTR__objects/seasonal_outfits/", 32) ? &outfit : &stock;
}
void* MmForm_KafeiWhistleHandDL(const char*) { return nullptr; }
void* HarpoonSkinSync_ResolvePlayerLimbDL(const char*) { return peer ? &remote : nullptr; }
void* CustomForms_ResolveVanillaResource(const char*) { return customForm ? &form : nullptr; }
int SwordAura_ResourceModel(const char*, const Gfx*, size_t) { return -1; }
size_t ResourceGetSizeByName(const char*) { return sizeof(Gfx); }
const void* ResourceGetDataByName(const char*) { return nullptr; }
void DinFireSword_SetHandModel(void*, const Gfx*, int) {}
/* NATIVE */
int main() {
    char sword[] = "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingBgsNearDL";
    char shield[] = "__OTR__objects/object_link_boy/gLinkAdultRightHandHoldingHylianShieldFarDL";
    char aim[] = "__OTR__alt/objects/object_link_boy/gLinkAdultRightHandHoldingBowFirstPersonDL";
    char gi[] = "__OTR__objects/object_gi_sword/gGiSwordDL";
    assert(Player_ResolveLimbDLForDummyOrLocal(sword) == &outfit);
    assert(requested == "__OTR__objects/seasonal_outfits/din/adult/winter/gLinkAdultLeftHandHoldingBgsNearDL");
    assert(Player_ResolveLimbDLForDummyOrLocal(shield) == &outfit);
    assert(PakLoader_GetSeasonalDLOverride(aim) == &outfit);
    assert(requested == "__OTR__objects/seasonal_outfits/din/adult/winter/gLinkAdultRightHandHoldingBowFirstPersonDL");
    assert(Player_ResolveLimbDLForDummyOrLocal(gi) == &stock);
    available=false;
    assert(Player_ResolveLimbDLForDummyOrLocal(sword) == &stock);
    available=true;selected=false;
    assert(Player_ResolveLimbDLForDummyOrLocal(sword) == &stock);
    selected=true;customForm=true;
    assert(Player_ResolveLimbDLForDummyOrLocal(sword) == &form);
    peer=true;
    assert(Player_ResolveLimbDLForDummyOrLocal(sword) == &remote);
    assert(PakLoader_GetSeasonalDLOverride(aim) == nullptr);
}
'''.replace('/* NATIVE */', native))
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', str(path / 'test.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS seasonal OoT gameplay and aiming aliases, ownership priority and GI isolation')

with tempfile.TemporaryDirectory(prefix='seasonal-outfit-config-') as directory:
    path = Path(directory)
    context = path / 'ship/Context.h'
    context.parent.mkdir()
    context.write_text(r'''
#pragma once
#include "ship/config/ConsoleVariable.h"
namespace Ship {
class Context {
  public:
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    std::shared_ptr<ConsoleVariable> GetConsoleVariables() const { return variables; }
  private:
    std::shared_ptr<ConsoleVariable> variables = std::make_shared<ConsoleVariable>();
};
}
''')
    (path / 'test.cpp').write_text(r'''
#include "combo/actors/SeasonalOutfitsConfig.h"
#include <cassert>
#include <map>

static std::map<std::string, std::string> cvars;
namespace Ship {
ConsoleVariable::ConsoleVariable() = default;
ConsoleVariable::~ConsoleVariable() = default;
std::shared_ptr<CVar> ConsoleVariable::Get(const char* key) {
    return cvars.contains(key) ? std::make_shared<CVar>() : nullptr;
}
void ConsoleVariable::CopyVariable(const char* from, const char* to) { cvars[to] = cvars.at(from); }
void ConsoleVariable::ClearVariable(const char* key) { cvars.erase(key); }
}

int main() {
    const char* oldSet = "gEnhancements.SeasonalOutfits.Set";
    const char* oldIndex = "gEnhancements.SeasonalOutfits.SetIndex";
    const std::string oldPrefix = std::string(SEASONAL_OUTFITS_CVAR) + ".";
    cvars = {{SEASONAL_OUTFITS_CVAR, "1"}, {oldSet, "custom-set"}, {oldIndex, "5"}};
    SeasonalOutfits::MigrateOutfitSetConfig();
    assert(cvars.at(SEASONAL_OUTFITS_CVAR) == "1");
    assert(cvars.at(SEASONAL_OUTFITS_SET_CVAR) == "custom-set");
    assert(!CVarExists(oldSet) && !CVarExists(oldIndex));
    assert(std::string(SEASONAL_OUTFITS_INDEX_CVAR).rfind(oldPrefix, 0) != 0);
    assert(std::string(SEASONAL_OUTFITS_SET_CVAR).rfind(oldPrefix, 0) != 0);
    cvars[oldSet] = "old-choice";
    SeasonalOutfits::MigrateOutfitSetConfig();
    SeasonalOutfits::MigrateOutfitSetConfig();
    assert(cvars.at(SEASONAL_OUTFITS_SET_CVAR) == "custom-set");
    assert(!CVarExists(oldSet));
}
''')
    bridge = ROOT / 'libultraship/src/libultraship/bridge/consolevariablebridge.cpp'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-ffunction-sections',
                    '-Wl,--gc-sections', '-I', str(path), '-I', str(ROOT), '-I', str(ROOT / 'libultraship/include'),
                    str(path / 'test.cpp'), str(bridge), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS seasonal configuration migration and non-conflicting persistence keys')
