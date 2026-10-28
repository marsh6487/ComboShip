#!/usr/bin/env python3
"""Run real token gates and OC admission, including a full native collider table."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile
from run_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    token = (ROOT / "soh/src/overlays/actors/ovl_En_Si/z_en_si.c").read_text()
    collision = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    header = (ROOT / "soh/include/z64collision_check.h").read_text()
    capacity = re.search(r"^#define COLLISION_CHECK_OC_MAX\s+\d+", header, re.M)[0]
    prefix = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
using u8=uint8_t; using u16=uint16_t; using s32=int; using f32=float;
struct Vec3f { float x{},y{},z{}; };
struct Actor {
    int flags{}, params{}, room{}, freezeTimer{};
    void (*update)(){};
    float xzDistToPlayer{}, yDistToPlayer{};
    Vec3f scale;
    struct { struct { int y{}; } rot; } shape;
};
struct Collider { Actor* actor{}; int shape{}, acFlags{}, ocFlags1{}, ocFlags2{}; };
struct ColliderCylinder { Collider base; struct { int radius{},height{},yShift{}; } dim; };
struct CollisionCheckContext { int colOCCount{},sacFlags{}; Collider* colOC[COLLISION_CHECK_OC_MAX]{}; };
struct Player {
    Actor actor; ColliderCylinder cylinder;
    int csAction{},unk_6AD{}; unsigned stateFlags1{},stateFlags2{},stateFlags3{};
};
struct PlayState {
    Player* player; CollisionCheckContext colChkCtx;
    int sceneNum{82},gameplayFrames{},transitionTrigger{};
    struct { struct { int num{}; } curRoom; } roomCtx;
    int msgCtx{};
};
struct EnSi { Actor actor; void (*actionFunc)(EnSi*,PlayState*){}; ColliderCylinder collider; u8 unk_19C{}; };
#define GET_PLAYER(play) ((play)->player)
#define CHECK_FLAG_ALL(value,bits) (((value)&(bits))==(bits))
#define SET_GS_FLAGS(group,bits) (++gsFlags)
enum { COLSHAPE_QUAD=3,AC_HIT=2,OC2_HIT_PLAYER=1,ACTOR_FLAG_HOOKSHOT_ATTACHED=8,
       VB_GIVE_ITEM_SKULL_TOKEN,VB_FREEZE_ON_SKULL_TOKEN,ITEM_SKULL_TOKEN,TEXT_GS_NO_FREEZE,
       NA_BGM_SMALL_ITEM_GET,TEXT_STATE_CLOSING,FLAG_GS_TOKEN,LUSLOG_LEVEL_INFO };
static int blockLogs,contactGives,gsFlags,queueEvents; static bool cutscene,vanilla;
static char lastLog[1200];
static void lusprintf(const char*,int,int,const char* fmt,...) {
    va_list args; va_start(args,fmt); vsnprintf(lastLog,sizeof(lastLog),fmt,args); va_end(args);
    if(strstr(lastLog,"phase=blocked")) ++blockLogs;
}
#define osSyncPrintf(...) ((void)0)
static bool GameInteractor_SecondCollisionUpdate() { return false; }
static bool FrameAdvance_IsEnabled(PlayState*) { return false; }
static void resetOC(PlayState*,Collider*) {}
static void (*sOCResetFuncs[])(PlayState*,Collider*)={resetOC,resetOC,resetOC,resetOC};
static void Math_SmoothStepToF(float* p,float target,float,float,float) { *p=target; }
static void Actor_SetScale(Actor*,float) {}
static bool Player_InCsMode(PlayState*) { return cutscene; }
static bool GameInteractor_Should(int flag,bool,...) {
    return flag == VB_GIVE_ITEM_SKULL_TOKEN ? vanilla : false;
}
static void Item_Give(PlayState*,int) { ++contactGives; }
static void Message_StartTextbox(PlayState*,int,void*) {}
static void Audio_PlayFanfare(int) {}
static void Collider_UpdateCylinder(Actor*,ColliderCylinder*) {}
static void CollisionCheck_SetAC(PlayState*,CollisionCheckContext*,Collider*) {}
static int Message_GetState(int*) { return TEXT_STATE_CLOSING; }
static void GameInteractor_ExecuteOnFlagSet(int type,int params) {
    assert(type==FLAG_GS_TOKEN && params==0x810); ++queueEvents;
}
static void Actor_Kill(Actor*) {}
static void Actor_MoveXZGravity(Actor*) {}
static void Actor_UpdateBgCheckInfo(PlayState*,Actor*,float,float,float,int) {}
static void Actor_SetFocus(Actor*,float) {}
static void live() {}
void func_80AFB950(EnSi*,PlayState*);
void func_80AFB89C(EnSi*,PlayState*);
'''
    source = capacity + "\n" + prefix + function(collision,"s32 CollisionCheck_SetOC(") + "\n"
    if "static void EnSi_LogPickupBlock(" in token:
        source += function(token,"static void EnSi_LogPickupBlock(") + "\n"
    for signature in ("s32 func_80AFB748(","void func_80AFB768(","void func_80AFB89C(",
                      "void func_80AFB950(","void EnSi_Update("):
        # Skip forward declarations: start at the real function definition.
        match = re.search(r"^"+re.escape(signature)+r"[^;{}]*\)\s*\{",token,re.M)
        source += function(token[match.start():],signature) + "\n"
    source = re.sub(r"\bthis\b","self",source)
    source += r'''
int main() {
    Player player{}; PlayState play{}; play.player=&player;
    Actor fillerActor{}; fillerActor.update=live;
    Collider filler{}; filler.actor=&fillerActor;
    for(int i=0;i<COLLISION_CHECK_OC_MAX;++i)
        assert(CollisionCheck_SetOC(&play,&play.colChkCtx,&filler)==i);
    EnSi token{}; token.actor.params=0x810; token.actor.update=live;
    token.collider.base.actor=&token.actor; token.actionFunc=func_80AFB768;
    assert(CollisionCheck_SetOC(&play,&play.colChkCtx,&token.collider.base)==-1);
    EnSi_Update(&token.actor,&play);
    assert(play.colChkCtx.colOCCount==COLLISION_CHECK_OC_MAX);
    assert(token.actionFunc==func_80AFB768 && !queueEvents && !contactGives);
    assert(blockLogs==1 && strstr(lastLog,"reason=oc-registration"));
    EnSi_Update(&token.actor,&play);
    assert(blockLogs==1); // bounded trace while contact remains blocked

    play.colChkCtx.colOCCount=0; token.unk_19C=0; blockLogs=0;
    cutscene=true; EnSi_Update(&token.actor,&play);
    assert(blockLogs==1 && strstr(lastLog,"reason=cutscene"));
    assert(!play.colChkCtx.colOCCount && !queueEvents && !contactGives);
    cutscene=false; vanilla=false;
    token.collider.base.ocFlags2=OC2_HIT_PLAYER;
    EnSi_Update(&token.actor,&play);
    assert(token.actionFunc==func_80AFB950 && !contactGives);
    EnSi_Update(&token.actor,&play);
    assert(queueEvents==1 && gsFlags==1); // randomized flag dispatch still owns the reward

    token.actionFunc=func_80AFB768; token.actor.flags=ACTOR_FLAG_HOOKSHOT_ATTACHED;
    EnSi_Update(&token.actor,&play);
    assert(token.actionFunc==func_80AFB89C);
    token.actor.flags=0; vanilla=true;
    EnSi_Update(&token.actor,&play);
    assert(token.actionFunc==func_80AFB950 && contactGives==1);
}
'''
    with tempfile.TemporaryDirectory(prefix="skull-pickup-") as directory:
        path = Path(directory)
        (path / "test.cpp").write_text(source)
        flags=["-std=c++20","-O1","-g"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-no-pie"]
        subprocess.run([os.environ.get("CXX","g++"),*flags,str(path/"test.cpp"),"-o",str(path/"test")],check=True)
        subprocess.run([str(path/"test")],check=True)
    print("PASS: real 50-slot OC refusal traced; native cutscene, hookshot, randomizer gates preserved")


if __name__=="__main__":
    main()
