#!/usr/bin/env python3
"""Compile real Saria eligibility and cross-hint composer with narrow native boundaries."""
from pathlib import Path
import os,subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
actor=(ROOT/'soh/src/overlays/actors/ovl_En_Sa/z_en_sa.c').read_text()
body=block_from(actor,actor.index('s32 func_80AF5DFC('))
action=block_from(actor,actor.index('void func_80AF6448(EnSa* this, PlayState* play) {'))
idle=block_from(actor,actor.index('void func_80AF67D0(EnSa* this, PlayState* play) {'))
prefix=r'''
#include <cassert>
using s32=int;
struct PlayState{int sceneNum;};
struct EnSa{struct{int talkState;}interactInfo;struct{int textId;}actor;struct{void* animation;float playSpeed,curFrame;}skelAnime;int unk_208,unk_20B,mouthIndex;void(*actionFunc)(EnSa*,PlayState*);};
struct {int cutsceneIndex;} gSaveContext;
enum{SCENE_KOKIRI_FOREST,SCENE_SACRED_FOREST_MEADOW,SCENE_SARIAS_HOUSE};
enum{EVENTCHKINF_OBTAINED_ZELDAS_LETTER,QUEST_SONG_SARIA,QUEST_KOKIRI_EMERALD,INFTABLE_GREETED_BY_SARIA,VB_BE_ELIGIBLE_FOR_SARIAS_SONG,VB_NOT_BE_GREETED_BY_SARIA,ITEM_OCARINA_FAIRY,RSK_SARIA_HINT};
bool adult,rando,letter,song,emerald,hint,learned;
enum{VB_SARIA_GESTURE=99,NPC_TALK_STATE_IDLE=0,INFTABLE_01=1,INFTABLE_03=3,ENSA_ANIM1_6=6,ANIMMODE_ONCE=0};
int gSariaHandsBehindBackWaitAnim,gSariaStopPlayingOcarinaAnim;
void func_80AF67D0(EnSa*,PlayState*);void func_80AF6448(EnSa*,PlayState*);
void func_80AF5CD4(EnSa*,int){}void func_80AF5CE4(EnSa*){}void EnSa_ChangeAnim(EnSa*,int){}
void Animation_Change(decltype(EnSa::skelAnime)*,void*,float,float,float,int,float){}
#define LINK_IS_ADULT adult
#define IS_RANDO rando
#define CHECK_QUEST_ITEM(x) ((x)==QUEST_SONG_SARIA?song:emerald)
#define INV_CONTENT(x) 0
bool Flags_GetEventChkInf(int){return letter;}
bool Flags_GetInfTable(int){return true;}
bool Randomizer_GetSettingValue(int){return hint;}
bool GameInteractor_Should(int k,bool v,...){return rando&&k==VB_BE_ELIGIBLE_FOR_SARIAS_SONG?!learned:v;}
'''
main=r'''
int main(){EnSa sa{};PlayState p{SCENE_SACRED_FOREST_MEADOW};
assert(func_80AF5DFC(&sa,&p)==0);rando=true;hint=true;
assert(func_80AF5DFC(&sa,&p)==2); // hint only: no song event/reward
sa.interactInfo.talkState=1;func_80AF6448(&sa,&p);assert(sa.actionFunc==func_80AF67D0&&!learned&&!song);
sa.interactInfo.talkState=0;func_80AF67D0(&sa,&p);assert(sa.actionFunc==func_80AF6448&&!learned&&!song);
adult=true;assert(func_80AF5DFC(&sa,&p)==0);adult=false;
hint=false;assert(func_80AF5DFC(&sa,&p)==0);
letter=true;assert(func_80AF5DFC(&sa,&p)==5);
song=true;assert(func_80AF5DFC(&sa,&p)==5); // randomizer check remains independent of inventory
learned=true;assert(func_80AF5DFC(&sa,&p)==2);
rando=false;assert(func_80AF5DFC(&sa,&p)==2);song=false;assert(func_80AF5DFC(&sa,&p)==5);
letter=false;hint=true;assert(func_80AF5DFC(&sa,&p)==0);
gSaveContext.cutsceneIndex=0xFFF0;assert(func_80AF5DFC(&sa,&p)==5);
}
'''
with tempfile.TemporaryDirectory(prefix='saria-magic-') as tmp:
 p=Path(tmp);(p/'actor.cpp').write_text(prefix+(body+action+idle).replace('this', 'self')+main)
 subprocess.run(['g++','-std=c++20',str(p/'actor.cpp'),'-o',str(p/'actor')],check=True)
 subprocess.run([str(p/'actor')],check=True)
 print('PASS: native Saria hint-only eligibility and progression guards')
 cpp=ROOT/'tests/saria_magic/hints_test.cpp'
 if cpp.exists():
  subprocess.run(['g++','-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'combo'),'-I'+os.environ.get('NLOHMANN_JSON_INCLUDE','/usr/include'),str(cpp),'-o',str(p/'hints')],check=True)
  subprocess.run([str(p/'hints')],check=True)
 native=(ROOT/'soh/soh/Enhancements/randomizer/3drando/hints.cpp').read_text()
 cache=block_from(native,native.index('void CaptureSariaMagicHintLocation('))
 cache_prefix=r'''#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
using RandomizerCheck=int;
constexpr int RC_UNKNOWN_CHECK=-1,RG_MAGIC_STAT_UPGRADE=7;
struct Location{std::string name;int item;int GetPlacedRandomizerGet(){return item;}std::string GetName(){return name;}};
Location locs[]={{"A late",7},{"Z early",7},{"B early",7},{"Other",1}};
struct Context{unsigned mFinalSeed=1;void SetSeed(uint32_t);Location* GetItemLocation(int i){return &locs[i];}unsigned GetSeed(){return mFinalSeed;}static Context* GetInstance(){static Context c;return &c;}};
namespace Rando{using Context=::Context;}
namespace StaticData{Location* GetLocation(int i){return &locs[i];}}
static int sEarlySariaMagicCheck=-1;
static uint32_t sEarlySariaMagicSeed=0;
static bool sSariaMagicReachabilityVerified=false;
'''
 cache_main=r'''int main(){
 CaptureSariaMagicHintLocation({{1,2},{0}});assert(sEarlySariaMagicCheck==2); // full, before optional-copy pruning
 Context::GetInstance()->SetSeed(1);assert(sEarlySariaMagicCheck==-1); // import same raw seed, old check still stat
 CaptureSariaMagicHintLocation({{1,2},{0}});
 CaptureSariaMagicHintLocation({});assert(sEarlySariaMagicCheck==-1);
 Context::GetInstance()->SetSeed(2);CaptureSariaMagicHintLocation({{0},{2}});assert(sEarlySariaMagicCheck==0&&sEarlySariaMagicSeed==2);
 locs[0].item=1;locs[1].item=1;locs[2].item=1;
 CaptureSariaMagicHintLocation({{0,1,2,3}});assert(sEarlySariaMagicCheck==-1);
}'''
 ctx_source=(ROOT/'soh/soh/Enhancements/randomizer/SeedContext.cpp').read_text()
 setter=block_from(ctx_source,ctx_source.index('void Context::SetSeed('))
 invalidate=block_from(native,native.index('void InvalidateSariaMagicHintLocation('))
 (p/'cache.cpp').write_text(cache_prefix+invalidate+cache+setter+cache_main)
 subprocess.run(['g++','-std=c++20',str(p/'cache.cpp'),'-o',str(p/'cache')],check=True)
 subprocess.run([str(p/'cache')],check=True)
 fill=(ROOT/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text()
 playthrough=block_from(fill,fill.index('std::vector<RandomizerCheck> GeneratePlaythrough('))
 assert 'InvalidateSariaMagicHintLocation()' in playthrough
 assert 'CaptureSariaMagicHintLocation(sariaMagicSpheres)' in playthrough
 generation=(ROOT/'tests/saria_magic/native_spheres_test.cpp').read_text().replace('/* GENERATE_PLAYTHROUGH */',playthrough)
 (p/'generation.cpp').write_text(generation)
 subprocess.run(['g++','-std=c++20',str(p/'generation.cpp'),'-o',str(p/'generation')],check=True)
 subprocess.run([str(p/'generation')],check=True)
 print('PASS: native full-sphere capture, successive seeds, empty reset and mode change')
 settings=(ROOT/'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
 callback=settings[settings.index('OPT_CALLBACK(RSK_MAGIC_STAT_UPGRADE,'):settings.index('OPT_BOOL(RSK_DEFENSE_UPGRADE_ADJUSTABLE,')]
 assert 'CVarSetInteger(CVAR_RANDOMIZER_SETTING("SariaHint"), 0)' not in callback
 assert 'mOptions[RSK_SARIA_HINT].Disable' not in callback
 print('PASS: magic stat setting preserves Saria hint preference')
