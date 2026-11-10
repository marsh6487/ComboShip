#!/usr/bin/env python3
"""Compile the actual creation policy and MM native opening adapter.

The renderer, flash I/O and native owl table are service boundaries. The real
adapter must retain native story/form/time; both arrival entrypoints are checked.
"""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    begin = source.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


policy = ROOT / 'combo/vanilla/ComboVanillaPolicy.h'
assert policy.exists(), 'Missing creation and first-cycle policy'
title = (ROOT / 'mm/src/code/title_setup.c').read_text()
adapter = function(title, 'static s32 ComboVanilla_PrepareMMOpening(')
setup = function(title, 'void Setup_InitImpl(')
mm = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
shared = (ROOT / 'mm/2s2h/FleetShipCombo/FleetSharedItems.cpp').read_text()
pull = function(shared, 'static void PullFromPeerWhenReady(')
direct = function(mm, 'extern "C" COMBO_EXPORT void MM_GrantSharedItem(')
loader = function(mm, 'extern "C" int Combo_LoadMMSaveFile(')
forest = (ROOT / 'mm/2s2h/Enhancements/Gameplay/ComboOpeningForest.h').read_text()
normalize = function(forest, 'static inline void ComboOpeningForest_NormalizeEntry(')
souls = (ROOT / 'mm/2s2h/Rando/SoulShuffleShared.cpp').read_text()
soul_apply = function(souls, 'extern "C" void MM_ApplySharedSoulState(')
soul_mask = function(souls, 'static uint32_t Mask(')
soul_grant = function(souls, 'static void GrantNative(')
option_types = (ROOT / 'mm/2s2h/Rando/Types.h').read_text()
option_end = option_types.index('} RandoOptionId;') + len('} RandoOptionId;')
option_begin = option_types.rfind('typedef enum {', 0, option_end)
option_enum = option_types[option_begin:option_end]

choose = (ROOT / 'soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c').read_text()
ui = function(choose, 'void FileChoose_UpdateRandomizerMenu(').replace('this->', 'ui->').replace('FileChooseContext* this =', 'FileChooseContext* ui =').replace('(this,', '(ui,').replace('(this);', '(ui);')
code = r'''
#include <cassert>
#include <iostream>
#include "combo/vanilla/ComboVanillaPolicy.h"
using s32 = int;
static bool vanilla = true;
int Combo_IsVanilla() { return vanilla; }
enum { SOUTH_CLOCK_TOWN=1, CUTSCENE=2, PLAYER_FORM_HUMAN=4, PLAYER_FORM_DEKU=3 };
#define ENTRANCE(scene, spawn) ((scene)*512+(spawn)*16)
#define CLOCK_TIME(h,m) ((h)*60+(m))
struct SceneFlags { int chest=1,switch0=2,switch1=3,clearedRoom=4,collectible=5; };
struct SaveContext {
    struct { int entrance=987,cutsceneIndex=0xFFF3,day=2,time=123,playerForm=PLAYER_FORM_DEKU,isFirstCycle=0,isOwlSave=0;
        struct { int pauseSaveEntrance=-1,saveType=0;
            struct { unsigned randoSaveOptions[RO_MAX]{}; } rando; } shipSaveInfo;
        struct { struct { unsigned threeDayResetCount=0; int magicLevel=2; } playerData;
            SceneFlags permanentSceneFlags[2]; } saveInfo;
    } save;
    int nextCutsceneIndex=0xFFEF,fileNum=0,flashSaveAvailable=0,showTitleCard=0,gameMode=0,cutsceneTrigger=1,respawnFlag=3;
    SceneFlags cycleSceneFlags[2];
} gSaveContext;
static int owlCalls=0;
void Combo_ApplyOwlSaveOpen(int resolve) { ++owlCalls; if(resolve)gSaveContext.save.entrance=777; }
'''
code = code.replace('struct SceneFlags {', option_enum + '\nstruct SceneFlags {', 1)
code += adapter
code += r'''
using uint8_t=unsigned char;
#define COMBO_BUILD
#define CVAR_SETTING(x) x
#define CHECK_BTN_ALL(x,b) (((x)&(b))==(b))
#define CHECK_BTN_ANY(x,b) (((x)&(b))!=0)
#define ABS(x) ((x)<0?-(x):(x))
enum { BTN_A=1,BTN_B=2,BTN_DLEFT=4,BTN_DRIGHT=8, QUEST_OOTXMM=4,
 CBO_OPEN_SETTINGS=3,CBO_LOAD_SEED=2,RSM_OPEN_RANDOMIZER_SETTINGS=2,
 RSM_START_RANDOMIZER=0,RSM_GENERATE_RANDOMIZER=1,CM_SETTINGS_MENU_TO_QUEST=7,
 NA_SE_SY_FSEL_CURSOR=0,NA_SE_SY_FSEL_DECIDE_L=1,NA_SE_SY_OCARINA_ERROR=2 };
struct Input { struct { int button=0; } press; };
struct GameState { Input input[1]; int next=0; };
struct FileChooseContext { GameState state; int questType[3]{2,2,2}; int buttonIndex=0,
 randomizerIndex=0,randomizerUIAlpha=0,randomizerArrowOffset=0,stickRelX=0,configMode=0; };
static bool combined=true,choice=false,seedReady=false,busy=false,generating=false;
static int remember=0;
static int starts=0,presets=0,errors=0,sComboFleetIdx=0;
int gSfxDefaultPos=0,gSfxDefaultFreqAndVolScale=0,gSfxDefaultReverb=0;
int Combo_IsCombinedGame(){return combined;}
int CVarGetInteger(const char* key,int){ return std::string(key)=="gCombo.NewFileVanilla"?choice:remember; }
void CVarSetInteger(const char*,int value){choice=value;}
void FileChoose_UpdateStickDirectionPromptAnim(GameState*){}
void FileChoose_UpdateRandomizer(){}
int FleetComboFS_IsBusy(){return busy;}
void FileChoose_UpdateListMenuAnim(int*,int*){}
int FileChoose_MoveListCursor(FileChooseContext*,int*,void*,int,int){return 0;}
void GameInteractor_ExecuteOnUpdateFileRandomizerOptionSelection(int){}
int FleetComboFS_FleetCount(){return 0;}
void Audio_PlaySoundGeneral(int,int*,int,int*,int*,int*){}
void Audio_PlaySfxGeneral(int,int*,int,int*,int*,int*){}
int Randomizer_IsSeedGenerated(){return seedReady;}
int Randomizer_IsSpoilerLoaded(){return 0;}
void SohFileSelect_ShowPresetModal(){++presets;}
void FileChoose_StartNameEntryFromMenu(FileChooseContext*){++starts;}
void Sfx_PlaySfxCentered(int){++errors;}
void SOH_TriggerComboGenerate(){}
void Randomizer_GenerateRandomizer(){}
void FleetComboFS_LoadSeedIndex(int){}
void Randomizer_ShowRandomizerMenu(){}
void SOH_OpenComboRandoSettings(){}
'''
code += ui
code += r'''
#include "mm/2s2h/Rando/SoulShuffleShared.h"
#include "mm/2s2h/FleetShipCombo/FleetSharedItems.h"
#include "combo/rando/SharedSoulRuntime.h"
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
#define ARRAY_COUNT(a) (int(sizeof(a)/sizeof((a)[0])))
#define STOP_GAMESTATE(s) ((void)(s))
#define SET_NEXT_GAMESTATE(s,fn,size) ((s)->next=(fn))
enum { VB_DELETE_OWL_SAVE=1,Play_Init=1,ConsoleLogo_Init=2 };
struct ConsoleLogoState {};
struct PlayState {};
struct SetupState { GameState state; };
static bool sComboCrossGameTitleCardPending=false,sPullPending=false;
static int gComboStartFileNum=0,gComboOwlBlobSlot=-1,gComboEntryIsResume=0;
static int loadFailure=0,saveTypeOverride=-1,repairs=0,portals=0,blackMarket=0,farore=0,owlDrops=0,saveHooks=0;
SaveContext loadedSave;
void SysFlashrom_InitFlash(){}
void SaveContext_Init(){gSaveContext=SaveContext{};}
void Sram_LoadGlobalOptions(){}
void Setup_InitRegs(){}
void Combo_AdoptOOTGlobalOptions(){}
enum { SAVETYPE_VANILLA=0,SAVETYPE_RANDO=1 };
int SaveManager_LoadSaveFile(int slot){ assert(slot==1); if(loadFailure)return -1;gSaveContext=loadedSave;
 gSaveContext.save.shipSaveInfo.saveType=saveTypeOverride>=0?saveTypeOverride:(vanilla?SAVETYPE_VANILLA:SAVETYPE_RANDO);return 0; }
void SaveManager_MarkNoSaveLoaded(){gSaveContext.fileNum=0xFF;}
int Combo_PeekVanillaMode(int){return vanilla;}
void Combo_RepairMMSaveForSlot(int){++repairs;}
void GameInteractor_ExecuteOnSaveLoad(int){++saveHooks;}
void MM_ApplyComboPortalArrival(){++portals;}
void ComboBlackMarket_ApplyResume(int){++blackMarket;}
int Combo_MM_ConsumeCrossDungeonArrival(){return -1;}
int Combo_MM_ConsumeCrossBossRecoveryResume(){return -1;}
int Combo_MM_ConsumeCrossDungeonRecovery(){return -1;}
int ComboFw_ApplyArrival(){++farore;return 0;}
int GameInteractor_Should(int,bool){return 1;}
void Combo_MMDropOwlSaveBlob(){++owlDrops;}
'''
code += loader
code += setup.replace('this->', 'ui->').replace('SetupState* this)', 'SetupState* ui)')
code += r'''
static PlayState play;
static PlayState* gPlayState=&play;
static int delivered=0,granted=0;
static int receiveDepth=0,soulGrants=0,gMMComboSharedMask=0;
static std::set<std::string> ownedSouls;
static std::string soulSnapshot="{}";
extern "C" void FleetShared_BeginReceive(){++receiveDepth;}
extern "C" void FleetShared_EndReceive(){assert(receiveDepth>0);--receiveDepth;}
using FnExtractSharedState=const char* (*)();
const char* Extract(){return "{}";}
const char* ExtractSouls(){return soulSnapshot.c_str();}
void* Combo_ResolveSym(const char*,const char* symbol){
    return std::string(symbol)=="SOH_ExtractSharedSoulState" ? reinterpret_cast<void*>(&ExtractSouls) :
                                                          reinterpret_cast<void*>(&Extract);
}
void FleetSync_ApplySharedState(const char*){++delivered;}
void FleetShared_RequestPullFromPeer(){sPullPending=true;}
void GrantMmItemByName(const char* name,void*){
    ++granted;
    for(const auto& pair:ComboRando::SharedSoulPairs) if(std::string(name)==pair.mmName){
        assert(receiveDepth>0); ownedSouls.insert(name); ++soulGrants; break;
    }
}
#define IS_RANDO (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO)
#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.randoSaveOptions
'''
code += direct[:direct.index('{')].rstrip() + ';\n'
code += '\nstruct NativeSharedAdapter {\n'
code += 'static ComboRando::GameId Game(){return ComboRando::GAME_MM;}\n'
code += 'static int Slot(){return gSaveContext.fileNum;}\n' + soul_mask + '\n' + soul_grant
code += 'static bool Owned(const std::string& name){return ownedSouls.contains(name); }\n};\n'
code += 'using Runtime=ComboRando::SharedSoulRuntime<NativeSharedAdapter>;\n' + soul_apply + '\n'
code += pull + '\n' + direct
code += r'''
constexpr int ENTR_SCENE_OPENING_DUNGEON=6,CLOCK_TOWER_INTERIOR=7,GAMEMODE_NORMAL=0;
int ComboLostWoods_IsPlayableSlot(int slot){return slot>=0&&slot<=2;}
'''
code += normalize

code += r'''
int main() {
    for (int combined=0; combined<2; ++combined)
    for (int choice=0; choice<2; ++choice)
    for (int seed=0; seed<2; ++seed)
        assert(ComboVanilla_CanStart(combined, choice, seed)==((combined&&choice)||seed));
    for (int mode=0; mode<2; ++mode) for (unsigned resets=0; resets<1000; ++resets)
        assert(ComboVanilla_MMFirstCycle(mode,resets)==(mode&&resets==0));
    gSaveContext=SaveContext{};
    assert(ComboVanilla_PrepareMMOpening(0));
    assert(gSaveContext.save.entrance==ENTRANCE(CUTSCENE,0));
    assert(gSaveContext.save.playerForm==PLAYER_FORM_HUMAN&&gSaveContext.nextCutsceneIndex==0);
    assert(gSaveContext.save.cutsceneIndex==0xFFF3); // Native opening cutscene retained.
    gSaveContext=SaveContext{};gSaveContext.save.isFirstCycle=1;
    assert(ComboVanilla_PrepareMMOpening(0));
    assert(gSaveContext.save.entrance==ENTRANCE(SOUTH_CLOCK_TOWN,0));
    assert(gSaveContext.save.playerForm==PLAYER_FORM_DEKU);
    assert(gSaveContext.save.day==0&&gSaveContext.save.time==CLOCK_TIME(6,0)-1);
    gSaveContext=SaveContext{};
    assert(ComboVanilla_PrepareMMOpening(1));
    assert(owlCalls==1&&gSaveContext.save.entrance==777&&gSaveContext.save.day==2);
    assert(gSaveContext.save.playerForm==PLAYER_FORM_DEKU&&gSaveContext.save.time==123);
    for (bool seeded : {false,true}) {
        gSaveContext=SaveContext{};vanilla=!seeded;
        if (!seeded) gSaveContext.save.saveInfo.playerData.threeDayResetCount=1;
        assert(!ComboVanilla_PrepareMMOpening(0));
        assert(gSaveContext.save.entrance==987&&gSaveContext.save.playerForm==PLAYER_FORM_DEKU);
    }
    FileChooseContext menu; menu.state.input[0].press.button=BTN_A;
    // Combined native file-select uses QUEST_RANDOMIZER; the creation mode must
    // still appear there, rather than only on the legacy QUEST_OOTXMM carousel.
    for (bool solo : {false,true}) for (bool chooseVanilla : {false,true})
    for (bool seed : {false,true}) {
        combined=!solo;choice=chooseVanilla;seedReady=seed;starts=presets=errors=0;
        FileChoose_UpdateRandomizerMenu(&menu.state);
        const bool allowed=(!solo&&chooseVanilla)||seed;
        assert(starts==allowed&&errors==!allowed);
        assert(presets==(allowed&&!(chooseVanilla&&!solo)));
    }
    combined=true;menu.randomizerIndex=4;choice=false;
    FileChoose_UpdateRandomizerMenu(&menu.state);assert(choice);
    FileChoose_UpdateRandomizerMenu(&menu.state);assert(!choice);
    busy=true;FileChoose_UpdateRandomizerMenu(&menu.state);assert(!choice);
    // Execute the real combined boot/resume path around the native adapter.
    busy=false;combined=true;remember=0;
    for (bool mode : {false,true}) for (int resets : {0,1})
    for (bool owl : {false,true}) for (bool resume : {false,true}) {
        vanilla=mode;loadedSave=SaveContext{};
        loadedSave.save.isFirstCycle=1;loadedSave.save.isOwlSave=owl;
        loadedSave.save.saveInfo.playerData.threeDayResetCount=resets;
        gComboStartFileNum=0;gComboOwlBlobSlot=owl?1:-1;gComboEntryIsResume=resume;
        portals=blackMarket=farore=owlCalls=owlDrops=saveHooks=0;
        SetupState state;Setup_InitImpl(&state);
        assert(state.state.next==Play_Init&&gComboStartFileNum==-1&&saveHooks==1);
        assert(portals==1); // Consumes even a quarantined portal route.
        if (mode&&resets==0) {
            assert(blackMarket==0&&farore==0&&!sComboCrossGameTitleCardPending);
            assert(gSaveContext.save.playerForm==PLAYER_FORM_DEKU);
            assert(gSaveContext.save.cutsceneIndex==0xFFF3);
            assert(gSaveContext.save.entrance==(owl?777:ENTRANCE(SOUTH_CLOCK_TOWN,0)));
        } else {
            assert(blackMarket==1&&farore==1&&gSaveContext.save.cutsceneIndex==0);
            assert(gSaveContext.save.entrance==(owl&&resume?777:ENTRANCE(SOUTH_CLOCK_TOWN,0)));
        }
        assert(owlDrops==owl);
        assert(gSaveContext.cycleSceneFlags[0].switch1==3);
    }
    vanilla=true;loadedSave=SaveContext{};gComboStartFileNum=0;gComboOwlBlobSlot=-1;
    SetupState fresh;Setup_InitImpl(&fresh);
    assert(gSaveContext.save.entrance==ENTRANCE(CUTSCENE,0)&&gSaveContext.nextCutsceneIndex==0);
    for (int expectedMode : {0,1}) {
        vanilla=expectedMode;saveTypeOverride=expectedMode?SAVETYPE_RANDO:SAVETYPE_VANILLA;
        assert(Combo_LoadMMSaveFile(1)==-6&&gSaveContext.fileNum==0xFF);
    }
    saveTypeOverride=-1;vanilla=true;
    loadFailure=1;repairs=0;gComboStartFileNum=0;
    SetupState failed;Setup_InitImpl(&failed);
    assert(failed.state.next==ConsoleLogo_Init&&repairs==0&&gComboStartFileNum==-1);
    // No direct or snapshot grants before reset; the request survives and drains
    // exactly once on the first safe tick after MM's native reset counter changes.
    vanilla=true;gSaveContext=SaveContext{};sPullPending=true;
    for(int i=0;i<10;++i){PullFromPeerWhenReady();MM_GrantSharedItem("Lantern");}
    assert(sPullPending&&delivered==0&&granted==0);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount=1;
    PullFromPeerWhenReady();PullFromPeerWhenReady();
    assert(!sPullPending&&delivered==1);
    MM_GrantSharedItem("Lantern");assert(granted==0&&sPullPending);
    PullFromPeerWhenReady();PullFromPeerWhenReady();
    assert(!sPullPending&&delivered==2&&granted==0);
    // Seeded files retain the existing native name-grant boundary, including
    // before their cycle counter changes. Vanilla always uses the NEI profile.
    vanilla=false;gSaveContext.save.saveInfo.playerData.threeDayResetCount=0;
    MM_GrantSharedItem("Lantern");assert(granted==1);
    for (bool mode : {false,true}) for (int resets : {0,1}) {
        vanilla=mode;gSaveContext=SaveContext{};
        gSaveContext.save.saveInfo.playerData.threeDayResetCount=resets;
        gSaveContext.save.entrance=ENTR_SCENE_OPENING_DUNGEON<<9;
        ComboOpeningForest_NormalizeEntry();
        if(mode&&resets==0) {
            assert(gSaveContext.save.entrance==(ENTR_SCENE_OPENING_DUNGEON<<9));
            assert(gSaveContext.save.cutsceneIndex==0xFFF3&&gSaveContext.nextCutsceneIndex==0xFFEF);
            assert(gSaveContext.cutsceneTrigger==1&&gSaveContext.respawnFlag==3);
        } else {
            assert(gSaveContext.save.entrance==ENTRANCE(CLOCK_TOWER_INTERIOR,0));
            assert(gSaveContext.save.cutsceneIndex==0&&gSaveContext.cutsceneTrigger==0);
        }
    }
    assert(soulGrants==0 && ownedSouls.empty() && receiveDepth==0);
    // Actual shared-soul wrapper/runtime and native saved-mask predicate. The
    // existing native grant boundary records the exact target under receive guard.
    const auto* pair=&ComboRando::SharedSoulPairs[0];
    for(const auto& candidate:ComboRando::SharedSoulPairs) if(candidate.bit==ComboRando::SS_NPC){pair=&candidate;break;}
    gSaveContext=SaveContext{};gSaveContext.fileNum=0;
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;gMMComboSharedMask=ComboRando::SS_NPC;
    soulSnapshot=nlohmann::json{{"slot",0},{"mask",uint32_t(ComboRando::SS_NPC)},{"owned",{pair->ootName}}}.dump();
    for(int blocked=0;blocked<4;++blocked){
        vanilla=blocked==0;combined=blocked!=1;
        gSaveContext.save.saveInfo.playerData.threeDayResetCount=blocked==0?0:1;
        gSaveContext.fileNum=blocked==2?3:0;
        RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=blocked==3?0:1;
        sPullPending=true;PullFromPeerWhenReady();assert(soulGrants==0&&receiveDepth==0);
    }
    vanilla=false;combined=true;gSaveContext.fileNum=0;
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;
    sPullPending=true;PullFromPeerWhenReady();
    assert(soulGrants>0&&ownedSouls.contains(pair->mmName)&&receiveDepth==0);
    const int once=soulGrants;sPullPending=true;PullFromPeerWhenReady();assert(soulGrants==once);
    std::cout << "PASS actual native Start/mode selection and seed guards, combined MM boot/resume and intro/Deku/owl state, failed-load refusal, deferred sharing, seeded/solo controls\n";
}
'''
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
if '--sanitize' in sys.argv:
    flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='combo-vanilla-opening-') as tmp:
    cpp = Path(tmp) / 'opening.cpp'; cpp.write_text(code)
    exe = Path(tmp) / 'opening'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

# Compile both real DLL mode shims: inspecting another slot must never activate
# its identity or change the active file's native/rando dispatch.
for path in ('soh/soh/OTRGlobals.cpp', 'mm/2s2h/BenPort.cpp'):
    source = (ROOT / path).read_text()
    shims = '\n'.join(function(source, name) for name in
                      ('extern "C" int Combo_PeekVanillaMode(', 'extern "C" int Combo_QueryVanillaMode('))
    query = r'''
#include <cassert>
#include <iostream>
#define COMBO_BUILD
#include "combo/vanilla/ComboVanillaMode.h"
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; int gComboVanillaMode=0; }
static int Mode(int slot) { return slot==0?1:slot==1?0:-1; }
static int (*sComboVanillaModeQuery)(int)=Mode;
'''
    query += shims
    query += r'''
int main() {
    assert(Combo_QueryVanillaMode(0)==1&&Combo_IsVanilla());
    assert(Combo_PeekVanillaMode(1)==0&&Combo_IsVanilla());
    assert(Combo_PeekVanillaMode(99)==-1&&Combo_IsVanilla());
    assert(Combo_QueryVanillaMode(1)==0&&!Combo_IsVanilla());
    assert(Combo_PeekVanillaMode(0)==1&&!Combo_IsVanilla());
    gComboGameMode=COMBO_GAME_OOT;
    assert(Combo_PeekVanillaMode(0)==0&&!Combo_IsVanilla());
    std::cout << "PASS real DLL slot previews retain the active file mode and solo guard\n";
}
'''
    with tempfile.TemporaryDirectory(prefix='combo-vanilla-preview-') as tmp:
        cpp = Path(tmp) / 'query.cpp'; cpp.write_text(query)
        exe = Path(tmp) / 'query'
        subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
