#!/usr/bin/env python3
"""Execute OoT logical dungeon counts and the native game-over menu boundaries."""
import pathlib,re,subprocess,tempfile
import os, sys
sanitize_flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitize' in sys.argv else []
if sanitize_flags:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
root=pathlib.Path(__file__).resolve().parents[2]
def fn(text,marker):
 a=text.index(marker);b=text.index('{',a);d,e=1,b+1
 while d:d+=(text[e]=='{')-(text[e]=='}');e+=1
 return text[a:e]
logic=fn((root/'soh/soh/Enhancements/randomizer/logic.cpp').read_text(),'uint8_t Logic::DungeonCount()').replace('Logic::DungeonCount','CountLogic')
play=fn((root/'soh/src/code/z_play.c').read_text(),'u8 CheckDungeonCount()')
death=fn((root/'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text(),'void Entrance_SetGameOverEntrance(void)')
view=fn((root/'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c').read_text(),'void KaleidoScope_SetView(')
code=logic+'\n'+play+'\n'+death+'\n'+view
names=sorted(set(re.findall(r'\b(?:ENTR_|SCENE_|RSK_|RO_|LOGIC_|RAND_INF_|EVENTCHKINF_)[A-Z0-9_]+',code)))
prefix='''#include <cassert>
#include <cstdint>
#define COMBO_BUILD 1
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include "combo/rando/CrossEntranceBoss.h"
using u8=uint8_t;using s16=int16_t;
'''+ 'enum {'+','.join(names)+'};\n'+r'''
static bool active=false, CalculatingAvailableChecks=false;
static uint16_t logical=0;
static int flags[1000]{};static int Get(int id){return flags[id];}
static int CheckEventChkInf(int id){return flags[id];}static int CheckRandoInf(int id){return flags[id];}
static int Flags_GetEventChkInf(int id){return flags[id];}static int Flags_GetRandomizerInf(int id){return flags[id];}
int Combo_SOH_CrossBossEnabled(){return active;}
int Combo_SOH_CrossBossLogicalClear(int id,int native){return active?bool(logical&(1u<<id)):native;}
static int Randomizer_GetSettingValue(int){return RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF;}
static int Grotto_OverrideSpecialEntrance(int index){return index;}
static int bossSceneSaveDeathWarps[1000]{};
static struct {int entranceIndex=0;} gSaveContext;
static struct {int sceneNum=0;} playState;
static auto* gPlayState=&playState;
using f32=float;
struct Vec3f { float x, y, z; };
struct GraphicsContext { int top, bottom; };
struct View { GraphicsContext* gfxCtx; };
struct PauseContext { View view; int state; };
static constexpr int SCREEN_WIDTH=320, SCREEN_HEIGHT=240;
static void func_800AA358(View*, Vec3f*, Vec3f*, Vec3f*) {}
static void func_800AAA50(View* view, int) {
    view->gfxCtx->top=70;
    view->gfxCtx->bottom=170;
}
#define OPEN_DISPS(ctx) int POLY_OPA_DISP=0
#define CLOSE_DISPS(ctx) (void)POLY_OPA_DISP
#define gDPPipeSync(disp) (void)(disp)
#define gDPSetScissor(disp, mode, left, y0, right, y1) \
    do { (void)(disp); gfxCtx->top=(y0); gfxCtx->bottom=(y1); } while (false)
'''
main=r'''
int main(){
 for(auto& flag:flags)flag=1;flags[RAND_INF_DUNGEONS_DONE_GANONS_TOWER]=0;
 active=true;logical=(1u<<2)|(1u<<9);
 assert(CountLogic()==1 && "physical target victories cannot count as source dungeons");
 CalculatingAvailableChecks=true;assert(CountLogic()==1);assert(CheckDungeonCount()==1);
 flags[RAND_INF_DUNGEONS_DONE_GANONS_TOWER]=1;assert(CheckDungeonCount()==2);
 active=false;assert(CountLogic()==8 && CheckDungeonCount()==9);
 active=true;playState.sceneNum=SCENE_DEKU_TREE_BOSS;gSaveContext.entranceIndex=ENTR_DEKU_TREE_BOSS_ENTRANCE;
 Entrance_SetGameOverEntrance();assert(gSaveContext.entranceIndex==ENTR_DEKU_TREE_ENTRANCE && "native preprocessing leaves combined retry to Continue");
 active=false;gSaveContext.entranceIndex=ENTR_DEKU_TREE_BOSS_ENTRANCE;Entrance_SetGameOverEntrance();assert(gSaveContext.entranceIndex==ENTR_DEKU_TREE_ENTRANCE);
 GraphicsContext gfx{};
 PauseContext pause{{&gfx}, 0x10};
 KaleidoScope_SetView(&pause,0,0,64);
 assert(gfx.top==0 && gfx.bottom==SCREEN_HEIGHT && "the boss camera must not clip the game-over prompt");
 pause.state=7;
 KaleidoScope_SetView(&pause,0,0,64);
 assert(gfx.top==70 && gfx.bottom==170);
}
'''
with tempfile.TemporaryDirectory(prefix='boss-world-') as tmp:
 source=pathlib.Path(tmp)/'test.cpp';binary=pathlib.Path(tmp)/'test';source.write_text(prefix+code+main)
 subprocess.run(['g++',*sanitize_flags,'-std=c++20','-I',str(root),str(source),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
