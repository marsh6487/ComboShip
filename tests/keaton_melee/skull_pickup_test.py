#!/usr/bin/env python3
"""Run real token gates and growable native OC admission beyond the original limit."""
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
    capacities = "\n".join(re.findall(r"^#define COLLISION_CHECK_[A-Z_]+[^\n]*", header, re.M))
    native_header = (ROOT / "soh/include/z64.h").read_text()
    context_end = native_header.index("} CollisionCheckContext;") + len("} CollisionCheckContext;")
    context = native_header[native_header.rindex("typedef struct", 0, context_end):context_end]
    prefix = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <climits>
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
struct ColliderCylinder { Collider base; struct { f32 radius{},height{},yShift{}; } dim; };
struct OcLine {};
// NATIVE_CONTEXT
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
static int registers[32];
#define AREG(index) registers[index]
static void DinFireSword_FinishTeardown(PlayState*) {}
void CollisionCheck_ClearContext(PlayState*,CollisionCheckContext*);
// Adapt C's implicit allocator conversion at the C++ fixture boundary only.
static Collider** NativeRealloc(void* ptr,size_t bytes) { return (Collider**)std::realloc(ptr,bytes); }
#define realloc NativeRealloc
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
    lifecycle_start = collision.index("/* Actor collider queue ownership */")
    lifecycle_end = collision.index(function(collision, "void CollisionCheck_DisableSAC(")) + len(function(collision, "void CollisionCheck_DisableSAC("))
    source = capacities + "\n" + prefix.replace("// NATIVE_CONTEXT", context)
    source += collision[lifecycle_start:lifecycle_end] + "\n" + function(collision,"s32 CollisionCheck_SetOC(") + "\n"
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
    CollisionCheck_InitContext(&play,&play.colChkCtx);
    Actor fillerActor{}; fillerActor.update=live;
    Collider filler{}; filler.actor=&fillerActor;
    for(int i=0;i<75;++i)
        assert(CollisionCheck_SetOC(&play,&play.colChkCtx,&filler)==i);
    EnSi token{}; token.actor.params=0x810; token.actor.update=live;
    token.collider.base.actor=&token.actor; token.actionFunc=func_80AFB768;
    player.cylinder.dim = {12.5f,62.0f,-0.5f};
    token.collider.dim = {7.5f,14.0f,1.5f};
    EnSi_Update(&token.actor,&play);
    assert(play.colChkCtx.colOCCount==76 && play.colChkCtx.colOC[75]==&token.collider.base);
    assert(token.actionFunc==func_80AFB768 && !queueEvents && !contactGives);
    assert(blockLogs==1 && strstr(lastLog,"reason=awaiting-contact"));
    char capacityLog[80];
    snprintf(capacityLog,sizeof(capacityLog),"ocCount=%d/%d",play.colChkCtx.colOCCount,play.colChkCtx.colOCCapacity);
    assert(strstr(lastLog,capacityLog));
    assert(strstr(lastLog,"playerCyl=12.5,62.0,-0.5 tokenCyl=7.5,14.0,1.5"));
    EnSi_Update(&token.actor,&play);
    assert(blockLogs==1); // bounded trace while waiting for native contact

    CollisionCheck_ClearContext(&play,&play.colChkCtx); token.unk_19C=0; blockLogs=0;
    CollisionCheck_EnableSAC(&play,&play.colChkCtx);
    EnSi_Update(&token.actor,&play);
    assert(!play.colChkCtx.colOCCount && blockLogs==1 && strstr(lastLog,"reason=oc-registration"));
    CollisionCheck_DisableSAC(&play,&play.colChkCtx);
    token.unk_19C=0; blockLogs=0;
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
    CollisionCheck_DestroyContext(&play,&play.colChkCtx);
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
    print("PASS: token admitted after 75 OCs; actual capacity and float dimensions traced; native cutscene, hookshot and randomizer gates preserved")


if __name__=="__main__":
    main()
