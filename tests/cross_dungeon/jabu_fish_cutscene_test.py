#!/usr/bin/env python3
"""Compile Jabu's normal/skipped feeding exits and resolver in both build modes.

Engine discovery, native-map lookup, grotto materialization and the cross-runtime
boundary are controlled fixtures. The cutscene guard and resolver composition
and timesaver hook are production code; the native-shuffle gate breaks cross-only entry.
This checks routing selection; it does not execute the game or its cutscene frames.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compiler', default=os.environ.get('CXX', 'g++'))
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
demo = (ROOT / 'soh/src/code/z_demo.c').read_text()
native = (ROOT / 'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text()
switch = demo.index('switch (cmd->base)')
start = demo.index('            case 24:', switch)
case = demo[start:demo.index('            case 25:', start)]
resolver = function(native, 's16 Entrance_OverrideNextIndex(')
timesavers = (ROOT / 'soh/soh/Enhancements/TimeSavers/timesaver_hook_handlers.cpp').read_text()
skip = function(timesavers, 'static void SkipJabuFeedingCutscene(')
start = timesavers.index('        case VB_JABU_JABU_EAT_FISH:')
hook = timesavers[start:timesavers.index('        case VB_PLAY_BEAN_PLANTING_CS:', start)]

prefix = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
using s16 = int16_t;
enum {
    ENTR_JABU_JABU_ENTRANCE = 0x28,
    ENTR_CASTLE_GROUNDS_RAINBOW_BRIDGE_EXIT = 0x23D,
    SCENE_CASTLE_COURTYARD_GUARDS_DAY = 0x45,
    SCENE_CASTLE_COURTYARD_GUARDS_NIGHT = 0x46,
    RSK_SHUFFLE_ENTRANCES = 1,
    TRANS_TRIGGER_START = 1,
    TRANS_TYPE_FADE_BLACK = 2,
    ITEM_BOTTLE = 1, PLAYER_IA_BOTTLE = 2,
    EVENTCHKINF_OFFERED_FISH_TO_JABU_JABU = 0x3A,
    NA_SE_SY_CORRECT_CHIME = 3,
    VB_JABU_JABU_EAT_FISH = 4
};
struct PlayState {
    int sceneNum = 89;
    s16 nextEntranceIndex = -1;
    int transitionTrigger = 0, transitionType = -1;
};
static PlayState* gPlayState;
static bool isRando, nativeShuffle, combined;
static int crossKind, pendingPeer, discoveries, nativeLookups, crossLookups;
static int discoveredIndex;
static bool skipEnabled;
static int bottleUpdates, fishFlags, chimes;
struct Player {};
static Player player;
#define GET_PLAYER(play) (&player)
#define RAND_GET_OPTION(key) Randomizer_GetSettingValue(key)
#define CVAR_ENHANCEMENT(key) key
int CVarGetInteger(const char* key, int) {
    assert(!strcmp(key, "TimeSavers.SkipMiscInteractions"));
    return skipEnabled;
}
void Player_UpdateBottleHeld(PlayState* play, Player* actor, int item, int action) {
    assert(play == gPlayState && actor == &player && item == ITEM_BOTTLE && action == PLAYER_IA_BOTTLE);
    ++bottleUpdates;
}
void Flags_SetEventChkInf(int flag) { assert(flag == 0x3A); ++fishFlags; }
void Sfx_PlaySfxCentered(int sound) { assert(sound == NA_SE_SY_CORRECT_CHIME); ++chimes; }
#define IS_RANDO isRando
#define LUSLOG_INFO(...) ((void)0)
int Randomizer_GetSettingValue(int key) {
    assert(key == RSK_SHUFFLE_ENTRANCES);
    return nativeShuffle;
}
void Entrance_SetEntranceDiscovered(s16 index, bool reversed) {
    assert(!reversed);
    discoveredIndex = index;
    ++discoveries;
}
void EntranceTracker_SetLastEntranceOverride(s16 index) { assert(index == 0x28); }
s16 Entrance_GetOverride(s16 index) {
    ++nativeLookups;
    assert(index == 0x28 && "apply native permutation once, to the source mouth");
    return nativeShuffle ? 0x82 : index;
}
s16 Grotto_OverrideSpecialEntrance(s16 index) { return index; }
s16 Entrance_TraceResolved(const char*, s16, s16, s16 materialized) { return materialized; }
#ifdef COMBO_BUILD
bool Combo_IsCombinedGame() { return combined; }
int32_t Combo_SOH_CrossDungeonRoute(int32_t index, int beforeNative) {
    ++crossLookups;
    if (beforeNative) {
        assert(index == 0x28);
        return -1; // This fixture has no reverse/BeforeNative leg at the mouth.
    }
    assert(index == (nativeShuffle ? 0x82 : 0x28));
    if (crossKind == 1)
        return 0x100; // Local dungeon.
    if (crossKind == 2) {
        pendingPeer = 0x2600; // Foreign index belongs to MM, never the native loader.
        return index; // Production cross-runtime contract: retain host placeholder.
    }
    return -1;
}
#endif
'''
main = r'''
static void ResetAudit() {
    pendingPeer = -1;
    discoveries = nativeLookups = crossLookups = 0;
    discoveredIndex = -1;
    bottleUpdates = fishFlags = chimes = 0;
}
int main() {
    int cases = 0;
    for (int skipped = 0; skipped < 2; ++skipped)
    for (int rando = 0; rando < 2; ++rando)
    for (int native = 0; native < 2; ++native)
    for (int cross = 0; cross < 3; ++cross)
    for (int combo = 0; combo < 2; ++combo) {
        isRando = rando;
        nativeShuffle = native;
        crossKind = cross;
        combined = combo;
        bool crossAllowed = false;
#ifdef COMBO_BUILD
        crossAllowed = combined && isRando;
#endif
        const int wantNative = isRando && nativeShuffle ? 0x82 : 0x28;
        const int wantDestination = crossAllowed && crossKind == 1 ? 0x100 : wantNative;
        const int wantPeer = crossAllowed && crossKind == 2 ? 0x2600 : -1;
        PlayState play;
        gPlayState = &play;
        ResetAudit();
        skipEnabled = true;
        bool should = true;
        if (skipped) {
            FeedingHook(&should);
            assert(!should && bottleUpdates == 1 && fishFlags == 1 && chimes == 1);
        } else {
            FeedCutsceneExit(&play);
            assert(bottleUpdates == 0 && fishFlags == 0 && chimes == 0);
        }
        if (play.nextEntranceIndex != wantDestination || pendingPeer != wantPeer) {
            std::fprintf(stderr, "Jabu fish mismatch: rando=%d native=%d cross=%d combined=%d "
                         "destination=%#x want=%#x peer=%#x wantPeer=%#x\n",
                         rando, native, cross, combo, play.nextEntranceIndex,
                         wantDestination, pendingPeer, wantPeer);
            return 1;
        }
        assert(play.transitionTrigger == TRANS_TRIGGER_START);
        assert(play.transitionType == TRANS_TYPE_FADE_BLACK);
        if (isRando && (nativeShuffle || (crossAllowed && crossKind != 0))) {
            assert(discoveries == 1 && discoveredIndex == 0x28);
            assert(nativeLookups == 1);
            assert(crossLookups == (crossAllowed ? 2 : 0));
        }
        assert(nativeLookups <= 1 && crossLookups <= 2);
        if (!isRando)
            assert(discoveries == 0 && nativeLookups == 0 && crossLookups == 0);
        // Compare the regular physical-mouth path with the same literal expectations.
        ResetAudit();
        const int walkDestination = isRando ? Entrance_OverrideNextIndex(0x28) : 0x28;
        assert(walkDestination == wantDestination && pendingPeer == wantPeer);
        ++cases;
    }
    for (int enabled = 0; enabled < 2; ++enabled) {
        PlayState play;
        gPlayState = &play;
        skipEnabled = enabled;
        bool should = false;
        ResetAudit();
        FeedingHook(&should);
        assert(!should && play.nextEntranceIndex == -1 && discoveries == 0 && fishFlags == 0 && bottleUpdates == 0 && chimes == 0);
        if (!enabled) {
            should = true;
            FeedingHook(&should);
            assert(should && play.nextEntranceIndex == -1 && discoveries == 0 && fishFlags == 0 && bottleUpdates == 0 && chimes == 0);
        }
    }
    std::printf("PASS: %d normal/skipped Jabu fish/regular-mouth routing cases and disabled-hook guards\n", cases);
}
'''

source = (prefix + resolver + '\nstatic void FeedCutsceneExit(PlayState* play) {\n'
          + '    switch (24) {\n' + case + '    }\n}\n' + skip
          + '\nstatic void FeedingHook(bool* should) { switch ((int)VB_JABU_JABU_EAT_FISH) {\n'
          + hook + ' } }\n' + main)
with tempfile.TemporaryDirectory(prefix='jabu-fish-cutscene-') as temporary:
    directory = Path(temporary)
    cpp = directory / 'test.cpp'
    cpp.write_text(source)
    for combo in (False, True):
        binary = directory / ('combo' if combo else 'standalone')
        command = [args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wpedantic']
        if combo:
            command += ['-DCOMBO_BUILD']
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
        subprocess.run(command + [str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print(f'PASS: {binary.name}', flush=True)
