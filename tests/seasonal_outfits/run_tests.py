#!/usr/bin/env python3
"""Compile the two production seasonal selectors; exercise ownership and a full cycle."""
from pathlib import Path
import re, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[2]
def function(source, name):
    found = re.search(r'^(?:extern "C" )?(?:static )?(?:inline )?(?:s32|void|bool|const char\*) '+name+r'\([^;{}]*\)\s*\{',source,re.M)
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
