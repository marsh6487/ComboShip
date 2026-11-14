#!/usr/bin/env python3
"""Execute OoT's real Sram_InitSave with native/service boundaries controlled."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'soh/src/code/z_sram.c').read_text()
start = source.index('void Sram_InitSave(')
begin = source.index('{', start)
depth, end = 1, begin + 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
production = source[start:end]
code = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
using u8 = uint8_t; using u16 = uint16_t;
#define COMBO_BUILD
#define CVAR_GENERAL(x) x
constexpr int QUEST_NORMAL=0, QUEST_MASTER=1, QUEST_RANDOMIZER=2, QUEST_OOTXMM=3;
constexpr int ENTR_LINKS_HOUSE_CHILD_SPAWN=0, GAME_REGION_PAL=1, LANGUAGE_JPN=0;
constexpr int NAME_LANGUAGE_PAL=0, NAME_LANGUAGE_NTSC_JPN=1, NAME_LANGUAGE_NTSC_ENG=2;
struct SaveContext {
    int entranceIndex=0,linkAge=0,dayTime=0,cutsceneIndex=0,language=1,n64ddFlag=0,fileNum=0,health=77;
    uint8_t playerName[8]{};
    struct { int filenameLanguage=0; struct { int id=QUEST_NORMAL; } quest; } ship;
} gSaveContext;
struct SaveFileMetaInfo { bool valid=false; uint8_t playerName[8]{}; } metadata;
struct FileChooseContext { int buttonIndex=0,questType[3]{QUEST_RANDOMIZER},n64ddFlag=0,speedrunIndex=0; int prevConfigMode=99,configMode=99,nameBoxAlpha[3]{},nameAlpha[3]{},connectorAlpha[3]{}; } file;
static bool combined=true, vanilla=true, seed=false, beginOk=true, finishOk=true;
static int initCount=0,randoCount=0,saveCount=0,legacyCount=0,beginCount=0,endCount=0,abortCount=0;
// The form-session suite executes the cleanup body. This creation fixture owns
// its C-ABI handoff and whether accepted/rejected native creation reaches it.
static int sessionDiscardCount=0;
extern "C" void MmForm_DiscardSaveSession(void) { ++sessionDiscardCount; }
int gComboVanillaMode=0,gComboVanillaCreationFailed=0;
static int Creation(int slot,int phase) {
    assert(slot==0);
    if(phase==0) { ++beginCount; return beginOk; }
    if(phase==1) { ++endCount; return finishOk; }
    assert(phase==2); ++abortCount; return 1;
}
int (*gComboVanillaCreateCallback)(int,int)=Creation;
static void Legacy(int) {++legacyCount;}
void (*gComboSaveInitCallback)(int)=Legacy;
void (*gComboGenerateCallback)(int)=nullptr;
int Combo_IsCombinedGame(){return combined;}
int CVarGetInteger(const char*,int){return vanilla;}
int Ship_QuestDebugEnabled(int){return false;}
void Sram_InitNewSave(){assert(sessionDiscardCount==1);++initCount;gSaveContext.health=48;}
void Sram_InitDebugSave(){assert(false);}
int ResourceMgr_GetGameRegion(int){return GAME_REGION_PAL;}
SaveFileMetaInfo* Save_GetSaveMetaInfo(int){return &metadata;}
int Randomizer_IsSeedGenerated(){return seed;}
int Randomizer_IsSpoilerLoaded(){return false;}
void Randomizer_InitSaveFile(){++randoCount;}
void FleetComboFS_OnCreateSave(int){assert(false);}
#define IS_SPEEDRUN false
void Speedrun_InitSaveFile(int){assert(false);}
void Save_SaveFile(){++saveCount;metadata.valid=true;}
void SaveManager_ThreadPoolWait(){}
'''
code += production
code += "\nconstexpr int CM_MAIN_MENU=0, CM_NAME_ENTRY_TO_MAIN=1, NA_SE_SY_FSEL_ERROR=1;\nstatic int rumbles=0, gSfxDefaultPos=0,gSfxDefaultFreqAndVolScale=0,gSfxDefaultReverb=0;\nvoid CVarSetInteger(const char*,int){}\nvoid Rumble_Request(float,int,int,int){++rumbles;}\nvoid Audio_PlaySfxGeneral(int,int*,int,int*,int*,int*){}\n"
for region in ('PAL','NES'):
    ui_source=(ROOT / ('soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_'+region+'.c')).read_text()
    ui_start=ui_source.index('Sram_InitSave(this);')
    ui_end=ui_source.index(';',ui_source.index('Rumble_Request(',ui_start))+1
    commit=ui_source[ui_start:ui_end].replace('this->','ui->').replace('Sram_InitSave(this)','Sram_InitSave(ui)')
    code += '\nvoid Commit'+region+'(FileChooseContext* ui) { int dayTime=gSaveContext.dayTime;\n'+commit+'\n}\n'

code += r'''
static void Reset() {
    gSaveContext=SaveContext{}; metadata=SaveFileMetaInfo{}; file=FileChooseContext{};
    initCount=randoCount=saveCount=legacyCount=beginCount=endCount=abortCount=0;
    sessionDiscardCount=0;
    gComboVanillaCreationFailed=0;gComboVanillaMode=0;
    combined=true;vanilla=true;seed=false;beginOk=finishOk=true;
    std::memcpy(metadata.playerName,"NAME1234",8);
}
int main() {
    for(bool loadedSeed:{false,true}) {
        Reset();seed=loadedSeed;Sram_InitSave(&file);
        assert(sessionDiscardCount==1);
        assert(initCount==1&&saveCount==1&&randoCount==0&&legacyCount==0);
        assert(beginCount==1&&endCount==1&&gComboVanillaMode==1&&!gComboVanillaCreationFailed);
        assert(gSaveContext.ship.quest.id==QUEST_NORMAL&&metadata.valid);
        assert(!std::memcmp(gSaveContext.playerName,"NAME1234",8));
    }
    Reset();beginOk=false;Sram_InitSave(&file);
    assert(sessionDiscardCount==0);
    assert(gComboVanillaCreationFailed&&initCount==0&&saveCount==0&&!metadata.valid);
    assert(gSaveContext.health==77);
    Reset();finishOk=false;Sram_InitSave(&file);
    assert(sessionDiscardCount==1);
    assert(gComboVanillaCreationFailed&&saveCount==1&&!metadata.valid&&abortCount==1);
    assert(gSaveContext.health==77&&gComboVanillaMode==0);
    Reset();vanilla=false;seed=true;Sram_InitSave(&file);
    assert(sessionDiscardCount==1);
    assert(randoCount==1&&saveCount==1&&legacyCount==1&&beginCount==0);
    Reset();combined=false;file.questType[0]=QUEST_NORMAL;Sram_InitSave(&file);
    assert(sessionDiscardCount==1);
    assert(randoCount==0&&saveCount==1&&legacyCount==0&&beginCount==0);
    Reset();combined=false;file.questType[0]=QUEST_OOTXMM;Sram_InitSave(&file);
    assert(sessionDiscardCount==0&&initCount==0&&saveCount==0&&randoCount==0&&legacyCount==0&&beginCount==0);
    for(auto commit:{CommitPAL,CommitNES}) {
        Reset();finishOk=false;rumbles=0;commit(&file);
        assert(sessionDiscardCount==1);
        assert(gComboVanillaCreationFailed&&file.configMode==99&&rumbles==0);
        Reset();rumbles=0;commit(&file);
        assert(sessionDiscardCount==1);
        assert(!gComboVanillaCreationFailed&&file.configMode==CM_NAME_ENTRY_TO_MAIN&&rumbles==1);
    }
    std::cout<<"PASS actual OoT creation: seed-independent Vanilla, names, paired-failure rollback, Randomizer and Solo controls\n";
}
'''
flags=['-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-variable']
if '--sanitize' in sys.argv:
    flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='native-vanilla-create-') as directory:
    cpp=Path(directory)/'create.cpp';cpp.write_text(code)
    exe=Path(directory)/'create'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
