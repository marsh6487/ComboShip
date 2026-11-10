#!/usr/bin/env python3
"""Exercise actual Rito clip selection when optional authored packs are absent/present."""
from pathlib import Path
import re
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
flight = (ROOT/'soh/mods/transformation_masks/rito_flight.inc.c').read_text()
bow = (ROOT/'soh/mods/transformation_masks/rito_bow.inc.c').read_text()

def function(source, name):
    match = re.search(r'^(?:extern "C" |static )?(?:void|f32|u8|LinkAnimationHeader\*) '+name+r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, name
    end,depth=match.end(),1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]

def struct(source,name):
    return re.search(r'typedef struct \{[^{}]*\} '+name+r';',source,re.S)[0]

def defines(source):
    logical = source.replace("\\\n", " ")
    return '\n'.join(line for line in logical.splitlines() if line.startswith('#define RITO_'))

native_header=ROOT/'soh/assets/objects/gameplay_keep/gameplay_keep.h'
for symbol in ('gPlayerAnim_link_normal_jump','gPlayerAnim_link_normal_landing','gPlayerAnim_link_bow_bow_ready',
               'gPlayerAnim_link_bow_bow_wait','gPlayerAnim_link_bow_bow_shoot_end','gPlayerAnim_link_bow_bow_shoot_next'):
    assert symbol in native_header.read_text()
code=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "assets/objects/gameplay_keep/gameplay_keep.h"
using u8=uint8_t;using s16=int16_t;using f32=float;using PlayerActionFunc=void*;
struct Actor{};struct Vec3f{float x,y,z;};
struct LinkAnimationHeader{struct{int16_t frameCount;}common;};
struct Player{struct{LinkAnimationHeader* animation;}skelAnime;};
#define SPDLOG_WARN(...) ((void)0)
'''+defines(flight)+'\n'+defines(bow)+'\n'+struct(flight,'RitoFlight')+'\n'+struct(bow,'RitoBowArrow')+'\n'+struct(bow,'RitoBow')+r'''
static RitoFlight sRito;
static RitoBow sRitoBow;
static bool authored=false;
static int resamples=0,installs=0;
static LinkAnimationHeader customClip={{60}};
static bool ResourceMgr_FileExists(const char* path){return authored || strcmp(path,RITO_CLIP_FLY)==0;}
static LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeader(const char*){return &customClip;}
static LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceResampled(const char* path,int,int frames){
    assert(frames>=2);
    assert(strstr(path,"__OTR__misc/link_animetion/")==path);
    ++resamples;return &customClip;
}
static void MmForm_RitoInstallLanding(){++installs;}
'''+ '\n'.join(function(flight,n) for n in ('MmForm_RitoLoadClip','MmForm_RitoLoadClips'))+'\n'+ '\n'.join(function(bow,n) for n in ('MmForm_RitoBowLoadClip','MmForm_RitoBowLoadClips','MmForm_RitoBowDrawLift','MmForm_RitoBowUsesNativePose'))+'\n'+function(flight,'MmForm_RitoDrawYOffset')+r'''
int main(){
    MmForm_RitoLoadClips();MmForm_RitoBowLoadClips();
    assert(sRito.launch && sRito.fly && sRito.land && sRito.hopSettle && sRito.throwItem);
    for(auto* clip:sRito.hopClips)assert(clip);
    assert(sRitoBow.air && sRitoBow.enterStill && sRitoBow.holdStill && sRitoBow.enterMove && sRitoBow.holdMove);
    assert(sRitoBow.release && sRitoBow.seed && sRitoBow.airTap);
    assert(resamples==1); // Shipped wingbeat only; no native clip is mutated.
    assert(sRito.landUsesNative && sRitoBow.seedUsesNative && sRitoBow.releaseUsesNative);
    Player p{};
    p.skelAnime.animation=sRito.land;assert(MmForm_RitoDrawYOffset(&p)==RITO_DRAW_Y_BASE);
    p.skelAnime.animation=sRitoBow.seed;assert(MmForm_RitoBowDrawLift(&p)==0 && MmForm_RitoBowUsesNativePose(&p));
    p.skelAnime.animation=sRitoBow.release;assert(MmForm_RitoBowDrawLift(&p)==0);
    MmForm_RitoLoadClips();MmForm_RitoBowLoadClips();assert(resamples==1 && installs==1);
    authored=true;sRito={};sRitoBow={};resamples=0;
    MmForm_RitoLoadClips();MmForm_RitoBowLoadClips();
    assert(!sRito.landUsesNative && !sRitoBow.seedUsesNative && !sRitoBow.releaseUsesNative);
    assert(sRito.launch==&customClip && sRitoBow.holdStill==&customClip);
    p.skelAnime.animation=sRitoBow.holdStill;assert(!MmForm_RitoBowUsesNativePose(&p));
    assert(resamples==17); // Nine flight clips and eight bow clips.
    p.skelAnime.animation=sRito.land;assert(MmForm_RitoDrawYOffset(&p)==RITO_DRAW_Y_BASE+RITO_LAND_LIFT);
    puts("PASS Rito native clip fallback keeps flight/bow usable, never resamples native clips, preserves authored priority and authored-only root offsets");
}
'''
with tempfile.TemporaryDirectory(prefix='rito-animation-') as td:
    p=Path(td);(p/'test.cpp').write_text(code)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT/'soh'),'-I'+str(ROOT/'soh/include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
