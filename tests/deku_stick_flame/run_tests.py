#!/usr/bin/env python3
"""Compile and execute real selector, burning-stick function, and original wrapper bodies.

Only engine dependency types/functions are stubbed. No selector or caller logic is
reimplemented. --source-root permits red verification against immutable baseline.
"""
import argparse
from pathlib import Path
import re
import os
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--source-root', type=Path, default=ROOT)
a = parser.parse_args()

def function(src, name):
    match = re.search(r'^(?:static )?(?:const void\*|void) ' + name + r'\(', src, re.M)
    assert match, name
    start = match.start()
    opening = src.index('{', match.end())
    level = 1
    end = opening + 1
    while level:
        level += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[start:end]

common = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef float f32;
#define ALIGN_ASSET(n) __attribute__((aligned(n)))
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define EFFECT_SS_DUST_DRAW_DEKU_STICK_FLAME 0x200
#define DUST_DRAWFLAG0 0
#define DUST_UPDATE_NORMAL 0
#define EFFECT_SS_DUST_DRAW_SW97_FOREST 0x100
static int alt = 1, present[2][8], loadable[2][8];
static int resource_frame(const char* name, int* family) {
    const char* marker = strstr(name, "flame/flame");
    *family = marker ? 1 : 0;
    if (marker) return marker[11] - '1';
    marker = strstr(name, "forest/dust");
    assert(marker);
    return marker[11] - '1';
}
static int ResourceMgr_IsAltAssetsEnabled(void) { return alt; }
static int ResourceMgr_FileAltExists(const char* name) {
    int family, frame = resource_frame(name, &family);
    assert(frame >= 0 && frame < 8);
    return present[family][frame];
}
#ifdef MM
static void* ResourceGetDataByName(const char* name) {
    int family, frame = resource_frame(name, &family);
    return loadable[family][frame] ? (void*)name : NULL;
}
#else
char* ResourceMgr_LoadTexOrDListByName(const char* name) {
    int family, frame = resource_frame(name, &family);
    return loadable[family][frame] ? (char*)name : NULL;
}
#endif
static void reset_resources(void) {
    alt = 1;
    for (int f=0; f<2; ++f) for (int i=0; i<8; ++i) present[f][i]=loadable[f][i]=1;
}
static const char fallback[] = "native texture";
static void expect_texture(u16 flags, int frame, const char* expected) {
    const char* actual = EffectSsDust_SelectTexture(flags, frame, fallback);
    if (strcmp(actual, expected)) {
        fprintf(stderr, "FAIL flags=0x%x frame=%d expected=%s actual=%s\n", flags, frame, expected, actual);
        assert(0 && "production selector returned wrong texture family");
    }
}
static void selection_test(void) {
    char flame[80], forest[100];
    reset_resources();
    for (int i=0; i<8; ++i) {
        snprintf(flame, sizeof flame, "__OTR__custom/deku_stick/flame/flame%dTex", i+1);
        snprintf(forest, sizeof forest, "__OTR__custom/medallion_magic/spells/forest/dust%dTex", i+1);
        expect_texture(0x200, i, flame);
        for (int low=0; low<8; ++low) {
            expect_texture(low, i, fallback);
            expect_texture(0x100|low, i, forest);
            expect_texture(0x200|low, i, flame);
        }
    }
    alt=0;
    for (int i=0; i<8; ++i) { expect_texture(0x200,i,fallback); expect_texture(0x100,i,fallback); }
    alt=1;
    for (int missing=0; missing<8; ++missing) {
        present[1][missing]=0;
        for (int i=0; i<8; ++i) expect_texture(0x200,i,fallback);
        present[1][missing]=1;
        present[0][missing]=0;
        for (int i=0; i<8; ++i) expect_texture(0x100,i,fallback);
        present[0][missing]=1;
    }
    for (int missing=0; missing<8; ++missing) {
        loadable[1][missing]=0;
        for (int i=0; i<8; ++i) expect_texture(0x200,i,fallback);
        loadable[1][missing]=1;
#ifdef MM
        /* Preserve existing forest behavior: MM checks loads, OoT only existence. */
        loadable[0][missing]=0;
        for (int i=0; i<8; ++i) expect_texture(0x100,i,fallback);
        loadable[0][missing]=1;
#endif
    }
    /* Same selector instance, no restart: toggles, success, removal, restoration. */
    expect_texture(0x200,0,"__OTR__custom/deku_stick/flame/flame1Tex");
    alt=0; expect_texture(0x200,0,fallback);
    alt=1; expect_texture(0x200,0,"__OTR__custom/deku_stick/flame/flame1Tex");
    present[1][7]=0; expect_texture(0x200,0,fallback);
    present[1][7]=1; expect_texture(0x200,7,"__OTR__custom/deku_stick/flame/flame8Tex");
    puts("PASS real selector: full8, Alt off, all missing/unloadable frames, ordinary/forest, live changes");
}
'''
# Test helper is below selector in generated TU, so split dependency definitions.
split = common.index('static const char fallback')
spawn_stubs = r'''
typedef struct { f32 x,y,z; } Vec3f;
typedef struct { u8 r,g,b,a; } Color_RGBA8;
typedef struct { struct { struct { int enablePosLights; } curRoom; } roomCtx; } PlayState;
typedef struct { struct { Vec3f tip; } meleeWeaponInfo[1]; f32 unk_85C,unk_B0C; s16 unk_860,unk_B28; int lightInfo; } Player;
#define DECR(x) ((x) != 0 ? --(x) : 0)
#define ITEM_NONE 0
#define ITEM_STICK 1
#define ITEM_DEKU_STICK 1
#define VB_DEKU_STICK_BURN_OUT 0
#define VB_DEKU_STICK_BURN_DOWN 1
#ifndef MM
static int GameInteractor_Should(int behavior, int value) { (void)behavior; return value; }
#else
#define MREG(i) 0
static float Rand_ZeroOne(void) { return 0.5f; }
static void Lights_PointSetColorAndRadius(int* light, float r,float g,float b,float radius) {
    (void)light; (void)r; (void)g; (void)b; (void)radius;
}
#endif
static int used_item, ammo;
static void Player_UseItem(PlayState* play, Player* player, int item) { (void)play; (void)player; (void)item; used_item++; }
static void Inventory_ChangeAmmo(int item, int amount) { (void)item; ammo+=amount; }
typedef struct { int count; PlayState* play; u16 flags; Vec3f *pos,*velocity,*accel; Color_RGBA8 *prim,*env; s16 scale,step,life; u8 update; } Capture;
static Capture captured;
static void EffectSsDust_Spawn(PlayState* play,u16 flags,Vec3f* pos,Vec3f* velocity,Vec3f* accel,
    Color_RGBA8* prim,Color_RGBA8* env,s16 scale,s16 step,s16 life,u8 update) {
    captured=(Capture){captured.count+1,play,flags,pos,velocity,accel,prim,env,scale,step,life,update};
}
'''
temporary = tempfile.TemporaryDirectory(prefix='deku-stick-flame-')
for host, caller, wrapper, prefix in [('soh','Player_UpdateBurningDekuStick','func_8002836C','D_808547A4'),('mm','func_808442D8','func_800B0EB0','D_8085D364')]:
    dust_path = Path(host)/'src/overlays/effects/ovl_Effect_Ss_Dust/z_eff_ss_dust.c'
    player_path = Path(host)/'src/overlays/actors/ovl_player_actor/z_player.c'
    dust = (a.source_root/dust_path).read_text()
    player = (a.source_root/player_path).read_text()
    original_player = (HERE / (host + '_burn_parent.inc')).read_text()
    wrapper_src = (ROOT/host/'src/code/z_effect_soft_sprite_old_init.c').read_text()
    # The engine wrapper is extracted unchanged from immutable baseline.
    selector = function(dust,'EffectSsDust_SelectTexture')
    caller_body = function(player,caller)
    baseline_body = function(original_player,caller).replace(caller+'(',caller+'_baseline(',1)
    data_start = player.rfind('\n',0,player.index('Vec3f '+prefix)) + 1
    data = player[data_start:player.index('void '+caller+'(')]
    spawn_tests = r'''
static void spawn_test(void) {
    PlayState play={0};
    /* Exercise initial ramp, steady burn, shrink, burn out and already exhausted. */
    int timers[]={211,210,205,201,200,20,19,10,2,1,0};
    for (unsigned i=0;i<ARRAY_COUNT(timers);++i) for (int exhausted=0;exhausted<2;++exhausted) {
        Player old={0};
        old.unk_85C=old.unk_B0C=exhausted ? 0.0f : 1.0f;
        old.unk_860=old.unk_B28=timers[i];
        Player current=old;
        captured=(Capture){0}; used_item=ammo=0;
        CALLER_baseline(&play,&old);
        Capture baseline=captured; int old_item=used_item, old_ammo=ammo;
        captured=(Capture){0}; used_item=ammo=0;
        CALLER(&play,&current);
        assert(used_item==old_item && ammo==old_ammo);
        assert(old.unk_85C==current.unk_85C && old.unk_B0C==current.unk_B0C);
        assert(old.unk_860==current.unk_860 && old.unk_B28==current.unk_B28);
        assert(captured.count==baseline.count);
        if (captured.count) {
            assert(captured.flags==(baseline.flags|0x200));
            assert((captured.flags&0xff)==0);
            assert(captured.play==baseline.play);
            assert(captured.pos==&current.meleeWeaponInfo[0].tip);
            assert(baseline.pos==&old.meleeWeaponInfo[0].tip);
            assert(captured.velocity==baseline.velocity && captured.accel==baseline.accel);
            assert(captured.prim==baseline.prim && captured.env==baseline.env);
            assert(captured.velocity->x==0 && captured.velocity->y==0.5f && captured.velocity->z==0);
            assert(captured.accel->x==0 && captured.accel->y==0.5f && captured.accel->z==0);
            assert(captured.prim->r==255 && captured.prim->g==255 && captured.prim->b==100 && captured.prim->a==255);
            assert(captured.env->r==255 && captured.env->g==50 && captured.env->b==0 && captured.env->a==0);
            assert(captured.scale==baseline.scale && captured.step==0 && captured.life==8 && captured.update==0);
        }
    }
    puts("PASS real lit-stick caller vs unchanged native wrapper: all arguments and burn branches");
}
int main(void) { selection_test(); spawn_test(); return 0; }
'''.replace('CALLER',caller)
    output=Path(temporary.name)/host
    output.mkdir(parents=True,exist_ok=True)
    cfile=output/'regression.c'
    cfile.write_text(common[:split]+selector+'\n'+common[split:]+spawn_stubs+'\n'+data+function(wrapper_src,wrapper)+'\n'+baseline_body+'\n'+caller_body+'\n'+spawn_tests)
    command=['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-sign-compare','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
    if host=='mm': command+=['-DMM']
    command += [str(cfile),'-o',str(output/'regression')]
    print('COMPILE',host,flush=True)
    subprocess.run(command,check=True)
    print('RUN',host,flush=True)
    result=subprocess.run([str(output/'regression')], env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
    if result.returncode: sys.exit(1)
    header=(a.source_root/dust_path.with_suffix('.h')).read_text()
    assert re.search(r'^#define EFFECT_SS_DUST_DRAW_DEKU_STICK_FLAME 0x200$',header,re.M)
    assert player.count('EFFECT_SS_DUST_DRAW_DEKU_STICK_FLAME')==1
print('PASS both engines',flush=True)
