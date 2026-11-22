#!/usr/bin/env python3
"""Compare keeper presentation with the actual MM EnTakaraya actions.

Animation decoding/rendering and message table bytes remain runtime boundaries.
Native action bodies are extracted unchanged except renaming the C identifier `this`
for C++; only service calls are stubbed.
"""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/black_market'))
from run_shop_camera_mm_tests import function, array

native = (ROOT / 'mm/src/overlays/actors/ovl_En_Takaraya/z_en_takaraya.c').read_text()
source = r'''
#include "combo/BlackMarketKeeperInteraction.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace BlackMarketKeeperInteraction;
using u8=unsigned char; using u16=unsigned short;
struct AnimationHeader { Clip clip; };
AnimationHeader object_bg_Anim_009890{Clip::Idle}, object_bg_Anim_00A280{Clip::EnterTalk},
 object_bg_Anim_001384{Clip::Talk}, object_bg_Anim_00AD98{Clip::Raise}, object_bg_Anim_000968{Clip::Ask};
struct SkelAnime { AnimationHeader* animation; };
struct Actor { u16 textId; };
struct MsgCtx { unsigned choiceIndex; int unk1206C; };
struct PlayState { int state; MsgCtx msgCtx; };
struct EnTakaraya { SkelAnime skelAnime; Actor actor; int formSwitchFlag, switchFlag;
 void (*actionFunc)(EnTakaraya*, PlayState*); };
static struct { struct { struct { struct { int rupees; } playerData; } saveInfo; } save; } gSaveContext;
static bool finished, accepted, advance;
static int textState;
constexpr int TEXT_STATE_CLOSING=2, TEXT_STATE_DONE=3, TEXT_STATE_NEXT=1, TEXT_STATE_CHOICE=4;
constexpr int FACE_REACTION_SET_TREASURE_CHEST_SHOP_GAL=0, NA_SE_SY_ERROR=1;
#define GET_PLAYER_FORM 4
#define CLEAR_WEEKEVENTREG(...) ((void)0)
/* NATIVE GAL MESSAGE TABLES */
struct Change { Clip clip; float morph; };
static std::vector<Change> reference, candidate;
void changed(SkelAnime* skel, AnimationHeader* anim, float morph) { skel->animation=anim; reference.push_back({anim->clip,morph}); }
void Animation_MorphToPlayOnce(SkelAnime* s,AnimationHeader* a,float m) {changed(s,a,m);}
void Animation_MorphToLoop(SkelAnime* s,AnimationHeader* a,float m) {changed(s,a,m);}
void Animation_PlayOnce(SkelAnime* s,AnimationHeader* a) {changed(s,a,0);}
void Animation_PlayLoop(SkelAnime* s,AnimationHeader* a) {changed(s,a,0);}
int SkelAnime_Update(SkelAnime*) {return finished;}
bool Actor_TalkOfferAccepted(Actor*,int*) {return accepted;}
bool Actor_IsFacingPlayer(Actor*,int) {return false;}
int Text_GetFaceReaction(PlayState*,int) {return 0;}
void Actor_OfferTalk(Actor*,PlayState*,float) {}
void func_80ADF2D4(EnTakaraya*) {}
void EnTakaraya_SpawnWalls(EnTakaraya*,PlayState*) {}
int Message_GetState(MsgCtx*) {return textState;}
bool Message_ShouldAdvance(PlayState*) {return advance;}
void Audio_PlaySfx(int) {}
void Audio_PlaySfx_MessageDecide() {}
void Audio_PlaySfx_MessageCancel() {}
void Rupees_ChangeBy(int) {}
void Message_ContinueTextbox(PlayState*,u16) {}
void EnTakaraya_Wait(EnTakaraya*,PlayState*);
void EnTakaraya_Talk(EnTakaraya*,PlayState*);
void EnTakaraya_SetupTalk(EnTakaraya*);
'''
source = source.replace('/* NATIVE GAL MESSAGE TABLES */', '#define PLAYER_FORM_MAX 5\n' + '\n'.join(
    array(native, signature) for signature in ('u16 D_80ADFB2C[', 'u16 D_80ADFB44[', 'u16 D_80ADFB50[')))
source += '\n'.join(re.sub(r'\bthis\b', 'self', function(native, name)) for name in (
 'void EnTakaraya_SetupWait(', 'void EnTakaraya_Wait(',
 'void EnTakaraya_SetupTalk(', 'void EnTakaraya_Talk('))
source += r'''
int main() {
 PlayState play{};
 EnTakaraya gal{{&object_bg_Anim_009890},{0x76D},0,0,nullptr};
 EnTakaraya_SetupWait(&gal);
 State state;
 auto check = [&](bool active,bool done,bool page,bool refusal) {
   state.Update(active,done,page,refusal,[](Clip c,float m){candidate.push_back({c,m});});
   assert(reference.size()==candidate.size());
   for(size_t i=0;i<reference.size();++i) {
     assert(reference[i].clip==candidate[i].clip);
     assert(reference[i].morph==candidate[i].morph);
   }
 };
 // Native greeting -> talk sway, with no reset on each conversation frame.
 accepted=true; gal.actionFunc(&gal,&play); check(true,false,false,false);
 accepted=false; gal.actionFunc(&gal,&play); check(true,false,false,false);
 finished=true; gal.actionFunc(&gal,&play); check(true,true,false,false);
 finished=false; gal.actionFunc(&gal,&play); check(true,false,false,false);
 // Native next-page raise -> asking -> idle.
 textState=TEXT_STATE_NEXT; advance=true; gal.actionFunc(&gal,&play); check(true,false,true,false);
 textState=0; advance=false; finished=true; gal.actionFunc(&gal,&play); check(true,true,false,false);
 gal.actionFunc(&gal,&play); check(true,true,false,false);
 // An unaffordable selection from idle uses the native asking reaction.
 finished=false; textState=TEXT_STATE_CHOICE; advance=true; play.msgCtx.choiceIndex=0;
 play.msgCtx.unk1206C=20; gal.actionFunc(&gal,&play); check(true,false,false,true);
 // Closing a talk while asking preserves its one-shot, then restores idle.
 textState=TEXT_STATE_CLOSING; advance=false; gal.actionFunc(&gal,&play); check(false,false,false,false);
 textState=0; finished=true; gal.actionFunc(&gal,&play); check(false,true,false,false);
 // Enter a new conversation and close while swaying: native raise then idle.
 finished=false; accepted=true; gal.actionFunc(&gal,&play); check(true,false,false,false);
 accepted=false; finished=true; gal.actionFunc(&gal,&play); check(true,true,false,false);
 finished=false; textState=TEXT_STATE_CLOSING; gal.actionFunc(&gal,&play); check(false,false,false,false);
 finished=true; textState=0; gal.actionFunc(&gal,&play); check(false,true,false,false);
 // Sleep uses a distinct wake clip, then resumes the native talk loop.
 State sleeping{.clip = Clip::Sleep, .sleepClips = true};
 std::vector<Change> blends;
 auto blend = [&](Clip clip, float frames) { blends.push_back({clip, frames}); };
 sleeping.Update(true,false,false,false,blend);
 sleeping.Update(true,false,true,false,blend);
 assert(blends.size() == 1);
 sleeping.Update(true,true,false,false,blend);
 sleeping.Update(false,false,false,false,blend);
 sleeping.Update(false,true,false,false,blend);
 assert(blends.size() == 4);
 assert(blends.front().clip == Clip::Wake && blends.front().morph < 0.f);
 assert(blends[1].clip == Clip::Talk && blends[1].morph < 0.f);
 assert(blends[2].clip == Clip::Raise && blends.back().clip == Clip::Sleep);
 sleeping.Update(true,false,false,false,blend);
 sleeping.Update(false,true,false,false,blend);
 assert(blends.back().clip == Clip::Sleep);
 // Cues are copied as controls, not guessed SFX IDs or false argument matches.
 assert(FirstMessageSfx(std::string("\x17Hi\x1E\x68\x58\xBF",8))==std::string("\x1E\x68\x58",3));
 assert(FirstMessageSfx(std::string("\x1B\x1E\x42Text\xBF",8)).empty());
 assert(FirstMessageSfx(std::string("\x14\x1E\xBF",3)).empty());
 assert(FirstMessageSfx(std::string("\x1E\x68",2)).empty());
 assert(FirstMessageSfx(std::string("\xBF\x1E\x68\x58",4)).empty());
 for(unsigned form=0;form<5;++form) {
   assert(Greeting(form)==D_80ADFB2C[form]);
   assert(Talk(form)==D_80ADFB50[form]);
   assert(Decline(form)==D_80ADFB44[form]);
 }
 assert(Greeting(99)==Greeting(4) && Talk(99)==Talk(4));
 std::puts("PASS native EnTakaraya choreography parity: greeting, talk, page advance, refusal, close/re-entry; bounded original-message SFX parsing (ASan/UBSan)");
}
'''
with tempfile.TemporaryDirectory(prefix='market-keeper-interaction-') as folder:
    cpp=Path(folder)/'test.cpp'; cpp.write_text(source)
    binary=Path(folder)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-g','-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all','-no-pie','-I'+str(ROOT),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

# Compile the production table lookup against the actual MM message/table types.
lookup = function((ROOT / 'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp').read_text(),
                  'std::string NativeKeeperMessageSfx(')
cue_source = r'''
#include "global.h"
#include "combo/BlackMarketKeeperInteraction.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#define MESSAGE_HEADER_SIZE 11
PlayState* gPlayState;
'''+lookup+r'''
int main() {
 static PlayState play{};
 assert(NativeKeeperMessageSfx(0x76D).empty());
 gPlayState=&play;
 assert(NativeKeeperMessageSfx(0x76D).empty());
 char text[18]{}; std::memcpy(text+11,"Hi\x1E\x68\x58\xBF",6);
 MessageTableEntry table[]={{0x76D,0,text,sizeof(text)},{0x77B,0,nullptr,20},{0xFFFF,0,nullptr,0}};
 play.msgCtx.messageTableNES=table;
 assert(NativeKeeperMessageSfx(0x76D)==std::string("\x1E\x68\x58",3));
 assert(NativeKeeperMessageSfx(0x77B).empty());
 assert(NativeKeeperMessageSfx(0x1234).empty());
 table[0].msgSize=10;
 assert(NativeKeeperMessageSfx(0x76D).empty());
 std::puts("PASS actual MM message table lookup: native cue, absent entry/table/play, short or missing message (ASan/UBSan)");
}
'''
with tempfile.TemporaryDirectory(prefix='market-keeper-message-') as folder:
    cpp=Path(folder)/'test.cpp'; cpp.write_text(cue_source)
    binary=Path(folder)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-g','-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all','-no-pie','-w','-DF3DEX_GBI_2',
                    *['-I'+str(ROOT / p) for p in ('.','mm','mm/include','mm/assets','mm/src','mm/2s2h','libultraship/include','combo')],
                    str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
