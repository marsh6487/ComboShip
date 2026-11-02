#!/usr/bin/env python3
"""Exercise native song grant/draw ownership and read-only quest-page diagnostics.

The actual production grant arm and song draw loop are compiled below. Graphics
commands are captured at submission; GPU rendering and installed archives are
outside this fixture. This does not claim to reproduce invisible runtime icons.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, signature):
    start = source.index('{', source.index(signature))
    end, depth = start + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--source-ref', help='negative control: read trace from an older revision')
args = parser.parse_args()
read = lambda p: (ROOT / p).read_text()
scope_path = 'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c'
scope = (subprocess.check_output(['git', 'show', args.source_ref + ':' + scope_path], cwd=ROOT, text=True)
         if args.source_ref else read(scope_path))
collect = read('soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_collect.c')
parameter = read('soh/src/code/z_parameter.c')
macros = read('soh/include/macros.h')
ownership = next(line for line in macros.splitlines() if line.startswith('#define CHECK_QUEST_ITEM('))
draw_signature = 'for (sp218 = 0; sp218 < QUEST_KOKIRI_EMERALD - QUEST_SONG_MINUET; sp218++, sp21A += 4)'
draw = draw_signature + block(collect, draw_signature)
grant = block(parameter, 'else if ((item >= ITEM_SONG_MINUET) && (item <= ITEM_SONG_STORMS))')
trace = block(scope, 'static void KaleidoNei_TraceQuestSongs(')
warp = read('soh/soh/Enhancements/QoL/PauseWarp.cpp')
selection = block(warp, 'static void PauseWarp_HandleSelection(')
assert 'KaleidoNei_TraceQuestSongs(play);' in block(scope, 'void KaleidoScope_DrawPages(')
code = r'''
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using u8=uint8_t; using u32=uint32_t; using s32=int32_t;
#include "soh/include/z64item.h"
constexpr int PAUSE_QUEST=2, ITEM_NONE_TEST=0xFF;
uint32_t gBitFlags[32];
struct Vertex { struct { int ob[3]; } v; };
struct PauseContext {
    int state=6, pageIndex=PAUSE_QUEST, randoQuestMode=0;
    int cursorPoint[4]{},cursorSlot[4]{},ocarinaSongIdx=0,alpha=255;
    Vertex* questVtx=nullptr;
};
struct PlayState {
    PauseContext pauseCtx;
    struct { struct { struct { unsigned button=0; } press; } input[1]; } state;
};
PlayState* gPlayState;
struct Save {
    struct { uint32_t questItems=0; int items[24]{}; } inventory;
    struct { struct { int id=0; } quest; } ship;
    int fileNum=0;
} gSaveContext;
constexpr int QUEST_RANDOMIZER=1, BTN_A=0x8000, RSK_SHUFFLE_OCARINA_BUTTONS=0;
enum { RAND_INF_HAS_OCARINA_A, RAND_INF_HAS_OCARINA_C_LEFT, RAND_INF_HAS_OCARINA_C_RIGHT,
       RAND_INF_HAS_OCARINA_C_UP, RAND_INF_HAS_OCARINA_C_DOWN };
static bool shuffleButtons=false, buttonOwned[5]={true,true,true,true,true};
int Randomizer_GetSettingValue(int) { return shuffleButtons; }
bool Flags_GetRandomizerInf(int button) { return buttonOwned[button]; }
#define CHECK_BTN_ALL(state,combo) (((state)&(combo))==(combo))
static int activations=0, activatedSong=-1;
void ActivateWarp(PauseContext*,int song) { ++activations; activatedSong=song; }
static int mmPage=0, instantPlay=1;
static std::vector<std::string> logs;
#define CVAR_ENHANCEMENT(x) "gEnhancements." x
int CVarGetInteger(const char* key,int fallback) {
    if (!std::strcmp(key,CVAR_ENHANCEMENT("SkijerNEI.MmQuestPage"))) return mmPage;
    if (!std::strcmp(key,CVAR_ENHANCEMENT("SkijerNEI.PausePlay"))) return instantPlay;
    return fallback;
}
void lusprintf(const char*,int,int,const char* fmt,...) {
    char text[512]; va_list ap; va_start(ap,fmt); std::vsnprintf(text,sizeof(text),fmt,ap); va_end(ap);
    logs.emplace_back(text);
}
void osSyncPrintf(const char*,...) {}
#define VT_FGCOL(x) ""
#define YELLOW 0
#define VT_RST ""
#define MOD_NONE 0
int Return_Item(int,int,int item) { return item; }
static std::vector<int> quads;
static int currentQuad;
#define POLY_OPA_DISP dummy
#define gDPSetPrimColor(...) ((void)0)
#define gSPVertex(cmd,vertex,count,index) (currentQuad=int((vertex)-pauseCtx->questVtx)/4)
#define gSP1Quadrangle(...) quads.push_back(currentQuad)
'''
code += ownership + '\n'
code += 'int Grant(int item) ' + grant + '\n'
code += 'void Draw(PauseContext* pauseCtx,int sp216) { int sp218,sp21A=24; ' + draw + '\n}\n'
code += 'void Trace(PlayState* play) ' + trace + '\n'
code += 'void SelectWarp() ' + selection + '\n'
code += r'''
int main() {
    for(int i=0;i<32;i++) gBitFlags[i]=1u<<i;
    PlayState play; Vertex vertices[188]{}; play.pauseCtx.questVtx=vertices;
    for (int song=0;song<12;song++) {
        gSaveContext.inventory.questItems=1u<<QUEST_MEDALLION_FIRE;
        Grant(ITEM_SONG_MINUET+song);
        assert(gSaveContext.inventory.questItems==((1u<<QUEST_MEDALLION_FIRE)|(1u<<(QUEST_SONG_MINUET+song))));
        const auto once=gSaveContext.inventory.questItems;
        Grant(ITEM_SONG_MINUET+song); assert(gSaveContext.inventory.questItems==once);
    }
    for(unsigned mask=0;mask<4096;mask++) {
        gSaveContext.inventory.questItems=mask<<QUEST_SONG_MINUET;
        quads.clear(); Draw(&play.pauseCtx,-1);
        std::vector<int> expected;
        for(int song=0;song<12;song++) if(mask&(1u<<song)) expected.push_back(QUEST_SONG_MINUET+song);
        assert(quads==expected);
    }
    // Real PauseWarp selection reaches its activation boundary only for an
    // owned warp song, with an ocarina and A pressed on the native quest page.
    // Textbox rendering, native transitions and the final warp are not mocked as proven.
    gPlayState=&play;
    for(int owned=0;owned<2;owned++) for(int ocarina=0;ocarina<2;ocarina++)
    for(int pressed=0;pressed<2;pressed++) for(int song=0;song<12;song++) {
        gSaveContext.inventory.questItems=owned?(1u<<(QUEST_SONG_MINUET+song)):0;
        gSaveContext.inventory.items[SLOT_OCARINA]=ocarina?ITEM_OCARINA_TIME:ITEM_NONE;
        play.state.input[0].press.button=pressed?BTN_A:0;
        play.pauseCtx.cursorPoint[PAUSE_QUEST]=QUEST_SONG_MINUET+song;
        activations=0; activatedSong=-1; SelectWarp();
        assert(activations==(owned&&ocarina&&pressed&&song<6));
        if(activations) assert(activatedSong==QUEST_SONG_MINUET+song);
    }
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER; shuffleButtons=true;
    gSaveContext.inventory.questItems=(1u<<QUEST_SONG_MINUET)|(1u<<QUEST_SONG_NOCTURNE);
    play.pauseCtx.cursorPoint[PAUSE_QUEST]=QUEST_SONG_MINUET;
    buttonOwned[RAND_INF_HAS_OCARINA_A]=false;
    activations=0; SelectWarp(); assert(activations==0);
    buttonOwned[RAND_INF_HAS_OCARINA_A]=true;
    SelectWarp(); assert(activations==1);
    play.pauseCtx.cursorPoint[PAUSE_QUEST]=QUEST_SONG_NOCTURNE;
    buttonOwned[RAND_INF_HAS_OCARINA_C_DOWN]=false;
    activations=0; SelectWarp(); assert(activations==0);
    buttonOwned[RAND_INF_HAS_OCARINA_C_DOWN]=true;
    SelectWarp(); assert(activations==1 && activatedSong==QUEST_SONG_NOCTURNE);
    gSaveContext.inventory.questItems=(1u<<QUEST_SONG_MINUET)|(1u<<QUEST_SONG_NOCTURNE);
    const auto save=gSaveContext.inventory.questItems;
    Trace(&play); assert(logs.size()==1);
    assert(logs.back().find("quest=00000440 minuet=1 nocturne=1 mmPage=0 randoPage=0")!=std::string::npos);
    for(int i=0;i<100;i++) Trace(&play);
    assert(logs.size()==1);
    mmPage=1; Trace(&play); assert(logs.size()==2 && logs.back().find("mmPage=1")!=std::string::npos);
    play.pauseCtx.randoQuestMode=1; Trace(&play); assert(logs.size()==3);
    play.pauseCtx.state=7; Trace(&play); assert(logs.size()==3);
    play.pauseCtx.state=6; Trace(&play); assert(logs.size()==4);
    gSaveContext.fileNum=1; Trace(&play); assert(logs.size()==5);
    gSaveContext.inventory.questItems&=~(1u<<QUEST_SONG_NOCTURNE); Trace(&play);
    assert(logs.size()==6 && logs.back().find("nocturne=0")!=std::string::npos);
    assert(gSaveContext.inventory.questItems==(save&~(1u<<QUEST_SONG_NOCTURNE)));
    assert(mmPage==1 && play.pauseCtx.randoQuestMode==1);
    std::cout<<"PASS native song grants, all 4096 ownership/draw masks, PauseWarp activation guards, and read-only bounded layout trace\n";
}
'''
with tempfile.TemporaryDirectory(prefix='oot-quest-songs-') as tmp:
    source = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    source.write_text(code)
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
