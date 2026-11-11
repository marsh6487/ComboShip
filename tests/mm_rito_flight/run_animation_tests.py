#!/usr/bin/env python3
"""Compile actual MM flight resource/landing-table functions with real animation headers."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function

flight = (ROOT / 'mm/mods/forms/rito_flight.inc.c').read_text()
internal = (ROOT / 'mm/mods/forms/forms_internal.h').read_text()
fixture = (ROOT / 'tests/mm_rito_flight/native_test.cpp').read_text()
headers = fixture[:fixture.index('SaveContext gSaveContext;')]
defines = '\n'.join(line for line in flight.replace('\\\n', ' ').splitlines()
                    if line.startswith('#define RITO_'))
state = flight[flight.index('typedef enum {'):flight.index('// Landing the way')]
code = headers + '#include "objects/gameplay_keep/gameplay_keep.h"\n' + defines + '\n' + state + r'''
#define SPDLOG_WARN(...) ((void)0)
static u8 sRitoNativeLandFallback;
static int resources, resamples, tableWrites;
static PlayerAnimationHeader authoredClip, originalLanding, originalShort;
static PlayerAnimationHeader* landing[PLAYER_ANIMTYPE_MAX];
static PlayerAnimationHeader* shortLanding[PLAYER_ANIMTYPE_MAX];
static bool ResourceMgr_FileExists(const char* path) {
    return resources == 2 || (resources == 1 && strcmp(path, RITO_CLIP_FLY) == 0);
}
static PlayerAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeader(const char*) { return &authoredClip; }
static PlayerAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceResampled(const char* path, u8 stripY, s16 frames) {
    assert(strstr(path, "__OTR__misc/link_animetion/") == path && stripY == 1);
    assert(frames == 30 || frames == 2);
    ++resamples;
    return &authoredClip;
}
static PlayerAnimationHeader* ExtPlayer_GetAnimGroupAnim(s32 group, s32 col) {
    return group == PLAYER_ANIMGROUP_landing ? landing[col] : shortLanding[col];
}
static void ExtPlayer_SetAnimGroupAnim(s32 group, s32 col, PlayerAnimationHeader* anim) {
    ++tableWrites;
    (group == PLAYER_ANIMGROUP_landing ? landing[col] : shortLanding[col]) = anim;
}
''' + function(internal, 'Forms_VanillaAnim') + '\n' + '\n'.join(
    function(flight, name) for name in ('MmRitoInstallLanding', 'MmRitoRestoreLanding',
                                      'MmRitoLoadClip', 'MmRitoLoadClips')) + r'''
static void run(int available) {
    sRito = {}; sRitoNativeLandFallback = 0; resamples = tableWrites = 0; resources = available;
    for (int i = 0; i < PLAYER_ANIMTYPE_MAX; ++i) {
        landing[i] = &originalLanding; shortLanding[i] = &originalShort;
    }
    MmRitoLoadClips();
    assert(sRito.charge_ == (PlayerAnimationHeader*)gPlayerAnim_link_bow_defense_wait);
    assert(sRito.fly && sRito.launch && sRito.land && sRito.hopSettle && sRito.throwItem);
    for (auto* clip : sRito.hopClips) assert(clip);
    assert(resamples == (available == 2 ? 9 : available));
    if (available < 2) {
        assert(sRito.launch == Forms_VanillaAnim(gPlayerAnim_link_normal_jump));
        assert(sRito.land == Forms_VanillaAnim(gPlayerAnim_link_normal_landing));
        assert(sRitoNativeLandFallback);
    } else {
        assert(sRito.launch == &authoredClip && !sRitoNativeLandFallback);
    }
    assert(sRito.fly == (available ? &authoredClip : Forms_VanillaAnim(gPlayerAnim_link_normal_jump)));
    for (int i = 0; i < PLAYER_ANIMTYPE_MAX; ++i)
        assert(landing[i] == sRito.land && shortLanding[i] == sRito.land);
    const int before = resamples;
    MmRitoLoadClips();
    assert(resamples == before && tableWrites == 2 * PLAYER_ANIMTYPE_MAX);
    MmRitoRestoreLanding();
    for (int i = 0; i < PLAYER_ANIMTYPE_MAX; ++i)
        assert(landing[i] == &originalLanding && shortLanding[i] == &originalShort);
}
int main() {
    authoredClip.common.frameCount = 60;
    run(0); run(1); run(2);
    authoredClip.common.frameCount = 1;
    assert(MmRitoLoadClip(RITO_CLIP_FLY) == &authoredClip); // minimum resampled frame count
    std::puts("PASS actual MM flight loader: no pack/native fallback, wingbeat-only pack, full authored priority, resampling and landing-table restore");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-rito-animation-') as temp:
    build = Path(temp)
    (build / 'test.cpp').write_text(code)
    includes = ('mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets',
                'mm/2s2h', 'libultraship/include', 'combo', 'combo/menu')
    for mode in ('original', 'combo'):
        executable = build / mode
        subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20', '-Wall', '-Wextra',
                        '-Wno-unused-parameter', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
                        '-DCONTROLLERBUTTONS_T=uint32_t', '-DMM_BUILD_DLL',
                        *(['-DCOMBO_BUILD'] if mode == 'combo' else []),
                        *['-I' + str(ROOT / p) for p in includes],
                        str(build / 'test.cpp'), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        print('PASS native MM Rito animation', mode)
